package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189SourceEvidence;
import dev.slinkywinky.SourceRuntimePreview;

/** Java8 exact-source primitive copy -> original native Lag Range e18. */
public final class Vanilla189SourceNativeLagSmoke {
    static void yes(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    static Vanilla189SourceEvidence.Sample source(Object player,Object target){
        try{return Vanilla189SourceEvidence.read(player,target);}
        catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
    }
    static SourceRuntimePreview.Receipt tick(SourceRuntimePreview nativeSource,
            long generation,long sequence,Vanilla189SourceEvidence.Sample sample){
        int[] flags={1,0,sample==null?0:1,0,0,0,0,0,0,0,0,0};
        double[] fields={9,10,0,0,0,150};
        if(sample!=null){
            fields[2]=sample.x;fields[3]=sample.y;fields[4]=sample.z;
        }
        return nativeSource.observe(1001,1002,generation,sequence,
            SourceRuntimePreview.LAG_RANGE_TICK,flags,fields);
    }
    /** R358 fixtures are package-private default-package exact obfuscated names. */
    static Object fixture(String name)throws Exception{
        java.lang.reflect.Constructor<?> ctor=Class.forName(name).getDeclaredConstructor();
        ctor.setAccessible(true);
        return ctor.newInstance();
    }
    public static void main(String[] args)throws Exception{
        Object player=fixture("pr");
        Object target=fixture("bex");
        Vanilla189SourceEvidence.Sample sample=source(player,target);
        yes(sample!=null&&sample.x==10&&sample.y==5&&sample.z==-4&&
            sample.lastAttackTick==18,"Exact source original candidate captured");
        SourceRuntimePreview jni=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt first=tick(jni,1,1,sample);
        yes(first.sourceEvent==18&&first.at(2)==1&&first.at(3)==1,
            "Real native C++ Lag Range event18 executed and active");
        SourceRuntimePreview.Receipt lost=tick(jni,1,2,null);
        yes(lost.at(2)==1&&lost.at(3)==0,
            "Ray target loss must disarm native Lag Range within same epoch");
        SourceRuntimePreview.Receipt second=tick(jni,1,3,sample);
        yes(second.at(2)==1&&second.at(3)==1,
            "Fresh original candidate source sample reactivates native model");
        SourceRuntimePreview.Receipt world=tick(jni,2,1,null);
        yes(world.at(2)==1&&world.at(3)==0,
            "New world owner generation has no stale candidate target");
        boolean rejected=false;
        try{tick(jni,1,4,sample);}
        catch(IllegalArgumentException|IllegalStateException expected){rejected=true;}
        yes(rejected,"Old original world generation forbidden");
        jni.close();
        rejected=false;
        try{tick(jni,2,2,sample);}
        catch(IllegalStateException expected){rejected=true;}
        yes(rejected,"Closed native source owner cannot execute");
        Vanilla189SourceEvidence.release();
        System.out.println("R358_REAL_ORIGINAL_SOURCE_VALUES_TO_NATIVE_LAG_E18_PASS");
    }
}
