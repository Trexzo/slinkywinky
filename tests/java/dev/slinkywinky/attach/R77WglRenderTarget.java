package dev.slinkywinky.attach;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

/** Isolated R77 Java8 process; synthetic obfuscated owner names, its OWN WGL. */
public final class R77WglRenderTarget {
    private static void check(boolean ok,String why) {
        if(!ok)throw new AssertionError(why);
    }
    public static void main(String[] args) throws Exception {
        check("1".equals(System.getenv("SLINKYWINKY_R74_DISPOSABLE_FIXTURE")) &&
              "1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")) &&
              "1".equals(System.getenv("SLINKYWINKY_R77_DISPOSABLE_FIXTURE")),
              "R77 all-three disposable opt-ins absent");
        R76WglJvmSmoke.r77Load();
        check(R76WglJvmSmoke.r77Begin(),"R77 owned WGL context unavailable");

        // Exact original names, but generated Java classes only.
        Class<?> gameClass=Class.forName("ave");
        Class<?> rendererClass=Class.forName("bfk");
        final Object game=gameClass.getMethod("A").invoke(null);
        final Method frame=gameClass.getMethod("av",float.class,long.class);
        final Field work=rendererClass.getField("work");
        R77WglRenderReceiver.beginOwner();
        System.out.println("R77_DISPOSABLE_WGL_RENDER_TARGET_READY");
        System.out.flush();

        long clock=1L;
        for(int i=0;i<2300 && !R74RenderAttachAgent.ready();i++) {
            frame.invoke(game,0.5f,clock++);
            Thread.sleep(10L);
        }
        check(R74RenderAttachAgent.ready(),
              "External test-only Java8 agent never admitted");
        int original=work.getInt(null);
        for(int i=0;i<16;i++) {
            frame.invoke(game,0.5f,clock++);
            Thread.sleep(12L);
        }
        check(R77WglRenderReceiver.completeOwner()==8 &&
              work.getInt(null)-original==16,
              "Expected eight JNI-render callbacks and sixteen original frames");
        System.out.println("R77_ATTACHED_BFK_CALLBACK_TO_NATIVE_GL11_MENU_PASS");
        System.out.println("R77_JAVA8_RENDER_OWNER_AND_WGL_STATE_RESTORE_PASS");
        System.out.flush();

        R74RenderAttachAgent.restoreOnOwner();
        int before=work.getInt(null);
        for(int i=0;i<12;i++)frame.invoke(game,0.5f,clock++);
        check(work.getInt(null)-before==12 &&
              R77WglRenderReceiver.count()==8 &&
              !R77WglRenderReceiver.invalid() &&
              R76WglJvmSmoke.r77Count()==8,
              "Generated renderer after retransformation was not restored");
        check(R76WglJvmSmoke.r77Finish(),
              "Disposable native WGL context did not shut down cleanly");
        System.out.println("R77_JAVA8_ATTACH_BYTECODE_RESTORE_NO_FURTHER_DRAW_PASS");
        System.out.println("R77_DISPOSABLE_WGL_CONTEXT_CLEAN_EXIT");
        System.out.flush();
    }
    private R77WglRenderTarget(){}
}
