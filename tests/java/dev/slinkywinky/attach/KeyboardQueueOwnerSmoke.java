package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.KeyboardQueueOwner;
import org.lwjgl.input.Keyboard;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/** R110 fixture-only Java 8 LWJGL2 queue transfer, never real Minecraft. */
public final class KeyboardQueueOwnerSmoke {
    private static void check(boolean good,String detail) {
        if(!good)throw new AssertionError(detail);
    }
    public static void main(String[] args)throws Exception {
        Keyboard.reset();
        check(!KeyboardQueueOwner.bind(String.class,
            String.class.getClassLoader(),true),"Unrelated Java type admitted");
        check(!KeyboardQueueOwner.bind(Keyboard.class,
            Keyboard.class.getClassLoader(),false),"Unaudited source admitted");
        check(!KeyboardQueueOwner.bind(Keyboard.class,null,true),
            "Missing original classloader admitted");
        check(KeyboardQueueOwner.bind(Keyboard.class,
            Keyboard.class.getClassLoader(),true),"Exact local LWJGL mock rejected");
        check("VANILLA".equals(KeyboardQueueOwner.state()),"Not pass-through initially");

        // Unarmed and dormant: the exact next() event/metadata is preserved.
        Keyboard.enqueue(17,true);
        check(KeyboardQueueOwner.next(),"Unarmed keyboard queue event lost");
        check(Keyboard.getEventKey()==17 && Keyboard.getEventKeyState(),
              "Original LWJGL metadata not preserved");
        check(!KeyboardQueueOwner.next(),"Unarmed queue not exhausted");

        final List<String> edges=new ArrayList<String>();
        KeyboardQueueOwner.KeySink sink=new KeyboardQueueOwner.KeySink(){
            @Override public void onEdge(int key,boolean down) {
                edges.add(key+":"+(down?1:0));
            }
        };
        check(!KeyboardQueueOwner.arm(1,false,true,sink),
              "Modal without exclusive dispatcher admitted");
        check(!KeyboardQueueOwner.arm(1,true,false,sink),
              "Modal with held physical controls admitted");
        check(!KeyboardQueueOwner.arm(0,true,true,sink),"Zero epoch admitted");
        check(!KeyboardQueueOwner.arm(1,true,true,sink),
              "Modal without foreground heartbeat admitted");
        check(KeyboardQueueOwner.observeForeground(true),"Fresh focus rejected");
        check(KeyboardQueueOwner.arm(1,true,true,sink),
              "Source-attested synthetic modal not armed");
        check(!KeyboardQueueOwner.arm(2,true,true,sink),"Double modal admitted");
        final AtomicBoolean wrongThreadRejected=new AtomicBoolean();
        Thread alien=new Thread(new Runnable(){
            @Override public void run(){
                wrongThreadRejected.set(!KeyboardQueueOwner.next() &&
                    !KeyboardQueueOwner.close(true) &&
                    "UNAVAILABLE".equals(KeyboardQueueOwner.state()) &&
                    KeyboardQueueOwner.epoch()==0);
            }
        });
        alien.start();alien.join();
        check(wrongThreadRejected.get(),"Cross-thread captured event ownership");

        Keyboard.enqueue(17,true);
        Keyboard.enqueue(17,true); // repeat: must not double navigate.
        Keyboard.enqueue(17,false);
        Keyboard.enqueue(0,true); // character-only event, no preview support.
        Keyboard.enqueue(200,true);
        Keyboard.enqueue(200,false);
        Keyboard.enqueue(1,true); // close key stays owned through release.
        check(!KeyboardQueueOwner.next(),"Modal delivered input to vanilla");
        check(Keyboard.pending()==0,"Modal did not consume pending keys");
        check(edges.size()==5 &&
              edges.get(0).equals("17:1") &&
              edges.get(1).equals("17:0") &&
              edges.get(2).equals("200:1") &&
              edges.get(3).equals("200:0") &&
              edges.get(4).equals("1:1"),
              "Modal keydown/repeat/keyup routing inaccurate");
        check(!KeyboardQueueOwner.close(false),"Unhidden overlay admitted close");
        check(KeyboardQueueOwner.close(true),"Verified overlay close refused");
        check("DRAINING".equals(KeyboardQueueOwner.state()),"Missing release quarantine");
        Keyboard.enqueue(1,false);
        check(!KeyboardQueueOwner.next(),"Close release leaked to vanilla");
        check("VANILLA".equals(KeyboardQueueOwner.state()),
              "Original pass-through not restored after release drain");
        Keyboard.enqueue(31,true);
        check(KeyboardQueueOwner.next() && Keyboard.getEventKey()==31,
              "Post-modal original input not resumed");
        check(!KeyboardQueueOwner.next(),"Post-modal event repeated");
        check(!KeyboardQueueOwner.arm(1,true,true,sink),"Replayed epoch accepted");

        // Loss of independently attested foreground must revoke input mode.
        check(KeyboardQueueOwner.observeForeground(true),"Second focus refused");
        check(KeyboardQueueOwner.arm(2,true,true,sink),"Second modal epoch refused");
        Keyboard.enqueue(99,true);
        check(!KeyboardQueueOwner.next(),"Second modal leaked keyboard input");
        check(!KeyboardQueueOwner.observeForeground(false),"Focus loss accepted");
        check("REFUSED".equals(KeyboardQueueOwner.state()),"Focus loss did not revoke");
        check(!KeyboardQueueOwner.arm(3,true,true,sink),"Revoked owner rearmed");
        Keyboard.enqueue(99,false);
        check(!KeyboardQueueOwner.next(),"Revoked owner forwarded key release");
        check(!KeyboardQueueOwner.unbind(false,true,true),
              "Restore without original bytecode attestation admitted");
        check(!KeyboardQueueOwner.unbind(true,false,true),
              "Restore with held physical input admitted");
        check(!KeyboardQueueOwner.unbind(true,true,false),
              "Restore while GUI visible admitted");
        check(!KeyboardQueueOwner.unbind(true,true,true),
              "Queued GUI key release reopened vanilla input");
        check(Keyboard.pending()==0,"Pending key was not quarantined");
        check(KeyboardQueueOwner.unbind(true,true,true),
              "Attested restoration refused after empty queue");
        check("UNAVAILABLE".equals(KeyboardQueueOwner.state()),
              "Unbound owner still published game events");
        check(!KeyboardQueueOwner.next(),"Unbound owner guessed source");

        // Session IDs remain monotonic even after a verified full release.
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"Rebind failed");
        check(KeyboardQueueOwner.observeForeground(true),"New focus refused");
        check(!KeyboardQueueOwner.arm(2,true,true,sink),"Old epoch replay after rebind");
        check(KeyboardQueueOwner.arm(3,true,true,sink),"Fresh epoch refused");
        Keyboard.failNext(true);
        check(!KeyboardQueueOwner.next(),"Broken source leaked to game");
        check("REFUSED".equals(KeyboardQueueOwner.state()),
              "Broken source did not revoke modal delivery");
        Keyboard.failNext(false);
        Keyboard.reset();
        check(KeyboardQueueOwner.unbind(true,true,true),"Source failure cleanup failed");

