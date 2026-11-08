package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.MouseQueueOwner;
import dev.slinkywinky.instrument.KeyboardQueueOwner;
import org.lwjgl.input.Mouse;
import org.lwjgl.input.Keyboard;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/** R161 Java 8 paired-LWJGL test. Never runs within a Minecraft process. */
public final class R161PairedInputRollbackSmoke {
    private static void check(boolean ok,String msg) {
        if(!ok)throw new AssertionError("R161 "+msg);
    }
    private static void bindBoth() {
        Mouse.reset(); Keyboard.reset();
        check(MouseQueueOwner.bind(Mouse.class,Mouse.class.getClassLoader(),true),
              "Mouse source-bound fixture rejected");
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"Keyboard source-bound fixture rejected");
        check("VANILLA".equals(MouseQueueOwner.state()) &&
              "VANILLA".equals(KeyboardQueueOwner.state()),"Both queues not passive");
    }
    private static void focusBoth() {
        check(MouseQueueOwner.observeForeground(true),"Mouse focus");
        check(KeyboardQueueOwner.observeForeground(true),"Keyboard focus");
    }
    private static boolean armMouse(long epoch,MouseQueueOwner.PointerSink mouse) {
        return MouseQueueOwner.arm(epoch,true,true,true,676,630,mouse);
    }
    private static boolean armKeyboard(long epoch,KeyboardQueueOwner.KeySink keys) {
        return KeyboardQueueOwner.arm(epoch,true,true,keys);
    }
    private static void restoreBoth() {
        check("VANILLA".equals(MouseQueueOwner.state()) &&
              "VANILLA".equals(KeyboardQueueOwner.state()),"Incomplete modal quarantine");
        check(!MouseQueueOwner.unbind(false,true,true,true),
              "Missing bytecode restore reopened mouse");
        check(!MouseQueueOwner.unbind(true,true,true,false),
              "Unrestored keyboard reopened mouse");
        check(!KeyboardQueueOwner.unbind(false,true,true),
              "Missing bytecode restore reopened keyboard");
        check(MouseQueueOwner.unbind(true,true,true,true),"Mouse restore after empty queue");
        check(KeyboardQueueOwner.unbind(true,true,true),"Keyboard restore after empty queue");
        check("UNAVAILABLE".equals(MouseQueueOwner.state()) &&
              "UNAVAILABLE".equals(KeyboardQueueOwner.state()),"Original owners still bound");
    }

    public static void main(String[] args) throws Exception {
        final R161InputCoverageLedger ledger=R161InputCoverageLedger.fixtureOnly();
        final int all=R161InputCoverageLedger.ALL;
        final String pin=R161InputCoverageLedger.PINNED_AVE_SHA256;
        final List<String> pointer=new ArrayList<String>();
        final List<String> keys=new ArrayList<String>();
        final MouseQueueOwner.PointerSink pointerSink=new MouseQueueOwner.PointerSink() {
            @Override public void onPointer(int phase,int x,int y) {
                pointer.add(phase+":"+x+":"+y);
            }
        };
        final KeyboardQueueOwner.KeySink keySink=new KeyboardQueueOwner.KeySink() {
            @Override public void onEdge(int key,boolean down) {
                keys.add(key+":"+(down?"1":"0"));
            }
        };
        for(int bit=0;bit<16;bit++)
            check(!ledger.tryOpen(pin,1,all & ~(1<<bit),true,true),
                  "Uncovered original input/restore route admitted: "+bit);
        check(!ledger.tryOpen("WRONG ORIGINAL",1,all,true,true),"Unpinned original");
        check(!ledger.tryOpen(pin,0,all,true,true),"Invalid epoch");
        check(!ledger.tryOpen(pin,1,all,false,true),"Foreign game thread");
        check(!ledger.tryOpen(pin,1,all,true,false),"Missing paired queue owners");

        bindBoth();
        focusBoth();
        check(armMouse(1,pointerSink),"Mouse modal denied");
        check(armKeyboard(1,keySink),"Keyboard modal denied");
        check(ledger.tryOpen(pin,1,all,true,true),"Paired fixture ledger failed");
        check(!ledger.tryOpen(pin,2,all,true,true),"Duplicate modal accepted");

        Mouse.enqueue(0,true,44,600,0);
        Mouse.enqueue(-1,false,48,598,0);
        Mouse.enqueue(0,false,48,598,0);
        Keyboard.enqueue(17,true);
        Keyboard.enqueue(17,true);
        Keyboard.enqueue(17,false);
        check(!MouseQueueOwner.next() && !KeyboardQueueOwner.next(),
              "Source queue leaked to vanilla while modal");
        check(pointer.size()==3 &&
              "0:44:29".equals(pointer.get(0)) &&
              "1:48:31".equals(pointer.get(1)) &&
              "2:48:31".equals(pointer.get(2)),
              "Paired mouse edges/coordinate conversion");
        check(keys.size()==2 && "17:1".equals(keys.get(0)) &&
              "17:0".equals(keys.get(1)),
              "Paired keyboard edges/repeats");
        check(!ledger.tryClose(all,true,false,true),
              "One queue restore admitted paired close");
        check(!ledger.tryClose(all,true,true,false),
              "Stale queued key admitted paired close");
        check(!ledger.tryClose(all ^ R161InputCoverageLedger.ROLLBACK_BYTES,
                               true,true,true),
              "Missing original-byte restoration admitted close");

        // Both original queues must drain BEFORE passing through vanilla.
        check(MouseQueueOwner.close(true,true),"Mouse close");
        check(KeyboardQueueOwner.close(true),"Keyboard close");
        check(!MouseQueueOwner.next() && !KeyboardQueueOwner.next(),
              "Modal drain leaked event");
        check("VANILLA".equals(MouseQueueOwner.state()) &&
              "VANILLA".equals(KeyboardQueueOwner.state()),
              "Independent queues not drained");
        check(ledger.tryClose(all,true,true,true),"Paired close failed");
        check(!ledger.isOpen(),"Ledger leaked modal session");
        Mouse.enqueue(2,true,11,22,0);
        Keyboard.enqueue(31,true);
        check(MouseQueueOwner.next() && Mouse.getEventButton()==2 &&
              KeyboardQueueOwner.next() && Keyboard.getEventKey()==31,
              "Vanilla pass-through was not restored with original metadata");
        check(!MouseQueueOwner.next() && !KeyboardQueueOwner.next(),
              "Vanilla metadata replay");
        restoreBoth();

        // Partial arm must never count as complete ownership. Roll back
        // the already-armed keyboard owner without losing physical releases.
        bindBoth();
        focusBoth();
        check(armKeyboard(2,keySink),"Keyboard partial modal");
        check(!MouseQueueOwner.arm(2,false,true,true,676,630,pointerSink),
              "Unauthenticated Mouse owner admitted");
        check(!ledger.tryOpen(pin,2,all,true,false),
              "Partial mouse/keyboard owner admitted");
        check(KeyboardQueueOwner.close(true),"Partial keyboard quarantine");
        check(!KeyboardQueueOwner.next(),"Partial keyboard close leaked");
        check("VANILLA".equals(KeyboardQueueOwner.state()),"Partial keyboard drain");
        restoreBoth();
        check(ledger.epoch()==1,"Failed partial arm consumed ledger epoch");

        // If one focus receipt fails, both queues must be revoked and
        // separately emptied before any claimed bytecode restoration.
        bindBoth();
        focusBoth();
        check(armMouse(3,pointerSink) && armKeyboard(3,keySink),
              "Paired focus-fault setup");
        check(ledger.tryOpen(pin,3,all,true,true),"New epoch");
        check(!MouseQueueOwner.observeForeground(false),"Mouse focus falsely healthy");
        check(!KeyboardQueueOwner.observeForeground(false),"Keyboard focus falsely healthy");
        check("REFUSED".equals(MouseQueueOwner.state()) &&
              "REFUSED".equals(KeyboardQueueOwner.state()),
              "Both queues not revoked after foreground loss");
        Mouse.enqueue(0,false,44,600,0);
        Keyboard.enqueue(17,false);
        check(!MouseQueueOwner.unbind(true,true,true,true),
              "Stale mouse event passed into vanilla");
        check(!KeyboardQueueOwner.unbind(true,true,true),
              "Stale key event passed into vanilla");
        check(Mouse.pending()==0 && Keyboard.pending()==0,
              "Pending input not quarantined");
        check(!ledger.tryClose(all,true,true,false),
              "Stale event allowed paired return");
        check(MouseQueueOwner.unbind(true,true,true,true) &&
              KeyboardQueueOwner.unbind(true,true,true),
              "Revoked source rollback refused");
        check(ledger.tryClose(all,true,true,true),"Paired revoked close");
        check(!ledger.tryOpen(pin,3,all,true,true),"Replayed session epoch");

        // Same Java owner thread MUST control both queue models.
        bindBoth();
        final AtomicBoolean foreignRejected=new AtomicBoolean();
        Thread outsider=new Thread(new Runnable() {
            @Override public void run() {
                foreignRejected.set(!MouseQueueOwner.observeForeground(true) &&
                    !KeyboardQueueOwner.observeForeground(true) &&
                    !MouseQueueOwner.next() && !KeyboardQueueOwner.next() &&
                    "UNAVAILABLE".equals(MouseQueueOwner.state()) &&
                    "UNAVAILABLE".equals(KeyboardQueueOwner.state()));
            }
        });
        outsider.start();outsider.join();
        check(foreignRejected.get(),"Foreign thread acquired either queue");
        restoreBoth();
        System.out.println("R161_SOURCE_ROUTE_COVERAGE_ALL_16_NEGATIVE_CONTROLS_PASS");
        System.out.println("R161_JAVA8_PAIRED_MOUSE_KEYBOARD_DRAIN_AND_RESTORE_PASS");
        System.out.println("R161_NO_ORIGINAL_MINECRAFT_RUNTIME_INPUT_HOOK");
    }
}
