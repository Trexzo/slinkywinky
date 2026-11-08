package org.lwjgl.input;
import java.util.ArrayDeque;

/** Disposable LWJGL2 Mouse API fixture; never packaged with the agent. */
public final class Mouse {
    private static final class Event {
        final int button,x,y,wheel;final boolean down;
        Event(int b,boolean d,int x0,int y0,int w) {
            button=b;down=d;x=x0;y=y0;wheel=w;
        }
    }
    private static final ArrayDeque<Event> queue=new ArrayDeque<Event>();
    private static Event event;
    private static boolean broken;
    public static synchronized void reset() {
        queue.clear();event=null;broken=false;
    }
    public static synchronized void enqueue(
            int button,boolean down,int x,int y,int wheel) {
        queue.addLast(new Event(button,down,x,y,wheel));
    }
    public static synchronized boolean next() {
        if(broken)throw new IllegalStateException("mock mouse source broken");
        event=queue.pollFirst();
        return event!=null;
    }
    private static Event current() {
        if(event==null)throw new IllegalStateException("No mouse event");
        return event;
    }
    public static synchronized int getEventButton(){return current().button;}
    public static synchronized boolean getEventButtonState(){return current().down;}
    public static synchronized int getEventX(){return current().x;}
    public static synchronized int getEventY(){return current().y;}
    public static synchronized int getEventDWheel(){return current().wheel;}
    public static synchronized int pending(){return queue.size();}
    public static synchronized void breakSource(boolean b){broken=b;}
    private Mouse(){}
}
