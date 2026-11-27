package dev.slinkywinky.instrument;

import java.lang.reflect.Field;

/**
 * R358: original Minecraft 1.8.9 read-only client-thread source evidence.
 * Only SHA-verified vanilla source class/field names are admitted; no entity
 * reference crosses the returned immutable value snapshot or JNI.
 */
public final class Vanilla189SourceEvidence {
    public static final class Sample {
        public final double x,y,z;
        public final int lastAttackTick;
        private Sample(double x,double y,double z,int attack){
            this.x=x;this.y=y;this.z=z;lastAttackTick=attack;
        }
    }
    private static Class<?> playerClass,targetClass;
    private static Field attack,x,y,z;

    private Vanilla189SourceEvidence(){}

    private static Class<?> livingOwner(Class<?> start){
        for(Class<?> c=start;c!=null;c=c.getSuperclass())
            if("pr".equals(c.getName()))return c;
        return null;
    }
    private static Field exact(Class<?> owner,String name,Class<?> type)
        throws ReflectiveOperationException{
        if(owner==null)throw new NoSuchFieldException("No original living-owner class");
        Field f=owner.getDeclaredField(name);
        if(f.getType()!=type)throw new NoSuchFieldException("Wrong original field type");
        f.setAccessible(true);
        return f;
    }
    /**
     * Non-player ray selections intentionally return null. Mismatched original
     * classes, field types, nonfinite coordinates and inaccessible fields deny.
     * Call ONLY from the already admitted Minecraft client thread.
     */
    public static synchronized Sample read(Object player,Object target)
            throws ReflectiveOperationException{
        if(player==null||target==null||
           !"bex".equals(target.getClass().getName()))return null;
        final Class<?> p=player.getClass(),t=target.getClass();
        if(p!=playerClass||t!=targetClass||attack==null||x==null||y==null||z==null){
            final Class<?> owner=livingOwner(p);
            final Field newAttack=exact(owner,"bl",Integer.TYPE);
            final Field newX=exact(t,"c",Double.TYPE);
            final Field newY=exact(t,"d",Double.TYPE);
            final Field newZ=exact(t,"e",Double.TYPE);
            playerClass=p;targetClass=t;
            attack=newAttack;x=newX;y=newY;z=newZ;
        }
        int last=attack.getInt(player);
        double xx=x.getDouble(target),yy=y.getDouble(target),zz=z.getDouble(target);
        if(!Double.isFinite(xx)||!Double.isFinite(yy)||!Double.isFinite(zz)||
           Math.abs(xx)>30000000d||Math.abs(yy)>30000000d||
           Math.abs(zz)>30000000d||last<0)
            throw new IllegalStateException("R358 invalid original source evidence");
        return new Sample(xx,yy,zz,last);
    }
    public static synchronized void release(){
        playerClass=targetClass=null;
        attack=x=y=z=null;
    }
}
