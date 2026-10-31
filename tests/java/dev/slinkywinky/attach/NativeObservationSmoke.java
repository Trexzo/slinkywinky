package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.InstrumentationProbe;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Offline JNI ABI contract. Executes only in this disposable Java 8 process. */
public final class NativeObservationSmoke {
    private static void check(boolean ok,String what) {
        if(!ok) throw new AssertionError(what);
    }
    private static int step(Method method,long handle,long time,long generation,int flags,
                            double[] coords,float[] angles) throws Exception {
        return ((Integer)method.invoke(null,handle,time,generation,flags,coords,angles)).intValue();
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException("Absolute JNI library path required");
        System.load(args[0]);
        Class<?> api=InstrumentationProbe.class;
        Method open=api.getDeclaredMethod("nativeOpenObservation");
        Method observe=api.getDeclaredMethod("nativeObserveFrame",
                            long.class,long.class,long.class,int.class,
                            double[].class,float[].class);
        Method close=api.getDeclaredMethod("nativeCloseObservation",long.class);
        open.setAccessible(true);observe.setAccessible(true);close.setAccessible(true);
        final long handle=((Long)open.invoke(null)).longValue();
        check(handle!=0,"Native context creation");
        double[] xyz={1.0,65.62,-2.0,4.5,66.0,1.0};
        float[] camera={40f,-8f};
        // Same native owner, same session: preserve state across multiple calls.
        check(step(observe,handle,1000,1,0,new double[6],new float[2])==1,"Menu frame");
        check(step(observe,handle,1050,2,3,xyz,camera)==1,"World/player frame");
        check(step(observe,handle,1100,2,7,xyz,camera)==1,"Ray-selected target frame");
        check(step(observe,handle,1150,3,1,new double[6],new float[2])==1,
              "World without player permitted during reentry");
        check(step(observe,handle,1200,4,0,new double[6],new float[2])==1,
              "World disconnect");
        check(step(observe,handle,1250,5,3,xyz,camera)==1,"New world session");
        // Invalid frame shape, impossible flags and nonfinite values must fail closed.
        check(step(observe,handle,1300,5,2,xyz,camera)==0,"Player without world rejected");
        check(step(observe,handle,1300,5,4,xyz,camera)==0,"Target without world rejected");
        check(step(observe,handle,1300,5,6,xyz,camera)==0,"Target and player without world rejected");
        check(step(observe,handle,1300,5,8,xyz,camera)==0,"Unknown flag rejected");
        check(step(observe,handle,-1,5,3,xyz,camera)==0,"Negative time rejected");
        check(step(observe,handle,1300,0,3,xyz,camera)==0,"Zero generation rejected");
        check(step(observe,handle,1300,5,3,null,camera)==0,"Null coordinate array rejected");
        check(step(observe,handle,1300,5,3,xyz,null)==0,"Null camera array rejected");
        check(step(observe,handle,1300,5,3,new double[5],camera)==0,"Short coordinate array rejected");
        check(step(observe,handle,1300,5,3,xyz,new float[3])==0,"Long camera array rejected");
        double[] invalidXyz=xyz.clone(); invalidXyz[2]=Double.NaN;
        check(step(observe,handle,1300,5,3,invalidXyz,camera)==0,"NaN rejected");
        invalidXyz=xyz.clone();invalidXyz[4]=Double.POSITIVE_INFINITY;
        check(step(observe,handle,1300,5,7,invalidXyz,camera)==0,"Infinite target rejected");
        check(step(observe,handle,1300,5,3,xyz,new float[]{Float.NEGATIVE_INFINITY,0})==0,
              "Infinite yaw rejected");
        final AtomicInteger otherResult=new AtomicInteger(-99);
        final AtomicReference<Throwable> otherError=new AtomicReference<Throwable>();
        Thread outsider=new Thread(new Runnable() {
            @Override public void run() {
                try {otherResult.set(step(observe,handle,1350,5,3,
                                         new double[]{1,2,3,4,5,6},new float[]{0,0}));}
                catch(Throwable failure){otherError.set(failure);}
            }
        },"off-thread-negative-control");
        outsider.start(); outsider.join(5000L);
        check(!outsider.isAlive() && otherError.get()==null && otherResult.get()==0,
              "Foreign-thread observation must be rejected");
        check(step(observe,handle,1400,5,3,xyz,camera)==1,
              "Rejected calls must not destroy owner context");
        check(((Integer)close.invoke(null,handle)).intValue()==1,"Close from owner thread");
        System.out.println("NATIVE_OBSERVATION_CONTRACT_PASS");
    }
}
