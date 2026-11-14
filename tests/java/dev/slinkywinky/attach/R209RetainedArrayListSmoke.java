package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RendererWglEvidence;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R209: real Java8 -> JNI -> disposable Windows WGL -> retained menu +
 * owner-published module ArrayList. No Minecraft process or input hooks.
 */
public final class R209RetainedArrayListSmoke {
    private static void need(boolean accepted,String explanation) {
        if(!accepted)throw new AssertionError(explanation);
    }
    private static Method method(String name,Class<?>... signature) throws Exception {
        Method result=RendererWglEvidence.class.getDeclaredMethod(name,signature);
        result.setAccessible(true);
        return result;
    }
    private static boolean call(Method method,Object... params) throws Exception {
        return Boolean.TRUE.equals(method.invoke(null,params));
    }
    public static void main(String[] argv) throws Exception {
        need(argv.length==1 &&
             "1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")) &&
             "1".equals(System.getenv("SLINKYWINKY_R78_WGL_FIXTURE")),
             "R209 disposable-gate required");
        R76WglJvmSmoke.r78Load();
        RendererWglEvidence.open(argv[0]);
        Method arm=method("nativeArmRetainedPreview");
        Method draw=method("nativeDrawRetainedPreview",
            long.class,long.class,int.class,int.class);
        Method publish=method("nativePublishRetainedModuleArrayList",
            long.class,long.class,int.class,int.class,
            long.class,long.class,boolean.class,
            String[].class,boolean[].class,boolean[].class);
        Method end=method("nativeEndRetainedPreview");
        String[] names={"Sprint","Timer"};
        boolean[] req={true,true};
        boolean[] onlySprint={true,false};
        need(!call(publish,0L,0L,640,480,1L,9L,true,
            names,req,onlySprint),"unbound publication accepted");
        need(R76WglJvmSmoke.r78Begin(),"disposable owner WGL setup failed");
        long[] h=R76WglJvmSmoke.r209Handles();
        need(h!=null && h.length==2 && h[0]!=0 && h[1]!=0,
            "fixture failed to supply own WGL handles");
        need(call(arm),"cannot arm retained preview");
        need(!call(publish,h[0],h[1],640,480,1L,9L,true,
            names,req,onlySprint),"publisher accepted before painted owner");
        need(R76WglJvmSmoke.r209ClearFrame(),"fixture clear failed");
        need(call(draw,h[0],h[1],640,480),
            "first original-shaped retained menu paint failed");
        final AtomicBoolean foreign=new AtomicBoolean(true);
        Thread other=new Thread(new Runnable(){
            @Override public void run() {
                try {
                    foreign.set(call(publish,h[0],h[1],640,480,1L,9L,true,
                        names,req,onlySprint));
                } catch(Exception ignored) {foreign.set(false);}
            }
        },"r209-disposable-foreign-publisher");
        other.start();other.join(4000);
        need(!other.isAlive() && !foreign.get(),"foreign thread admitted");
        need(call(publish,h[0],h[1],640,480,1L,9L,true,
            names,req,onlySprint),"valid owner snapshot rejected");
        need(R76WglJvmSmoke.r209ClearFrame(),"frame clear failed");
        need(call(draw,h[0],h[1],640,480),
            "retained menu + active module ArrayList draw rejected");
        final int accent=0xF35D12;
        need(R76WglJvmSmoke.r209Pixel(630,464)==accent,
             "actual native ArrayList accent pixel missing");
        need(R76WglJvmSmoke.r209Pixel(630,443)!=accent,
             "requested-only Timer incorrectly appeared as active");
        System.out.println("R209_JNI_RETAINED_WGL_ACTIVE_ONLY_ARRAYLIST_PIXEL_PASS");
        need(call(publish,h[0],h[1],640,480,2L,9L,false,
            names,req,new boolean[]{false,false}),"world-loss retirement refused");
        need(R76WglJvmSmoke.r209ClearFrame(),"world-loss clear failed");
        need(call(draw,h[0],h[1],640,480),"world-loss menu draw failed");
        need(R76WglJvmSmoke.r209Pixel(630,464)!=accent,
            "stale active row survived world loss");
        need(!call(publish,h[0],h[1],640,480,2L,9L,false,
            names,req,new boolean[]{false,false}),"replayed sequence accepted");
        need(R76WglJvmSmoke.r209ClearFrame(),"post-replay clear failed");
        need(call(draw,h[0],h[1],640,480),"empty quarantined frame failed");
        need(R76WglJvmSmoke.r209Pixel(630,464)!=accent,
            "quarantined producer reactivated stale pixels");
        need(call(end),"retained owner release failed");
        need(!call(publish,h[0],h[1],640,480,3L,9L,true,
            names,req,onlySprint),"publication succeeded after unload");
        need(R76WglJvmSmoke.r78Finish(),"WGL fixture teardown failed");
        System.out.println("R209_OWNER_SEQUENCE_WORLD_LOSS_AND_UNLOAD_PASS");
    }
    private R209RetainedArrayListSmoke(){}
}
