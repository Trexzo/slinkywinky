package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.MouseQueueOwner;
import dev.slinkywinky.instrument.KeyboardQueueOwner;
import org.lwjgl.input.Mouse;
import org.lwjgl.input.Keyboard;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/** Standalone Java 8 session lifecycle regression; no original game hooks. */
public final class R165ModalInputSessionSmoke {
    private static void check(boolean yes,String msg) {
        if(!yes)throw new AssertionError("R165 "+msg);
    }
    private static final String SHA=R161InputCoverageLedger.PINNED_AVE_SHA256;
    private static final int ALL=R161InputCoverageLedger.ALL;
    private static final int WIDTH=676,HEIGHT=630;
    // Simulates the original method body after R164's separately tested
    // in-memory-only entry cut. It is NOT the original game method.
    private static final class TinyVanillaInput {
        int originalCalls;
        void run() {
            if(R164OfflineModalGate.shouldQuarantineInput())return;
            originalCalls++;
        }
    }
    private static final class Receipt {
        final List<String> mouse=new ArrayList<String>();
        final List<String> keys=new ArrayList<String>();
        final MouseQueueOwner.PointerSink mouseSink=
            new MouseQueueOwner.PointerSink(){
                @Override public void onPointer(int phase,int x,int y){
                    mouse.add(phase+":"+x+":"+y);
                }
            };
        final KeyboardQueueOwner.KeySink keySink=
            new KeyboardQueueOwner.KeySink(){
                @Override public void onEdge(int code,boolean down) {
                    keys.add(code+":"+(down?"1":"0"));
                }
            };
    }
    private static R165ModalInputSession bound() {
        Mouse.reset();Keyboard.reset();
        R165ModalInputSession session=new R165ModalInputSession();
        check(!session.bind(Mouse.class,Keyboard.class,
            Mouse.class.getClassLoader(),false),"Unverified source bound");
        check(session.bind(Mouse.class,Keyboard.class,
            Mouse.class.getClassLoader(),true),"Verified synthetic source refused");
        check(session.phase()==R165ModalInputSession.Phase.PASSIVE,
              "Missing passive bind phase");
        return session;
    }
    private static boolean open(R165ModalInputSession session,long epoch,
                                MouseQueueOwner.PointerSink mouse,
                                KeyboardQueueOwner.KeySink keys) {
        return session.open(SHA,epoch,ALL,true,true,true,true,
                            WIDTH,HEIGHT,mouse,keys);
    }
    private static void testNormalModalNormal()throws Exception {
        final TinyVanillaInput vanilla=new TinyVanillaInput();
        vanilla.run();vanilla.run();
        check(vanilla.originalCalls==2,"Vanilla baseline");
        final Receipt received=new Receipt();
        final R165ModalInputSession session=bound();
        check(!session.open(SHA,1,ALL-1,true,true,true,true,
                            WIDTH,HEIGHT,received.mouseSink,received.keySink),
              "Incomplete source coverage opened");
        check(!session.open("WRONG",1,ALL,true,true,true,true,
                            WIDTH,HEIGHT,received.mouseSink,received.keySink),
              "Wrong original SHA opened");
        check(!session.open(SHA,1,ALL,false,true,true,true,
                            WIDTH,HEIGHT,received.mouseSink,received.keySink),
              "No whole-method gate opened");
        check(!session.open(SHA,1,ALL,true,false,true,true,
                            WIDTH,HEIGHT,received.mouseSink,received.keySink),
              "No foreground ownership opened");
        check(!session.open(SHA,1,ALL,true,true,false,true,
                            WIDTH,HEIGHT,received.mouseSink,received.keySink),
              "Held physical input opened");
        check(!session.open(SHA,1,ALL,true,true,true,false,
                            WIDTH,HEIGHT,received.mouseSink,received.keySink),
              "Hidden GUI opened");
        check(!session.quarantined(),"Invalid open switched gate");
        check(open(session,1,received.mouseSink,received.keySink),
              "Both source owners did not enter modal session");
        check(session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.MODAL,
              "Gate not quarantined before source modal");
        vanilla.run();
        check(vanilla.originalCalls==2,"Vanilla method ran while modal");
        check(!open(session,2,received.mouseSink,received.keySink),
              "Nested modal session was admitted");
        final AtomicBoolean alienRejected=new AtomicBoolean(false);
        Thread foreign=new Thread(new Runnable(){
            @Override public void run(){
                alienRejected.set(!session.poll(true) &&
                    session.phase()==R165ModalInputSession.Phase.LOCKOUT &&
                    session.epoch()==0L);
            }
        });
        foreign.start();foreign.join();
        check(alienRejected.get(),"Foreign thread entered owner session");
        Mouse.enqueue(0,true,25,629,0);
        Mouse.enqueue(-1,false,26,628,0);
        Keyboard.enqueue(17,true);
        check(session.poll(true),"Paired modal polling refused");
        check(received.mouse.size()==2 &&
              "0:25:0".equals(received.mouse.get(0)) &&
              "1:26:1".equals(received.mouse.get(1)),
              "Pointer events lost or wrong coordinate origin");
        check(received.keys.size()==1 &&
              "17:1".equals(received.keys.get(0)),
              "Key-down event missing");
        check(!session.beginClose(false),"Visible GUI closed");
        check(session.beginClose(true),"Paired close denied");
        check(session.phase()==R165ModalInputSession.Phase.DRAINING &&
              session.quarantined(),"Close prematurely released gate");
        check(!session.drain(true,true,true),"Held input prematurely restored");
        vanilla.run();
        check(vanilla.originalCalls==2,"Closing key leaked into game");
        Mouse.enqueue(0,false,27,627,0);
        Keyboard.enqueue(17,false);
        check(!session.drain(true,false,true),
              "Unknown pending queues accepted without empty receipt");
        check(session.quarantined() &&
              "VANILLA".equals(MouseQueueOwner.state()) &&
              "VANILLA".equals(KeyboardQueueOwner.state()),
              "Quarantine not maintained pending independent drain proof");
        check(session.drain(true,true,true),"Both queues not restored");
        check(!session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.PASSIVE,
              "Session did not resume vanilla only after all drains");
        vanilla.run();
        check(vanilla.originalCalls==3,"Vanilla method did not resume");
        Mouse.enqueue(1,true,12,42,0);
        Keyboard.enqueue(30,true);
        check(MouseQueueOwner.next() && Mouse.getEventButton()==1 &&
              KeyboardQueueOwner.next() && Keyboard.getEventKey()==30,
              "Vanilla queue metadata not preserved");
        check(!MouseQueueOwner.next() && !KeyboardQueueOwner.next(),
              "Vanilla event replay");
        check(!session.restore(false,true,true,true),
              "Unverified original-byte restore admitted");
        check(session.restore(true,true,true,true),"Source owners not unbound");
        check(!session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.UNBOUND,
              "Unexpected foreign/unbound phase");
        System.out.println("R165_NORMAL_MODAL_NORMAL_TWO_QUEUES_AND_GATE_PASS");
    }
    private static void testPartialArmRecovery() {
        Receipt sink=new Receipt();
        R165ModalInputSession session=bound();
        check(!open(session,2,sink.mouseSink,null),
              "Partial keyboard admission was accepted");
        check(!session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.PASSIVE &&
              "VANILLA".equals(MouseQueueOwner.state()) &&
              "VANILLA".equals(KeyboardQueueOwner.state()),
              "Mouse did not compensate partial keyboard arm");
        check(!open(session,2,sink.mouseSink,sink.keySink),
              "Reused source epoch despite partial arm was accepted");
        check(open(session,3,sink.mouseSink,sink.keySink),
              "Higher source epoch could not recover");
        check(session.beginClose(true),"Close after partial arm");
        check(session.drain(true,true,true),"Drain after partial arm");
        check(session.restore(true,true,true,true),"Unbind after partial arm");
        System.out.println("R165_PARTIAL_KEYBOARD_ARM_COMPENSATED_PASS");
    }
    private static void testFocusFaultRollback() {
        Receipt sink=new Receipt();
        R165ModalInputSession session=bound();
        check(open(session,4,sink.mouseSink,sink.keySink),"Focus session open");
        Mouse.enqueue(0,true,44,600,0);
        Keyboard.enqueue(17,true);
        check(session.poll(true),"Initial modal packet");
        check(!session.poll(false),"Focus loss did not revoke");
        check(session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.LOCKOUT &&
              "REFUSED".equals(MouseQueueOwner.state()) &&
              "REFUSED".equals(KeyboardQueueOwner.state()),
              "Both input owners not fail-closed on lost focus");
        check(!session.beginClose(true),"Refused session closed as normal");
        check(!session.restore(true,false,true,true),
              "Missing keyboard original restoration accepted");
        Mouse.enqueue(0,false,44,600,0);
        Keyboard.enqueue(17,false);
        check(!session.restore(true,true,true,true),
              "Stale GUI release events passed to vanilla");
        check(session.quarantined(),"Gate reopened despite pending release");
        check(Mouse.pending()==0 && Keyboard.pending()==0,
              "Uncertain releases not consumed");
        check(session.restore(true,true,true,true),
              "Independently verified source recovery failed");
        check(!session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.UNBOUND,
              "Source lockout was not cleared after recovery");
        System.out.println("R165_FOCUS_LOSS_STALE_RELEASE_QUARANTINE_PASS");
    }
    private static void testCallbackFault() {
        Receipt sink=new Receipt();
        R165ModalInputSession session=bound();
        MouseQueueOwner.PointerSink fault=new MouseQueueOwner.PointerSink(){
            @Override public void onPointer(int phase,int x,int y){
                throw new IllegalStateException("Synthetic pointer callback fault");
            }
        };
        check(open(session,5,fault,sink.keySink),"Fault test open");
        Mouse.enqueue(0,true,8,8,0);
        check(!session.poll(true),"Failed GUI callback was accepted");
        check(session.quarantined() &&
              session.phase()==R165ModalInputSession.Phase.LOCKOUT &&
              "REFUSED".equals(MouseQueueOwner.state()),
              "Sink fault did not lock out game input");
        Mouse.reset();Keyboard.reset();
        check(session.restore(true,true,true,true),
              "Quarantined callback fault not recoverable");
        check(!session.quarantined(),"Callback fault did not clear on restore");
        System.out.println("R165_CALLBACK_EXCEPTION_INPUT_LOCKOUT_PASS");
    }
    public static void main(String[] args)throws Exception {
        check(args.length==0,"No original-game binary or process expected");
        testNormalModalNormal();
        testPartialArmRecovery();
        testFocusFaultRollback();
        testCallbackFault();
        System.out.println("R165_ONLY_SYNTHETIC_JAVA8_INPUT_OWNERSHIP_FIXTURES");
        System.out.println("R165_NO_ORIGINAL_CLIENT_INPUT_HOOK");
    }
}
