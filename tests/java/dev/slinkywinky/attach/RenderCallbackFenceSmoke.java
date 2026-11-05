package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RenderCallbackFence;
import dev.slinkywinky.instrument.RenderFrameGate;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/** R79 Java 8 regression: no WGL/JNI read before owner admission or after drain. */
public final class RenderCallbackFenceSmoke {
    private static void check(boolean ok,String what) {
        if(!ok)throw new AssertionError("R79 "+what);
    }
    private static StackTraceElement el(String type,String method) {
        return new StackTraceElement(type,method,"R79Generated.java",1);
    }
    private static final StackTraceElement[] GOOD={
        el("dev.slinkywinky.instrument.RendererFrameDiagnostic","onRender"),
        el("bfk","a"),el("ave","av")
    };
    private static final StackTraceElement[] WRONG={
        el("dev.slinkywinky.instrument.RendererFrameDiagnostic","onRender"),
        el("bfk","a"),el("other","av")
    };

    public static void main(String[] args) throws Exception {
        final AtomicInteger calls=new AtomicInteger();
        RenderFrameGate gate=new RenderFrameGate(8);
        RenderCallbackFence fence=new RenderCallbackFence(gate,new Runnable() {
            @Override public void run(){calls.incrementAndGet();}
        });
        for(int n=1;n<=8;n++) {
            fence.sample(Thread.currentThread(),0.5f,n,GOOD);
            check(calls.get()==n && gate.accepted()==n,
                  "valid sample was not admitted before observer "+n);
        }
        fence.sample(Thread.currentThread(),0.5f,9L,GOOD);
        check(calls.get()==8 && !fence.failed() &&
              gate.await(1,TimeUnit.MILLISECONDS),
              "finite cap allowed a ninth native query");
        fence.stopAndDrain();
        fence.sample(Thread.currentThread(),0.5f,10L,GOOD);
        check(fence.closed() && calls.get()==8,"post-close observation admitted");

        final AtomicInteger wrongCalls=new AtomicInteger();
        RenderFrameGate wrongGate=new RenderFrameGate(8);
        RenderCallbackFence wrong=new RenderCallbackFence(wrongGate,new Runnable() {
            @Override public void run(){wrongCalls.incrementAndGet();}
        });
        wrong.sample(Thread.currentThread(),0.5f,1L,GOOD);
        Thread foreign=new Thread(new Runnable() {
            @Override public void run() {
                wrong.sample(Thread.currentThread(),0.5f,2L,GOOD);
            }
        },"R79-foreign-thread");
        foreign.start();foreign.join(3000L);
        check(!foreign.isAlive() && wrongGate.invalid() &&
              wrongGate.accepted()==1 && wrongCalls.get()==1,
              "foreign Java thread entered native observer before rejection");
        wrong.sample(Thread.currentThread(),0.5f,3L,GOOD);
        check(wrongCalls.get()==1,
              "invalid session allowed more callbacks after owner violation");
        wrong.stopAndDrain();

        final AtomicInteger refusedCalls=new AtomicInteger();
        for(int negative=0;negative<4;negative++) {
            RenderFrameGate rejectGate=new RenderFrameGate(8);
            RenderCallbackFence reject=new RenderCallbackFence(rejectGate,new Runnable(){
                @Override public void run(){refusedCalls.incrementAndGet();}
            });
            switch(negative) {
                case 0: reject.sample(Thread.currentThread(),0.5f,1L,WRONG);break;
                case 1: reject.sample(Thread.currentThread(),Float.NaN,1L,GOOD);break;
                case 2: reject.sample(Thread.currentThread(),0.5f,-1L,GOOD);break;
                default: reject.sample(Thread.currentThread(),1.5f,1L,GOOD);break;
            }
            check(rejectGate.invalid() && rejectGate.accepted()==0,
                  "invalid original render sample admitted");
            reject.stopAndDrain();
        }
        check(refusedCalls.get()==0,
              "native observer ran for invalid callsite, partial ticks or clock");
        System.out.println("R79_ORIGINAL_RENDER_GATE_PRECEDES_NATIVE_WGL_PASS");

        final AtomicInteger failedCalls=new AtomicInteger();
        RenderFrameGate failedGate=new RenderFrameGate(8);
        RenderCallbackFence failed=new RenderCallbackFence(failedGate,new Runnable() {
            @Override public void run() {
                failedCalls.incrementAndGet();
                throw new IllegalStateException("simulated JNI observation failure");
            }
        });
        failed.sample(Thread.currentThread(),0.5f,1L,GOOD);
        failed.sample(Thread.currentThread(),0.5f,2L,GOOD);
        check(failed.failed() && failedCalls.get()==1 &&
              failedGate.await(1,TimeUnit.MILLISECONDS),
              "JNI failure did not permanently stop diagnostic callbacks");
        failed.stopAndDrain();

        // Wake the cleanup waiter while an admitted observer is in flight;
        // closing must wait for its completion rather than sleeping 100 ms.
        final CountDownLatch entered=new CountDownLatch(1);
        final CountDownLatch release=new CountDownLatch(1);
        final AtomicBoolean completed=new AtomicBoolean();
        final RenderFrameGate slowGate=new RenderFrameGate(1);
        final RenderCallbackFence slow=new RenderCallbackFence(slowGate,new Runnable() {
            @Override public void run() {
                entered.countDown();
                try {
                    if(!release.await(4,TimeUnit.SECONDS))
                        throw new IllegalStateException("R79 observer timeout");
                } catch(InterruptedException error) {
                    Thread.currentThread().interrupt();
                    throw new IllegalStateException(error);
                }
            }
        });
        Thread renderer=new Thread(new Runnable() {
            @Override public void run() {
                slow.sample(Thread.currentThread(),0.5f,1L,GOOD);
            }
        },"R79-disposable-render-owner");
        renderer.start();
        check(entered.await(3,TimeUnit.SECONDS),
              "slow observer did not reach native-read boundary");
        check(slowGate.await(1,TimeUnit.SECONDS),
              "completion latch not triggered before observer returns");
        Thread cleanup=new Thread(new Runnable() {
            @Override public void run() {
                slow.stopAndDrain();
                completed.set(true);
            }
        },"R79-cleanup");
        cleanup.start();
        Thread.sleep(65L);
        check(!completed.get(),"cleanup returned before in-flight native read");
        release.countDown();
        renderer.join(3000L);
        cleanup.join(3000L);
        check(!renderer.isAlive() && !cleanup.isAlive() &&
              completed.get() && slow.closed() && !slow.failed(),
              "cleanup did not drain and close owner callback");
        slow.sample(Thread.currentThread(),0.5f,2L,GOOD);
        check(slowGate.accepted()==1,
              "late callback observed after restoration drain");
        System.out.println("R79_NATIVE_READ_DRAIN_BEFORE_RETRANSFORM_PASS");
        System.out.println("R79_WRONG_THREAD_AND_FAILED_JNI_REFUSED_PASS");
        System.out.println("R79_JAVA8_ONLY_NO_MINECRAFT_OR_GRAPHICS");
    }
    private RenderCallbackFenceSmoke(){}
}