        // Callback exceptions must not propagate to vanilla tick code.
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"Callback-failure rebind failed");
        check(KeyboardQueueOwner.observeForeground(true),"Callback-failure focus refused");
        check(KeyboardQueueOwner.arm(4,true,true,new KeyboardQueueOwner.KeySink(){
            @Override public void onEdge(int code,boolean down){
                throw new IllegalStateException("Injected test-only sink fault");
            }
        }),"Callback-failure modal refused");
        Keyboard.enqueue(17,true);
        check(!KeyboardQueueOwner.next(),"Faulting sink escaped to game");
        check("REFUSED".equals(KeyboardQueueOwner.state()),
              "Callback exception failed to revoke input");
        Keyboard.reset();
        check(KeyboardQueueOwner.unbind(true,true,true),"Faulting sink cleanup failed");

        // A busy LWJGL source cannot monopolize a game tick. At most 128
        // queued events are swallowed per owner.next() invocation.
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"Flood test bind failed");
        check(KeyboardQueueOwner.observeForeground(true),"Flood focus refused");
        check(KeyboardQueueOwner.arm(5,true,true,sink),"Flood modal refused");
        for(int i=0;i<160;i++)Keyboard.enqueue(19,(i%2)==0);
        check(!KeyboardQueueOwner.next() && Keyboard.pending()==32,
              "Unbounded event drain or event leak");
        check(!KeyboardQueueOwner.next() && Keyboard.pending()==0,
              "Remaining queued events not swallowed");
        check(KeyboardQueueOwner.close(true),"Flood session close refused");
        check(!KeyboardQueueOwner.next() &&
              "VANILLA".equals(KeyboardQueueOwner.state()),
              "Bounded flood failed safe session close");
        check(KeyboardQueueOwner.unbind(true,true,true),"Flood cleanup failed");

        System.out.println("R110_MOCK_LWJGL_VANILLA_PASSTHROUGH_AND_MODAL_QUEUE_QUARANTINE_PASS");
        System.out.println("R110_KEY_DOWN_UP_REPEATS_CLOSE_DRAIN_AND_RESTORE_GUARDS_PASS");
        System.out.println("R110_NO_ORIGINAL_MINECRAFT_INPUT_HOOK_OR_PHYSICAL_TEST");
    }
}
