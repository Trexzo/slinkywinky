package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RendererWglEvidence;

/**
 * R78: actual Win32 Java8 JNI passive WGL evidence with a disposable context.
 * Never starts Minecraft and never modifies the current native GL state.
 */
public final class R78WglEvidenceSmoke {
    private static void require(boolean ok,String why) {
        if(!ok)throw new AssertionError(why);
    }
    private static RendererWglEvidence fresh(String dll) throws Exception {
        return RendererWglEvidence.open(dll);
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=1 || !"1".equals(System.getenv("SLINKYWINKY_R78_WGL_FIXTURE")))
            throw new IllegalArgumentException("R78 fixture args and gate required");
        String dll=args[0];
        R76WglJvmSmoke.r78Load();

        RendererWglEvidence absent=fresh(dll);
        absent.sample();
        require(absent.attempts()==1 && absent.absent()==1 &&
                absent.present()==0 && !absent.fullyStable(),
                "R78 no-context sample should not be admitted");
        System.out.println("R78_NO_WGL_CONTEXT_READ_ONLY_REFUSAL_PASS");

        require(R76WglJvmSmoke.r78Begin(),"R78 disposable owner context missing");
        RendererWglEvidence stable=fresh(dll);
        for(int i=0;i<8;i++)stable.sample();
        stable.sample();
        require(stable.attempts()==8 && stable.present()==8 &&
                stable.fullyStable() && R76WglJvmSmoke.r78Finish(),
                "R78 exact eight-sample owner context not stable");
        System.out.println("R78_EIGHT_JAVA8_JNI_WGL_SNAPSHOTS_STABLE_PASS");

        require(R76WglJvmSmoke.r78Begin(),"Context-switch epoch setup failed");
        RendererWglEvidence changed=fresh(dll);
        changed.sample();
        require(R76WglJvmSmoke.r78Switch(),"Alternate context creation failed");
        changed.sample();
        require(R76WglJvmSmoke.r78Return(),"Return to owner context failed");
        changed.sample();
        require(changed.changed()==1 && !changed.fullyStable() &&
                R76WglJvmSmoke.r78Finish(),
                "R78 HGLRC change not detected");
        System.out.println("R78_WGL_CONTEXT_SWITCH_READ_ONLY_CHANGE_PASS");

        require(R76WglJvmSmoke.r78Begin(),"Viewport epoch setup failed");
        RendererWglEvidence viewport=fresh(dll);
        viewport.sample();
        require(R76WglJvmSmoke.r78ChangeViewport(),"WGL viewport drift unavailable");
        viewport.sample();
        require(viewport.changed()==1 && !viewport.fullyStable() &&
                R76WglJvmSmoke.r78Finish(),
                "R78 viewport drift was not observed");
        System.out.println("R78_GL_VIEWPORT_DRIFT_PASS");

        require(R76WglJvmSmoke.r78Begin(),"Context loss fixture setup failed");
        RendererWglEvidence lost=fresh(dll);
        lost.sample();
        require(R76WglJvmSmoke.r78Clear(),"Could not clear current context");
        lost.sample();
        require(R76WglJvmSmoke.r78Restore(),"Could not restore context");
        lost.sample();
        require(lost.absent()==1 && lost.present()==2 &&
                !lost.fullyStable() && R76WglJvmSmoke.r78Finish(),
                "R78 missed temporary WGL context loss");
        System.out.println("R78_WGL_CONTEXT_ABSENCE_NOT_REPORTED_AS_STABLE_PASS");

        require(R76WglJvmSmoke.r78Begin(),"Foreign-thread fixture setup failed");
        final RendererWglEvidence foreign=fresh(dll);
        foreign.sample();
        Thread other=new Thread(new Runnable() {
            @Override public void run() {foreign.sample();}
        },"R78-wrong-native-owner");
        other.start();other.join(3000L);
        require(!other.isAlive() && foreign.invalid()==1 &&
                !foreign.fullyStable() && R76WglJvmSmoke.r78Finish(),
                "R78 foreign Java thread unexpectedly admitted");
        System.out.println("R78_FOREIGN_JAVA_RENDER_THREAD_REJECTED_PASS");
        System.out.println("R78_NO_GAME_HOOK_NO_DRAW_NO_GL_MUTATION");
    }
    private R78WglEvidenceSmoke(){}
}
