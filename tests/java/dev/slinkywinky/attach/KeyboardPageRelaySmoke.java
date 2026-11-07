package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.KeyboardQueueOwner;
import dev.slinkywinky.instrument.KeyboardPageRelay;
import org.lwjgl.input.Keyboard;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R118: real Java 8 bridge classes, disposable fake LWJGL2 queue only.
 * It never attaches to Minecraft, invokes native code, or changes a game key.
 */
public final class KeyboardPageRelaySmoke {
    private static void check(boolean condition,String detail) {
        if(!condition)throw new AssertionError(detail);
    }
    private static boolean refused(Runnable run) {
        try {run.run();return false;}
        catch(IllegalStateException good){return true;}
    }
    public static void main(String[] args)throws Exception {
        Keyboard.reset();
        final List<String> pageActions=new ArrayList<String>();
        final KeyboardPageRelay.PageSink sink=new KeyboardPageRelay.PageSink() {
            @Override public void onPage(int direction,long epoch) {
                pageActions.add(direction+":"+epoch);
            }
        };
        boolean denied=false;
        try {new KeyboardPageRelay(0,sink);}
        catch(IllegalArgumentException good) {denied=true;}
        check(denied,"Zero source epoch admitted");
        denied=false;
        try {new KeyboardPageRelay(118,null);}
        catch(IllegalArgumentException good) {denied=true;}
        check(denied,"Missing native preview receiver admitted");

        final KeyboardPageRelay relay=new KeyboardPageRelay(118,sink);
        check(relay.active() && relay.epoch()==118,"Owner not bound");
        // Must have source-matched fake Keyboard and independent fresh focus.
        check(!KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),false),
              "Unaudited source admitted");
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"Mock bind rejected");
        check(!KeyboardQueueOwner.arm(118,false,true,relay),
              "Unattested modal dispatcher admitted");
        check(KeyboardQueueOwner.observeForeground(true),"Focus signal failed");
        check(KeyboardQueueOwner.arm(118,true,true,relay),
              "Synthetic exclusive dispatcher rejected");

        final AtomicBoolean foreignRefused=new AtomicBoolean();
        Thread foreign=new Thread(new Runnable() {
            @Override public void run() {
                foreignRefused.set(refused(new Runnable() {
                    @Override public void run() {relay.onEdge(209,true);}
                }) && !relay.active() && relay.epoch()==0);
            }
        });
        foreign.start();foreign.join();
        check(foreignRefused.get(),"Foreign input owner was accepted");

        Keyboard.enqueue(209,true);
        Keyboard.enqueue(209,true); // repeating down must not repaginate
        Keyboard.enqueue(209,false);
        Keyboard.enqueue(201,true);
        Keyboard.enqueue(201,false);
        Keyboard.enqueue(17,true); // unrelated game key swallowed during modal
        Keyboard.enqueue(17,false);
        Keyboard.enqueue(0,true);  // character-only event is also withheld
        check(!KeyboardQueueOwner.next(),
              "Modal source events forwarded to vanilla keyboard loop");
        check(Keyboard.pending()==0,"Undrained synthetic modal input");
        check(pageActions.size()==2 &&
              "1:118".equals(pageActions.get(0)) &&
              "-1:118".equals(pageActions.get(1)),
              "Page edge/action/epoch delivery mismatch");
        check(!relay.dispose(false,true,true),"Visible GUI released");
        check(!relay.dispose(true,false,true),"Held input released");
        check(!relay.dispose(true,true,false),"Unrestored loop released");

        check(KeyboardQueueOwner.close(true),"Modal close rejected");
        check(!KeyboardQueueOwner.next(),"Draining forwarded a queue event");
        check("VANILLA".equals(KeyboardQueueOwner.state()),
              "Original game queue proxy not restored");
        check(relay.dispose(true,true,true),"Attested GUI relay close failed");
        check(!relay.active(),"Disposed relay remained active");
        check(refused(new Runnable() {
            @Override public void run(){relay.onEdge(209,true);}
        }),"Disposed relay still accepted key input");
        Keyboard.enqueue(30,true);
        check(KeyboardQueueOwner.next() &&
              Keyboard.getEventKey()==30 && Keyboard.getEventKeyState(),
              "Post-modal original LWJGL queue event not preserved");
        check(!KeyboardQueueOwner.next(),"Original queue event duplicated");
        check(pageActions.size()==2,"Relay fired after original restoration");
        check(KeyboardQueueOwner.unbind(true,true,true),"Synthetic unbind failed");

        // Reusing a previous epoch or the old relay is prohibited.
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"Rebind failed");
        check(KeyboardQueueOwner.observeForeground(true),"New focus rejected");
        check(!KeyboardQueueOwner.arm(118,true,true,relay),
              "Repeated epoch rearmed modal");
        final KeyboardPageRelay second=new KeyboardPageRelay(119,sink);
        check(KeyboardQueueOwner.arm(119,true,true,second),
              "New verified epoch rejected");
        Keyboard.enqueue(201,true);
        Keyboard.enqueue(201,false);
        check(!KeyboardQueueOwner.next(),"Second session leaked input");
        check(pageActions.size()==3 &&
              "-1:119".equals(pageActions.get(2)),
              "Second session did not isolate epoch");
        check(KeyboardQueueOwner.close(true),"Second session did not close");
        check(!KeyboardQueueOwner.next(),"Second drain forwarded keys");
        check(second.dispose(true,true,true),"Second relay restoration failed");
        check(KeyboardQueueOwner.unbind(true,true,true),"Second unbind failed");

        System.out.println("R118_MOCK_JAVA8_PAGE_UP_DOWN_TO_FIRST_PARTY_PREVIEW_SINK_PASS");
        System.out.println("R118_EXCLUSIVE_EPOCH_REPEAT_OWNER_CLOSE_AND_VANILLA_RESTORE_PASS");
        System.out.println("R118_NO_LIVE_MINECRAFT_INPUT_HOOK_OR_NATIVE_ACTION");
    }
}
