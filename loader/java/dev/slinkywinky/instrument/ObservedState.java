package dev.slinkywinky.instrument;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * Minecraft 1.8.9 read-only observation sampler. Reflection bindings are
 * resolved only when a runtime class changes, never on every sampled tick.
 * All cache entries are bounded to one current player/target/ray class each,
 * and release() removes every strong reference.
 */
public final class ObservedState {
    public static final class Frame {
        public final long timeMs, generation;
        public final int flags; // world=1, player=2, ray-selected entity=4
        public final double[] xyz; // player eye XYZ, selected entity XYZ
        public final float[] angles; // yaw, pitch

        Frame(long t, long g, int f, double[] p, float[] a) {
            timeMs=t; generation=g; flags=f; xyz=p; angles=a;
        }
    }

    private static Class<?> mcClass;
    private static Method getter;
    private static Field worldField, playerField, rayField;
    private static Object oldWorld, oldPlayer;
    private static long generation=1;
    private static boolean vanillaObfuscated;

    private static final class PositionFields {
        final Field x,y,z;
        PositionFields(Class<?> cls) throws NoSuchFieldException {
            x=choose(cls,"s","posX","field_70165_t");
            y=choose(cls,"t","posY","field_70163_u");
            z=choose(cls,"u","posZ","field_70161_v");
        }
    }
    private static final class PlayerFields {
        final Class<?> type;
        final PositionFields position;
        final Field yaw, pitch;
        final Method eyeHeight;
        PlayerFields(Class<?> cls) throws NoSuchFieldException, NoSuchMethodException {
            type=cls;
            position=new PositionFields(cls);
            yaw=choose(cls,"y","rotationYaw","field_70177_z");
            pitch=choose(cls,"z","rotationPitch","field_70125_A");
            eyeHeight=eyeHeightMethod(cls);
        }
    }
    private static final class TargetFields {
        final Class<?> type;
        final PositionFields position;
        TargetFields(Class<?> cls) throws NoSuchFieldException {
            type=cls;
            position=new PositionFields(cls);
        }
    }

    private static PlayerFields cachedPlayer;
    private static TargetFields cachedTarget;
    private static Class<?> cachedRayClass;
    private static Field cachedRayEntity;
    // Private regression-only counter; no additional game state or telemetry.
    private static long mappingBuilds;

    private static Field field(Class<?> type,String...names) throws NoSuchFieldException {
        for(Class<?> c=type;c!=null;c=c.getSuperclass()) {
            for(String name:names) {
                try {
                    Field f=c.getDeclaredField(name);
                    f.setAccessible(true);
                    return f;
                } catch(NoSuchFieldException ignored) { }
            }
        }
        throw new NoSuchFieldException(type.getName()+" "+
                                       java.util.Arrays.toString(names));
    }

    private static Field choose(Class<?> type,String original,String named,String srg)
            throws NoSuchFieldException {
        return vanillaObfuscated ? field(type,original) : field(type,named,srg);
    }

    private static Method eyeHeightMethod(Class<?> type) throws NoSuchMethodException {
        String[] names=vanillaObfuscated ? new String[]{"aS"} :
            new String[]{"getEyeHeight","func_70047_e"};
        for(Class<?> c=type;c!=null;c=c.getSuperclass()) {
            for(String name:names) {
                try {
                    Method m=c.getDeclaredMethod(name);
                    if(m.getParameterTypes().length!=0 || Modifier.isStatic(m.getModifiers()))
                        continue;
                    m.setAccessible(true);
                    return m;
                } catch(NoSuchMethodException ignored) { }
            }
        }
        throw new NoSuchMethodException("Player eye height mapping unavailable");
    }

    public static synchronized void admit(Class<?> target) throws ReflectiveOperationException {
        if(target==null) throw new IllegalArgumentException("Null Minecraft class");
        boolean vanilla="ave".equals(target.getName());
        Method get=null;
        String[] names=vanilla ? new String[]{"A"} :
            new String[]{"getMinecraft","func_71410_x"};
        for(String name:names) {
            try {
                Method m=target.getDeclaredMethod(name);
                if(Modifier.isStatic(m.getModifiers()) && m.getParameterTypes().length==0) {
                    m.setAccessible(true);
                    get=m;
                    break;
                }
            } catch(NoSuchMethodException ignored) { }
        }
        if(get==null) throw new NoSuchMethodException("Exact Minecraft singleton getter unavailable");
        Field world=vanilla ? field(target,"f") : field(target,"theWorld","field_71441_e");
        Field player=vanilla ? field(target,"h") : field(target,"thePlayer","field_71439_g");
        Field ray=vanilla ? field(target,"s") : field(target,"objectMouseOver","field_71476_x");

        // Only publish a fully validated admission; failed mapping retains old state.
        release();
        mcClass=target;
        getter=get;
        worldField=world;
        playerField=player;
        rayField=ray;
        vanillaObfuscated=vanilla;
    }

