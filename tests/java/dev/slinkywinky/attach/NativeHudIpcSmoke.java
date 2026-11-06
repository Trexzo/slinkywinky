package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.InstrumentationProbe;
import java.lang.reflect.Method;
import java.lang.management.ManagementFactory;
import java.util.concurrent.TimeUnit;

/** Disposable Java 8 JNI -> Windows IPC -> separate native process acceptance. */
public final class NativeHudIpcSmoke {
    private static void check(boolean accepted,String why){
        if(!accepted) throw new AssertionError(why);
    }
    public static void main(String[] args) throws Exception {
        check(args.length==2,"Expected JNI DLL and native IPC test executable");
        check("1".equals(System.getenv("SLINKYWINKY_HUD_IPC")),
              "This fixture requires explicit opt-in");
        System.load(args[0]);
        Class<?> api=InstrumentationProbe.class;
        Method open=api.getDeclaredMethod("nativeOpenObservation");
        Method observe=api.getDeclaredMethod("nativeObserveFrame",long.class,
            long.class,long.class,int.class,double[].class,float[].class);
        Method close=api.getDeclaredMethod("nativeCloseObservation",long.class);
        for(Method m:new Method[]{open,observe,close}) m.setAccessible(true);
        long handle=(Long)open.invoke(null);
        check(handle>0,"Opted-in native session must create publisher");
        try {
            int result=(Integer)observe.invoke(null,handle,1000L,1L,0,
                    new double[6],new float[2]);
            check(result==1,"Native zero-action frame rejected");
            String name=ManagementFactory.getRuntimeMXBean().getName();
            String pid=name.split("@")[0];
            check(pid.matches("[0-9]+"),"Could not resolve disposable JVM PID");
            Process process=new ProcessBuilder(args[1],"--java-reader",pid)
                .redirectErrorStream(true).start();
            // R103 CI reliability: the 1.5-second production HUD data TTL
            // is intentional. On busy hosted Windows runners starting the
            // separate process can exceed that TTL. Keep this disposable
            // FIRST-PARTY ZERO-ACTION test publisher active while its owned
            // child starts. Never lengthen the production expiry or send
            // gameplay input. The same owner JVM thread publishes every
            // strictly monotonic observation, at most 161 within 8 seconds.
            int published=1;
            final long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(8);
            while(process.isAlive() && System.nanoTime()<deadline &&
                  published<161) {
                if(process.waitFor(40,TimeUnit.MILLISECONDS))break;
                int outcome=(Integer)observe.invoke(null,handle,
                    1000L+50L*published,1L,0,
                    new double[6],new float[2]);
                check(outcome==1,"Zero-action publisher could not refresh a bounded IPC frame");
                published++;
            }
            check(process.waitFor(1,TimeUnit.SECONDS),"Separate Windows reader hung");
            check(process.exitValue()==0,"Separate process did not see a fresh JNI-published frame");
            System.out.println("JNI_CROSS_PROCESS_HUD_IPC_PASS");
        } finally {
            check((Integer)close.invoke(null,handle)==1,
                  "Native publisher must be revoked on session close");
        }
    }
}
