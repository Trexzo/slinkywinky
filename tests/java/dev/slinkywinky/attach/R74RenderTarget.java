package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RenderCallbackProbe;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
import java.util.concurrent.atomic.AtomicBoolean;

/** Disposable render-shaped Java 8 process exercising exact obfuscated names.
 * Uses locally generated classes; it is not Minecraft.
 */
public final class R74RenderTarget {
    private static void require(boolean passed,String why) {
        if(!passed)throw new AssertionError(why);
    }
    public static void main(String[] args) throws Exception {
        require("1".equals(System.getenv("SLINKYWINKY_R74_DISPOSABLE_FIXTURE")),
            "R74 opt-in absent");
        Class<?> gameClass=Class.forName("ave");
        Class<?> rendererClass=Class.forName("bfk");
        final Object game=gameClass.getMethod("A").invoke(null);
        final Method render=gameClass.getMethod("av",float.class,long.class);
        final Field work=rendererClass.getField("work");
        System.out.println("R74_GENERATED_OBFUSCATED_TARGET_READY");
        System.out.flush();
        long n=1;
        for(int i=0;i<2100 && !R74RenderAttachAgent.ready();i++) {
            render.invoke(game,0.5f,n++);
            Thread.sleep(12);
        }
        require(R74RenderAttachAgent.ready(),"External obfuscated fixture agent unavailable");
        require(RenderCallbackProbe.armFixture(74L,8),"Unable to arm R74 render owner");
        int start=work.getInt(null);
        for(int i=0;i<16;i++) {
            render.invoke(game,0.5f,n++);
            Thread.sleep(15);
        }
        RenderCallbackProbe.Summary summary=RenderCallbackProbe.closeFixture(74L);
        require(!summary.invalid && summary.accepted==8 &&
                work.getInt(null)-start==16,
                "Expected 8 callbacks and 16 original render invocations");
        System.out.println("R74_OBFUSCATED_OWNER_CALLBACKS_PASS accepted=8 original=16");
        System.out.flush();
        require(RenderCallbackProbe.armFixture(75L,8),"Owner reuse denied");
        final AtomicBoolean ran=new AtomicBoolean();
        Thread unexpected=new Thread(new Runnable() {
            @Override public void run() {
                try {
                    render.invoke(game,0.3f,999L);
                    ran.set(true);
                } catch(Throwable ex) {throw new AssertionError(ex);}
            }
        },"R74-offthread-negative");
        unexpected.start();unexpected.join();
        RenderCallbackProbe.Summary offThread=RenderCallbackProbe.closeFixture(75L);
        require(ran.get() && offThread.invalid && offThread.accepted==0,
                "Wrong-thread render callback unexpectedly admitted");
        System.out.println("R74_WRONG_RENDER_THREAD_FAIL_CLOSED_PASS");
        System.out.flush();
        R74RenderAttachAgent.restoreOnOwner();
        require(RenderCallbackProbe.armFixture(76L,4),"Unable to arm restoration negative");
        final int beforeRestore=work.getInt(null);
        for(int i=0;i<12;i++)render.invoke(game,0.5f,n++);
        RenderCallbackProbe.Summary restored=RenderCallbackProbe.closeFixture(76L);
        require(restored.accepted==0 && !restored.invalid &&
                work.getInt(null)-beforeRestore==12,
                "Original renderer body not fully restored");
        System.out.println("R74_OBFUSCATED_RENDER_RESTORED_NO_CALLBACK_PASS");
        System.out.println("R74_DISPOSABLE_PROCESS_CLEAN_EXIT");
        System.out.flush();
    }
    private R74RenderTarget(){}
}
