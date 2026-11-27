package dev.slinkywinky.attach;

import dev.slinkywinky.SourceRuntimePreview;

/** R359 real Java8/JNI handshake after externally preloaded absolute DLL. */
public final class SourcePreloadedNativeSmoke {
    static void yes(boolean value,String what){
        if(!value)throw new AssertionError(what);
    }
    public static void main(String[] args)throws Exception{
        if(args.length!=1)throw new IllegalArgumentException("Expected exact JNI DLL");
        System.load(args[0]); // Exact same classloader as the source facade.
        SourceRuntimePreview.admitPreloadedLibrary();
        SourceRuntimePreview r=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt frame=r.observe(
            1001,1002,1,1,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,0,0,0,0,0,0,0,0,0},
            new double[]{9,10,10,0,0,150});
        yes(frame.sourceEvent==18&&frame.at(2)==1&&frame.at(3)==1,
            "Preloaded absolute DLL must run ACTUAL native Lag Range e18");
        r.close();
        boolean closed=false;
        try{r.observe(1001,1002,1,2,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,0,0,0,0,0,0,0,0,0,0},
            new double[]{0,10,0,0,0,150});}
        catch(IllegalStateException expected){closed=true;}
        yes(closed,"Closed source owner cannot run native event");
        System.out.println("R359_JAVA8_ABSOLUTE_DLL_PRELOAD_NATIVE_SOURCE_PASS");
    }
}