    private static double numeric(Field field,Object instance) throws IllegalAccessException {
        Object value=field.get(instance);
        if(!(value instanceof Number))
            throw new IllegalStateException("Non-numeric game field: "+field.getName());
        double n=((Number)value).doubleValue();
        if(!Double.isFinite(n))
            throw new IllegalStateException("Nonfinite game field: "+field.getName());
        return n;
    }

    private static float eyeHeight(PlayerFields fields,Object player)
            throws ReflectiveOperationException {
        Object value=fields.eyeHeight.invoke(player);
        if(!(value instanceof Number))
            throw new IllegalStateException("Invalid eye-height type");
        float height=((Number)value).floatValue();
        if(!Float.isFinite(height) || height<0 || height>3)
            throw new IllegalStateException("Invalid eye-height range");
        return height;
    }

    private static PlayerFields playerBinding(Object player) throws ReflectiveOperationException {
        Class<?> type=player.getClass();
        if(cachedPlayer==null || cachedPlayer.type!=type) {
            PlayerFields next=new PlayerFields(type);
            cachedPlayer=next;
            mappingBuilds++;
        }
        return cachedPlayer;
    }
    private static TargetFields targetBinding(Object target) throws ReflectiveOperationException {
        Class<?> type=target.getClass();
        if(cachedTarget==null || cachedTarget.type!=type) {
            TargetFields next=new TargetFields(type);
            cachedTarget=next;
            mappingBuilds++;
        }
        return cachedTarget;
    }
    private static Field rayEntityBinding(Object ray) throws NoSuchFieldException {
        Class<?> type=ray.getClass();
        if(cachedRayEntity==null || cachedRayClass!=type) {
            Field next=vanillaObfuscated ? field(type,"d") :
                field(type,"entityHit","field_72308_g");
            cachedRayClass=type;
            cachedRayEntity=next;
            mappingBuilds++;
        }
        return cachedRayEntity;
    }

    public static synchronized Frame sample() throws ReflectiveOperationException {
        if(mcClass==null) throw new IllegalStateException("Sampler not admitted");
        Object mc=getter.invoke(null);
        if(mc==null) throw new IllegalStateException("Minecraft instance unavailable");
        Object world=worldField.get(mc),player=playerField.get(mc);
        if(world==null && player!=null)
            throw new IllegalStateException("Player exists without world");

        int flags=(world==null?0:1)|(player==null?0:2);
        double[] xyz=new double[6];
        float[] camera=new float[2];
        if(world!=null && player!=null) {
            PlayerFields bound=playerBinding(player);
            xyz[0]=numeric(bound.position.x,player);
            xyz[1]=numeric(bound.position.y,player)+eyeHeight(bound,player);
            xyz[2]=numeric(bound.position.z,player);
            double yaw=numeric(bound.yaw,player), pitch=numeric(bound.pitch,player);
            if(Math.abs(yaw)>Float.MAX_VALUE || Math.abs(pitch)>Float.MAX_VALUE)
                throw new IllegalStateException("Camera rotation overflow");
            camera[0]=(float)yaw;
            camera[1]=(float)pitch;

            Object ray=rayField.get(mc);
            if(ray!=null) {
                Object target=rayEntityBinding(ray).get(ray);
                if(target!=null) {
                    TargetFields targetFields=targetBinding(target);
                    xyz[3]=numeric(targetFields.position.x,target);
                    xyz[4]=numeric(targetFields.position.y,target);
                    xyz[5]=numeric(targetFields.position.z,target);
                    flags|=4;
                }
            }
        }
        for(double value:xyz)
            if(!Double.isFinite(value))
                throw new IllegalStateException("Nonfinite derived coordinates");

        // Commit the session identity only after a fully valid frame was sampled.
        if(oldWorld!=world || oldPlayer!=player) {
            generation++;
            oldWorld=world;
            oldPlayer=player;
        }
        return new Frame(System.nanoTime()/1000000L,generation,flags,xyz,camera);
    }

    /** Clear all strong world/player, reflection, and classloader references. */
    public static synchronized void release() {
        mcClass=null;
        getter=null;
        worldField=null;
        playerField=null;
        rayField=null;
        oldWorld=null;
        oldPlayer=null;
        vanillaObfuscated=false;
        generation=1;
        cachedPlayer=null;
        cachedTarget=null;
        cachedRayClass=null;
        cachedRayEntity=null;
        mappingBuilds=0;
    }

    private ObservedState() { }
}
