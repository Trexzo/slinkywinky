package dev.slinkywinky.instrument;

import java.lang.reflect.Field;
import dev.slinkywinky.SourcePacketReadOnlyQueue;

/**
 * R365: read-only outbound original 1.8.9 Packet (ff) sampler.
 * Never holds the Packet itself; ip.g:Z and ip.a/b/c:D come from the exact
 * original SHA-pinned C03PacketPlayer JNI mapping and are copied by value.
 */
public final class Vanilla189OutboundPacketSampler implements AutoCloseable {
    public static final class Sample {
        public final int kind;
        public final boolean positionKnown;
        public final double x,y,z;
        private Sample(int kind,boolean known,double xx,double yy,double zz){
            this.kind=kind;positionKnown=known;x=xx;y=yy;z=zz;
        }
    }
    private final Thread owner=Thread.currentThread();
    private Class<?> packetBase,movementBase;
    private Field moving,x,y,z;
    private volatile boolean closed;

    public Vanilla189OutboundPacketSampler(Class<?> packetClass,Class<?> movingClass)
            throws ReflectiveOperationException{
        if(packetClass==null||movingClass==null||
           !"ff".equals(packetClass.getName())||
           !"ip".equals(movingClass.getName())||
           packetClass.getClassLoader()!=movingClass.getClassLoader()||
           !packetClass.isAssignableFrom(movingClass))
            throw new IllegalArgumentException("Untrusted original Packet/C03 class identity");
        Field movingField=exact(movingClass,"g",Boolean.TYPE);
        Field xField=exact(movingClass,"a",Double.TYPE);
        Field yField=exact(movingClass,"b",Double.TYPE);
        Field zField=exact(movingClass,"c",Double.TYPE);
        packetBase=packetClass;movementBase=movingClass;
        moving=movingField;x=xField;y=yField;z=zField;
    }
    private static Field exact(Class<?> cls,String name,Class<?> type)
            throws ReflectiveOperationException{
        Field f=cls.getDeclaredField(name);
        if(f.getType()!=type)
            throw new NoSuchFieldException("Wrong original 1.8.9 packet field "+name);
        f.setAccessible(true);
        return f;
    }
    /** Netty or Minecraft sender thread. Returns primitives, no side effects. */
    public Sample sample(Object packet)throws ReflectiveOperationException{
        if(closed)throw new IllegalStateException("R365 packet snapshot owner retired");
        if(packet==null||!packetBase.isInstance(packet))
            throw new IllegalArgumentException("Untrusted non-ff outbound Packet");
        if(!movementBase.isInstance(packet))
            return new Sample(SourcePacketReadOnlyQueue.UNKNOWN,false,0,0,0);
        final boolean hasPosition=moving.getBoolean(packet);
        if(!hasPosition)
            return new Sample(SourcePacketReadOnlyQueue.MOVEMENT,false,0,0,0);
        final double xx=x.getDouble(packet),yy=y.getDouble(packet),zz=z.getDouble(packet);
        if(!Double.isFinite(xx)||!Double.isFinite(yy)||!Double.isFinite(zz)||
           Math.abs(xx)>30000000d||Math.abs(yy)>30000000d||
           Math.abs(zz)>30000000d)
            throw new IllegalStateException("Unbounded source packet movement");
        if(closed)throw new IllegalStateException("Packet sampler retired mid-snapshot");
        return new Sample(SourcePacketReadOnlyQueue.MOVEMENT,true,xx,yy,zz);
    }
    @Override public void close(){
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("Only Minecraft owner retires packet sampler");
        closed=true;
        packetBase=movementBase=null;
        moving=x=y=z=null;
    }
}
