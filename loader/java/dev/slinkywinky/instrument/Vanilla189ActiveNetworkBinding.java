package dev.slinkywinky.instrument;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * R368: exact original Minecraft getNetHandler() -> active NetworkManager.
 * Holds ONLY verified class/reflective metadata, never a network manager,
 * handler, channel, world, player, entity or a Packet reference.
 */
public final class Vanilla189ActiveNetworkBinding implements AutoCloseable {
    private Class<?> mcClass,handlerClass,managerClass;
    private Method getNetHandler;
    private Field netManager;
    private volatile boolean closed;

    public Vanilla189ActiveNetworkBinding(Class<?> mc,Class<?> handler,
                                           Class<?> manager)
            throws ReflectiveOperationException{
        if(mc==null||handler==null||manager==null||
           !"ave".equals(mc.getName())||
           !"bcy".equals(handler.getName())||
           !"ek".equals(manager.getName())||
           mc.getClassLoader()!=handler.getClassLoader()||
           mc.getClassLoader()!=manager.getClassLoader())
            throw new IllegalArgumentException("Untrusted original network ownership classes");
        final Method accessor=mc.getDeclaredMethod("u");
        if(accessor.getParameterTypes().length!=0||
           accessor.getReturnType()!=handler||
           Modifier.isStatic(accessor.getModifiers()))
            throw new NoSuchMethodException("Original ave.u()Lbcy; mismatch");
        final Field field=handler.getDeclaredField("c");
        if(field.getType()!=manager||
           Modifier.isStatic(field.getModifiers()))
            throw new NoSuchFieldException("Original bcy.c:Lek; mismatch");
        accessor.setAccessible(true);field.setAccessible(true);
        mcClass=mc;handlerClass=handler;managerClass=manager;
        getNetHandler=accessor;netManager=field;
    }
    /** Caller passes the ephemeral current Minecraft singleton. */
    public Object read(Object mc)throws ReflectiveOperationException{
        if(closed)throw new IllegalStateException("R368 network identity mapping retired");
        if(mc==null||!mcClass.isInstance(mc))
            throw new IllegalArgumentException("Untrusted original Minecraft owner");
        final Object handler=getNetHandler.invoke(mc);
        if(handler==null)return null;
        if(!handlerClass.isInstance(handler))
            throw new IllegalStateException("Wrong original NetHandler value");
        final Object manager=netManager.get(handler);
        if(manager!=null&&manager.getClass()!=managerClass)
            throw new IllegalStateException("Unknown source NetHandler NetworkManager");
        if(closed)throw new IllegalStateException("R368 identity retired mid-read");
        return manager; // transient only; caller MUST use WeakReference
    }
    @Override public void close(){
        closed=true;mcClass=handlerClass=managerClass=null;
        getNetHandler=null;netManager=null;
    }
}
