package org.lwjgl.input;

import java.util.ArrayDeque;

/** R110 disposable LWJGL2 API-shape fixture; never included in agent JAR. */
public final class Keyboard {
    private static final class Event {
        final int code;final boolean down;
        Event(int code,boolean down){this.code=code;this.down=down;}
    }
    private static final ArrayDeque<Event> queue=new ArrayDeque<Event>();
    private static Event current;
    private static boolean fail;
    public static synchronized void reset() {
        queue.clear();current=null;fail=false;
    }
    public static synchronized void enqueue(int key,boolean down) {
        queue.addLast(new Event(key,down));
    }
    public static synchronized void failNext(boolean value) {fail=value;}
    public static synchronized boolean next() {
        if(fail)throw new IllegalStateException("Simulated LWJGL queue error");
        current=queue.pollFirst();
        return current!=null;
    }
    public static synchronized int getEventKey() {
        if(current==null)throw new IllegalStateException("No key event");
        return current.code;
    }
    public static synchronized boolean getEventKeyState() {
        if(current==null)throw new IllegalStateException("No key event");
        return current.down;
    }
    public static synchronized int pending() {return queue.size();}
    private Keyboard(){}
}
