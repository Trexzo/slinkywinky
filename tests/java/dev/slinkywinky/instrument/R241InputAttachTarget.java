package dev.slinkywinky.instrument;

/** R241 external JVM target. Never launches or imports Minecraft classes. */
public final class R241InputAttachTarget {
    public static void main(String[] args) throws Exception {
        if(args.length!=0 ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R241 disposable target required");
        final R240InputFixture fixture=new R240InputFixture();
        System.out.println("R241_DISPOSABLE_INPUT_TARGET_READY");
        System.out.flush();
        final long deadline=System.nanoTime()+20000000000L;
        while(!R241InputAttachAgent.isReady() && System.nanoTime()<deadline)
            Thread.sleep(20);
        if(!R241InputAttachAgent.isReady())
            throw new IllegalStateException("R241 agent attach timeout");
        for(int i=0;i<8;i++)fixture.tick();
        if(fixture.calls!=8)
            throw new AssertionError("R241 original input body changed under observer");
        R241InputAttachAgent.restoreOnOwner();
        for(int i=0;i<8;i++)fixture.tick();
        if(fixture.calls!=16)
            throw new AssertionError("R241 original input behavior changed after restore");
        System.out.println("R241_ORIGINAL_INPUT_BODY_8_PLUS_8_PASS calls="+fixture.calls);
        System.out.println("R241_DISPOSABLE_INPUT_ATTACH_CLEAN_EXIT");
        System.out.flush();
    }
    private R241InputAttachTarget(){}
}
