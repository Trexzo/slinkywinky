package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RenderCallbackProbe;

/**
 * R66 disposable Java 8 renderer-shaped target. NOT Minecraft.
 * Its only method eligible for modification is a(float,long).
 */
public final class R65Fixture {
    private static final R65Fixture INSTANCE = new R65Fixture();
    public static volatile int work;
    private R65Fixture() {}

    public void a(float partialTicks, long frameNanos) {
        work++; // Original method must still execute once per call.
    }

    public static void main(String[] args) throws Exception {
        if (!"1".equals(System.getenv("SLINKYWINKY_R66_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("Disposable-only fixture gate absent");
        System.out.println("R66_DISPOSABLE_RENDER_TARGET_READY");
        System.out.flush();
        long sequence=1L;
        // Existing class is loaded and actively invoked before external attach.
        for(int i=0;i<2200 && !RenderAttachAgent.hookReady();i++) {
            INSTANCE.a(0.5f,sequence++);
            Thread.sleep(12L);
        }
        if (!RenderAttachAgent.hookReady())
            throw new IllegalStateException("External fixture agent never admitted");
        // This is the original renderer thread; no agent thread owns callback state.
        if(!RenderCallbackProbe.armFixture(41L,8))
            throw new AssertionError("Unable to arm on actual fixture owner");
        final int before=work;
        for(int i=0;i<16;i++) {
            INSTANCE.a(0.25f,sequence++);
            Thread.sleep(18L);
        }
        RenderCallbackProbe.Summary result=RenderCallbackProbe.closeFixture(41L);
        if(result.invalid || result.accepted!=8 || work-before!=16)
            throw new AssertionError("Expected eight passive callbacks and 16 original frames");
        System.out.println("R66_OWNER_FRAME_COUNT_PASS accepted="+result.accepted);
        System.out.flush();
        RenderAttachAgent.restoreOnOwner();
        final int restoredWork=work;
        for(int i=0;i<16;i++)INSTANCE.a(0.5f,sequence++);
        if(work-restoredWork!=16)
            throw new AssertionError("Restored original fixture method did not execute");
        System.out.println("R66_ORIGINAL_RENDER_BEHAVIOR_PASS frames=16");
        System.out.println("R66_DISPOSABLE_ATTACH_CLEAN_EXIT");
        System.out.flush();
    }
}
