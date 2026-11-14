package dev.slinkywinky.forgegui;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * R213: optional GUI->real PR2 owner-thread JNI module-control adapter.
 * No PR2 implementation lives in this branch. Without separately supplied
 * source/JNI this adapter is unavailable, NOT a mock control acknowledgement.
 */
public final class R213NativeSprintBridge implements AutoCloseable {
    public static final class Ack {
        public final long id, generation;
        public final boolean requested, active, settingsOverride;
        public final String module;
        private Ack(long id,String module,boolean requested,
                    boolean active,boolean settingsOverride,long generation) {
            this.id=id;this.module=module;this.requested=requested;
            this.active=active;this.settingsOverride=settingsOverride;
            this.generation=generation;
        }
    }
    private final Thread owner=Thread.currentThread();
    private final Object source;
    private final Method enable,state,close;
    private boolean closed;
    private long lastId;
    private Ack acknowledged;

    public R213NativeSprintBridge() {
        if(!Boolean.getBoolean("slinkywinky.r213NativeSprintBridge"))
            throw new IllegalStateException("Native module bridge not opted in");
        try {
            Class<?> clazz=Class.forName("dev.slinkywinky.SprintObservationProbe");
            source=clazz.newInstance(); // real PR2 source, real JNI handle
            enable=clazz.getMethod("setEnabled",boolean.class);
            state=clazz.getMethod("state");
            close=clazz.getMethod("close");
            // Default PR2 read-only Sprint is requested ON. Explicitly
            // establish a known OFF baseline before any GUI click.
            enable.invoke(source,false);
            acknowledged=read(0);
            if(acknowledged.requested || acknowledged.active)
                throw new IllegalStateException("PR2 Sprint initial off denied");
        } catch(ReflectiveOperationException failure) {
            throw new IllegalStateException(
                "Real PR2 Sprint Java/JNI bridge unavailable",failure);
        }
    }
    private void checkOwner() {
        if(closed || Thread.currentThread()!=owner)
            throw new IllegalStateException("Invalid native module owner");
    }
    private static Object field(Object data,String name)
            throws ReflectiveOperationException {
        Field found=data.getClass().getField(name);
        return found.get(data);
    }
    private Ack read(long id) {
        try {
            Object data=state.invoke(source);
            String name=(String)field(data,"moduleName");
            boolean requested=(Boolean)field(data,"requestedEnabled");
            boolean active=(Boolean)field(data,"activeInWorld");
            boolean settings=(Boolean)field(data,"settingsOverride");
            long generation=(Long)field(data,"worldGeneration");
            if(!"Sprint".equals(name) || generation<0 ||
               active) // This offline title-screen test has NO game world.
                throw new IllegalStateException("Invalid native no-world acknowledgement");
            return new Ack(id,name,requested,active,settings,generation);
        } catch(InvocationTargetException failure) {
            throw new IllegalStateException(
                "Native module manager rejected state",failure.getCause());
        } catch(ReflectiveOperationException failure) {
            throw new IllegalStateException(
                "Malformed PR2 native acknowledgement",failure);
        }
    }
    public synchronized Ack request(long id,String canonical,boolean target) {
        checkOwner();
        if(!"Sprint".equals(canonical) || id<=0 || id!=lastId+1 ||
           acknowledged.requested==target)
            throw new IllegalArgumentException("Invalid GUI module request");
        try {
            enable.invoke(source,target);
        } catch(InvocationTargetException failure) {
            throw new IllegalStateException(
                "Native owner request failed",failure.getCause());
        } catch(ReflectiveOperationException failure) {
            throw new IllegalStateException("Native owner ABI invalid",failure);
        }
        Ack next=read(id);
        if(next.requested!=target ||
           next.generation<acknowledged.generation)
            throw new IllegalStateException("Native owner mismatched request");
        lastId=id;
        acknowledged=next;
        return next;
    }
    public synchronized Ack snapshot() {
        checkOwner();
        Ack next=read(lastId);
        if(next.requested!=acknowledged.requested ||
           next.generation<acknowledged.generation)
            throw new IllegalStateException("Native ack drift");
        acknowledged=next;
        return next;
    }
    @Override public synchronized void close() {
        checkOwner();
        try {close.invoke(source);}
        catch(ReflectiveOperationException failure) {
            throw new IllegalStateException("Native module close failure",failure);
        } finally {closed=true;}
    }
}
