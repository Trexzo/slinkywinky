package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RenderFrameGate;
import dev.slinkywinky.instrument.RendererFrameDiagnostic;
import dev.slinkywinky.instrument.RendererWglEvidence;
import java.util.concurrent.TimeUnit;

/** R75 pure Java 8 negative controls; no Minecraft, Attach, JNI or graphics. */
public final class RenderFrameGateSmoke {
    private static void check(boolean ok,String why) {
        if(!ok)throw new AssertionError(why);
    }
    private static StackTraceElement el(String cls,String method) {
        return new StackTraceElement(cls,method,"GeneratedR75.java",1);
    }
    private static final StackTraceElement[] GOOD={
        el("java.lang.Thread","getStackTrace"),
        el("dev.slinkywinky.instrument.RendererFrameDiagnostic","onRender"),
        el("bfk","a"),el("ave","av"),el("org.example.Runner","main")
    };
    private static final StackTraceElement[] BAD={
        el("dev.slinkywinky.instrument.RendererFrameDiagnostic","onRender"),
        el("bfk","a"),el("not_original","av")
    };

    private static void sample(RenderFrameGate gate,int n) {
        for(int i=0;i<n;i++)gate.sample(Thread.currentThread(),0.5f,i+1L,GOOD);
    }
    public static void main(String[] args) throws Exception {
        check(RenderFrameGate.hasOriginalCallsite(GOOD),"Correct obfuscated caller missing");
        check(!RenderFrameGate.hasOriginalCallsite(BAD),"Unknown caller admitted");
        check(!RenderFrameGate.hasOriginalCallsite(null),"Null stack admitted");
        for(int invalid:new int[]{-1,0,9,999}) {
            boolean denied=false;
            try{new RenderFrameGate(invalid);}
            catch(IllegalArgumentException expected){denied=true;}
            check(denied,"Invalid frame cap admitted");
        }
        // R94 user-controlled session: the ordinary 8-frame and R92 72
        // gates cannot be silently expanded to the manual 2048-frame cap.
        for(int cap:new int[]{8,72,2047,2049,65535}) {
            boolean rejected=false;
            try{new RenderFrameGate(cap,false,true);}
            catch(IllegalArgumentException expected){rejected=true;}
            check(rejected,"R94 incorrect cap accepted");
        }
        RenderFrameGate manual=new RenderFrameGate(2048,false,true);
        sample(manual,23);
        check(manual.accepted()==23 && !manual.invalid(),
              "R94 prior-to-stop frames incorrect");
        manual.stop();
        sample(manual,100);
        check(manual.accepted()==23 && !manual.invalid() &&
              manual.await(1,TimeUnit.MILLISECONDS),
              "R94 operator-stop did not drain finite gate");
        RenderFrameGate manualTimeout=new RenderFrameGate(2048,false,true);
        sample(manualTimeout,2100);
        check(manualTimeout.accepted()==2048 && !manualTimeout.invalid() &&
              manualTimeout.await(1,TimeUnit.MILLISECONDS),
              "R94 finite callback cap unbounded");
        RenderFrameGate wrongManual=new RenderFrameGate(2048,false,true);
        wrongManual.sample(Thread.currentThread(),0.25f,1L,BAD);
        check(wrongManual.invalid() && wrongManual.accepted()==0,
              "R94 invalid game stack passed");
        System.out.println("R94_MANUAL_2048_CAP_OPERATOR_STOP_AND_OWNER_GATE_PASS");
        // R92: only an explicitly marked 72-frame epoch may exceed R75's
        // default eight callbacks. No Minecraft JVM or JNI is involved.
        for(int cap:new int[]{8,64,73,999}) {
            boolean denied=false;
            try{new RenderFrameGate(cap,true);}
            catch(IllegalArgumentException expected){denied=true;}
            check(denied,"R92 invalid finite cap admitted");
        }
        RenderFrameGate multi=new RenderFrameGate(72,true);
        sample(multi,100);
        check(multi.accepted()==72 && !multi.invalid() &&
              multi.await(1,TimeUnit.MILLISECONDS),
              "R92 exact 72-frame finite cap failed");
        RenderFrameGate multiWrong=new RenderFrameGate(72,true);
        multiWrong.sample(Thread.currentThread(),0.2f,1L,BAD);
        check(multiWrong.invalid() && multiWrong.accepted()==0,
              "R92 wrong original callsite admitted");
        System.out.println("R92_ONLY_EXPLICIT_72_FRAME_OWNER_GATE_PASS");
        int visible=0,hidden=0;
        for(int sampleIndex=1;sampleIndex<=72;sampleIndex++) {
            final boolean draw=RendererWglEvidence.scriptedVisibleAt(sampleIndex);
            final boolean invisible=RendererWglEvidence.scriptedHiddenAt(sampleIndex);
            check(!(draw && invisible),"R93 visible/hidden overlap");
            if(sampleIndex<=8) {
                check(!draw && !invisible,"R93 preflight must never draw");
            } else {
                check(draw || invisible,"R93 missing admitted visibility frame");
                if(draw)visible++; else hidden++;
            }
            if(sampleIndex<=28 && sampleIndex>=9 ||
               sampleIndex>=45 && sampleIndex<=72)
                check(draw,"R93 unexpected hidden phase");
        }
        check(visible==48 && hidden==16 &&
              !RendererWglEvidence.scriptedVisibleAt(0) &&
              !RendererWglEvidence.scriptedVisibleAt(73) &&
              !RendererWglEvidence.scriptedHiddenAt(73),
              "R93 show/hide/show exact cardinality failed");
        System.out.println("R93_SCRIPTED_SHOW_20_HIDE_16_SHOW_28_PASS");
        RenderFrameGate capped=new RenderFrameGate(8);
        sample(capped,15);
        check(capped.accepted()==8 && !capped.invalid() &&
              capped.await(1,TimeUnit.MILLISECONDS) &&
              capped.ownerThread().contains("#"),
              "Eight-frame bounded observation failed");
        RenderFrameGate wrongCaller=new RenderFrameGate(8);
        wrongCaller.sample(Thread.currentThread(),0.4f,1L,BAD);
        check(wrongCaller.invalid() && wrongCaller.accepted()==0,
              "Wrong caller not rejected");
        RenderFrameGate wrongThread=new RenderFrameGate(8);
        wrongThread.sample(Thread.currentThread(),0.4f,1L,GOOD);
        Thread foreign=new Thread(new Runnable() {
            @Override public void run() {
                wrongThread.sample(Thread.currentThread(),0.4f,2L,GOOD);
            }
        },"R75-foreign");
        foreign.start();foreign.join(3000L);
        check(!foreign.isAlive() && wrongThread.invalid() &&
              wrongThread.accepted()==1 &&
              wrongThread.await(1,TimeUnit.MILLISECONDS),
              "Cross-thread render sample not refused");
        for(float f:new float[]{Float.NaN,Float.POSITIVE_INFINITY,-0.1f,1.01f}) {
            RenderFrameGate invalid=new RenderFrameGate(8);
            invalid.sample(Thread.currentThread(),f,1L,GOOD);
            check(invalid.invalid() && invalid.accepted()==0,
                "Invalid render fraction admitted: "+f);
        }
        RenderFrameGate negativeTime=new RenderFrameGate(8);
        negativeTime.sample(Thread.currentThread(),0.2f,-1L,GOOD);
        check(negativeTime.invalid(),"Negative frame time admitted");
        RenderFrameGate stopped=new RenderFrameGate(8);
        stopped.stop();
        sample(stopped,9);
        check(stopped.accepted()==0 && stopped.await(1,TimeUnit.MILLISECONDS),
              "Stopped callback gate accepted additional frames");
        try {
            RendererFrameDiagnostic.weavePinnedOriginal(new byte[]{1,2,3});
            throw new AssertionError("Fake renderer passed pinned original SHA");
        } catch(IllegalArgumentException expected) {}
        System.out.println("R75_RENDER_OWNER_STACK_AND_EIGHT_FRAME_LIMIT_PASS");
        System.out.println("R75_CROSS_THREAD_INVALID_INPUT_AND_STOP_FAIL_CLOSED_PASS");
        System.out.println("R75_ORIGINAL_RENDERER_HASH_NEGATIVE_CONTROL_PASS");
        System.out.println("R75_NO_REAL_GAME_OR_GRAPHICS_TOUCHED");
    }
    private RenderFrameGateSmoke(){}
}
