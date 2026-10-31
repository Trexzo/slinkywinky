package dev.slinkywinky.instrument;
import java.lang.reflect.*;
/** Strict read-only 1.8.9 field sampler; missing mappings reject admission. */
public final class ObservedState {
    public static final class Frame {
        public final long timeMs,generation;
        public final int flags; // world=1, player=2, targeted entity=4
        public final double[] xyz;
        public final float[] angles;
        Frame(long t,long g,int f,double[] p,float[] a) {
            timeMs=t;generation=g;flags=f;xyz=p;angles=a;
        }
    }
    private static Class<?> mcClass;
    private static Method getter;
    private static Field worldField,playerField,rayField;
    private static Object oldWorld,oldPlayer;
    private static long generation=1;
    private static Field field(Class<?> type,String...names) throws NoSuchFieldException {
        for(Class<?> c=type;c!=null;c=c.getSuperclass())
            for(String name:names)
                try { Field f=c.getDeclaredField(name);f.setAccessible(true);return f; }
                catch(NoSuchFieldException ignored) { }
        throw new NoSuchFieldException(type.getName()+" "+java.util.Arrays.toString(names));
    }
    public static synchronized void admit(Class<?> target) throws ReflectiveOperationException {
        Method get=null;
        for(String name:new String[]{"getMinecraft","func_71410_x"}) {
            try { Method m=target.getDeclaredMethod(name);
                  if(Modifier.isStatic(m.getModifiers()) && m.getParameterTypes().length==0) {
                      m.setAccessible(true);get=m;break;
                  } } catch(NoSuchMethodException ignored) {}
        }
        if(get==null) throw new NoSuchMethodException("Exact Minecraft instance getter unavailable");
        Field world=field(target,"theWorld","field_71441_e");
        Field player=field(target,"thePlayer","field_71439_g");
        Field ray=field(target,"objectMouseOver","field_71476_x");
        mcClass=target;getter=get;worldField=world;playerField=player;rayField=ray;
        oldWorld=null;oldPlayer=null;generation=1;
    }
    private static double num(Object o,String...names) throws ReflectiveOperationException {
        Object raw=field(o.getClass(),names).get(o);
        if(!(raw instanceof Number)) throw new IllegalStateException("Non-numeric entity member");
        double n=((Number)raw).doubleValue();
        if(!Double.isFinite(n)) throw new IllegalStateException("Nonfinite observation");
        return n;
    }
    private static float eyeHeight(Object player) throws ReflectiveOperationException {
        for(Class<?> c=player.getClass();c!=null;c=c.getSuperclass()) {
            for(String name:new String[]{"getEyeHeight","func_70047_e"}) {
                try {
                    Method m=c.getDeclaredMethod(name);
                    if(m.getParameterTypes().length!=0) continue;
                    m.setAccessible(true);
                    Object value=m.invoke(player);
                    if(!(value instanceof Number)) throw new IllegalStateException("Invalid eye height type");
                    float height=((Number)value).floatValue();
                    if(!Float.isFinite(height) || height<0 || height>3)
                        throw new IllegalStateException("Invalid eye height range");
                    return height;
                } catch(NoSuchMethodException ignored) { }
            }
        }
        throw new NoSuchMethodException("Player eye height mapping unavailable");
    }
    private static float angle(Object o,String...names) throws ReflectiveOperationException {
        double n=num(o,names);
        if(Math.abs(n)>Float.MAX_VALUE) throw new IllegalStateException("Rotation overflow");
        return (float)n;
    }
    public static synchronized Frame sample() throws ReflectiveOperationException {
        if(mcClass==null) throw new IllegalStateException("Sampler not admitted");
        Object mc=getter.invoke(null);
        if(mc==null) throw new IllegalStateException("Minecraft instance unavailable");
        Object world=worldField.get(mc),player=playerField.get(mc);
        if(oldWorld!=world || oldPlayer!=player) {
            generation++;oldWorld=world;oldPlayer=player;
        }
        int flags=(world==null?0:1)|(player==null?0:2);
        double[] xyz=new double[6];
        float[] angles=new float[2];
        if(world!=null && player!=null) {
            xyz[0]=num(player,"posX","field_70165_t");
            xyz[1]=num(player,"posY","field_70163_u")+eyeHeight(player);
            xyz[2]=num(player,"posZ","field_70161_v");
            angles[0]=angle(player,"rotationYaw","field_70177_z");
            angles[1]=angle(player,"rotationPitch","field_70125_A");
            Object ray=rayField.get(mc);
            if(ray!=null) {
                Object entity=field(ray.getClass(),"entityHit","field_72308_g").get(ray);
                if(entity!=null) {
                    xyz[3]=num(entity,"posX","field_70165_t");
                    xyz[4]=num(entity,"posY","field_70163_u");
                    xyz[5]=num(entity,"posZ","field_70161_v");
                    flags|=4;
                }
            }
        }
        return new Frame(System.nanoTime()/1000000L,generation,flags,xyz,angles);
    }
    private ObservedState() { }
}
