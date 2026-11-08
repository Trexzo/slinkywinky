package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.MouseQueueOwner;
import org.lwjgl.input.Mouse;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/** Disposable Java 8 source-shaped test; no game JVM or transformed ave. */
public final class MouseQueueOwnerSmoke {
    private static void check(boolean yes,String reason) {
        if(!yes)throw new AssertionError(reason);
    }
    private static void focus() {
        check(MouseQueueOwner.observeForeground(true),"No focus heartbeat");
    }
    private static boolean arm(long epoch,MouseQueueOwner.PointerSink sink) {
        return MouseQueueOwner.arm(epoch,true,true,true,676,630,sink);
    }
    private static boolean unbind() {
        return MouseQueueOwner.unbind(true,true,true,true);
    }
    public static void main(String[] args)throws Exception {
        Mouse.reset();
        check(!MouseQueueOwner.bind(String.class,
            String.class.getClassLoader(),true),"Unrelated class admitted");
        check(!MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),false),
            "Unverified source admitted");
        check(!MouseQueueOwner.bind(Mouse.class,null,true),"Missing loader admitted");
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
            "Mock LWJGL class rejected");
        check("VANILLA".equals(MouseQueueOwner.state()),"Wrong default mode");

        Mouse.enqueue(0,true,30,40,0);
        check(MouseQueueOwner.next(),"Vanilla event swallowed");
        check(Mouse.getEventButton()==0 && Mouse.getEventButtonState() &&
              Mouse.getEventX()==30 && Mouse.getEventY()==40,
              "Vanilla event metadata changed");
        check(!MouseQueueOwner.next(),"Vanilla queue not exhausted");
        final List<String> received=new ArrayList<String>();
        MouseQueueOwner.PointerSink sink=new MouseQueueOwner.PointerSink() {
            @Override public void onPointer(int phase,int x,int y) {
                received.add(phase+":"+x+":"+y);
            }
        };
        check(!MouseQueueOwner.arm(1,false,true,true,676,630,sink),
              "Unproven full input ownership admitted");
        check(!MouseQueueOwner.arm(1,true,false,true,676,630,sink),
              "Held physical controls admitted");
        check(!MouseQueueOwner.arm(1,true,true,false,676,630,sink),
              "Hidden GUI admitted");
        check(!MouseQueueOwner.arm(1,true,true,true,0,630,sink),
              "Bad viewport admitted");
        check(!MouseQueueOwner.arm(0,true,true,true,676,630,sink),
              "Zero epoch admitted");
        check(!arm(1,sink),"Missing focus heartbeat admitted");
        focus();
        check(arm(1,sink),"First fully-attested mock modal refused");
        check(!arm(2,sink),"Double arm accepted");
        AtomicBoolean wrongThread=new AtomicBoolean(false);
        Thread alien=new Thread(new Runnable(){
            @Override public void run(){
                wrongThread.set(!MouseQueueOwner.next() &&
                    !MouseQueueOwner.close(true,true) &&
                    "UNAVAILABLE".equals(MouseQueueOwner.state()) &&
                    MouseQueueOwner.epoch()==0L);
            }
        });
        alien.start();alien.join();
        check(wrongThread.get(),"Foreign thread owned mouse queue");
        Mouse.enqueue(0,true,5,629,0); // top-left 5,0
        Mouse.enqueue(0,true,5,629,0); // repeated edge
        Mouse.enqueue(-1,false,20,620,0); // move while held
        Mouse.enqueue(-1,false,20,620,120); // wheel withheld
        Mouse.enqueue(1,true,20,620,0); // right button withheld
        Mouse.enqueue(1,false,20,620,0);
        Mouse.enqueue(0,false,21,600,0);
        check(!MouseQueueOwner.next(),"Modal leaked to vanilla");
        check(Mouse.pending()==0,"Modal source events not consumed");
        check(received.size()==3 &&
            "0:5:0".equals(received.get(0)) &&
            "1:20:9".equals(received.get(1)) &&
            "2:21:29".equals(received.get(2)),
            "LWJGL bottom-left conversion, repeat or wheel filtering wrong");
        check(!MouseQueueOwner.close(false,true),"Unhidden menu close accepted");
        check(!MouseQueueOwner.close(true,false),"Keyboard not quarantined");
        check(MouseQueueOwner.close(true,true),"Modal close refused");
        check("DRAINING".equals(MouseQueueOwner.state()),"Missing drain state");
        check(!MouseQueueOwner.next(),"Draining leaked vanilla event");
        check("VANILLA".equals(MouseQueueOwner.state()),
              "Queue did not restore after release");
        Mouse.enqueue(2,true,40,50,0);
        check(MouseQueueOwner.next() && Mouse.getEventButton()==2,
              "Restored vanilla mouse event missing");
        check(!MouseQueueOwner.next(),"Vanilla double poll");
        check(!arm(1,sink),"Previous epoch replayed");
        check(unbind(),"Verified empty source could not unbind");
        check(!MouseQueueOwner.next(),"Unbound queue guessed original owner");

        // Held left-button release remains quarantined after modal close.
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
              "Rebind failed");
        focus();
        check(arm(2,sink),"Next epoch refused");
        Mouse.enqueue(0,true,33,44,0);
        check(!MouseQueueOwner.next(),"Held button leaked");
        check(MouseQueueOwner.close(true,true),"Held close refused");
        Mouse.enqueue(0,false,33,44,0);
        check(!MouseQueueOwner.next(),"Held release leaked");
        check("VANILLA".equals(MouseQueueOwner.state()),
              "Held-release drain not completed");
        check(unbind(),"Post-release unbind failed");

        // Revoked state never replays GUI events into Minecraft.
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
              "Focus test bind failed");
        focus();
        check(arm(3,sink),"Focus test arm refused");
        check(!MouseQueueOwner.observeForeground(false),"Focus loss accepted");
        check("REFUSED".equals(MouseQueueOwner.state()),"Focus did not revoke");
        check(!MouseQueueOwner.next(),"Revoked mouse source leaked");
        check(!arm(4,sink),"Revoked modal rearmed");
        Mouse.enqueue(0,false,40,50,0);
        check(!unbind(),"Unconsumed release restored vanilla");
        check(Mouse.pending()==0,"Unbind failed to quarantine release");
        check(unbind(),"Independent rollback failed");

        // Bad bounds and unknown button must fail closed.
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
              "Bounds bind failed");
        focus();
        check(arm(4,sink),"Bounds arm refused");
        Mouse.enqueue(0,true,-1,12,0);
        check(!MouseQueueOwner.next(),"Bad bounds leaked");
        check("REFUSED".equals(MouseQueueOwner.state()),"Bad bounds not revoked");
        Mouse.reset();
        check(unbind(),"Bounds clean rollback failed");

        // A sink exception is contained and denies subsequent events.
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
              "Callback bind failed");
        focus();
        check(arm(5,new MouseQueueOwner.PointerSink(){
            @Override public void onPointer(int phase,int x,int y) {
                throw new IllegalStateException("synthetic callback fault");
            }
        }),"Callback arm failed");
        Mouse.enqueue(0,true,3,3,0);
        check(!MouseQueueOwner.next(),"Exception escaped modal");
        check("REFUSED".equals(MouseQueueOwner.state()),"Sink fault not revoked");
        Mouse.reset();
        check(unbind(),"Callback rollback failed");

        // Bound 128-event polls avoid indefinite source monopolization.
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
              "Flood bind failed");
        focus();
        check(arm(6,sink),"Flood arm failed");
        for(int i=0;i<160;i++)Mouse.enqueue(-1,false,15,25,120);
        check(!MouseQueueOwner.next() && Mouse.pending()==32,
              "Unbounded first drain");
        check(!MouseQueueOwner.next() && Mouse.pending()==0,
              "Unbounded second drain");
        check(MouseQueueOwner.close(true,true),"Flood close refused");
        check(!MouseQueueOwner.next() &&
              "VANILLA".equals(MouseQueueOwner.state()),"Flood drain failed");
        check(unbind(),"Flood unbind refused");
        System.out.println("R160_JAVA8_MOUSE_LWJGL_SOURCE_SHAPE_EDGE_COORDS_PASS");
        System.out.println("R160_MODAL_DRAIN_ROLLBACK_FOCUS_AND_OVERFLOW_PASS");
        System.out.println("R160_NO_ORIGINAL_GAME_MOUSE_HOOK_ATTACHED");
    }
}
