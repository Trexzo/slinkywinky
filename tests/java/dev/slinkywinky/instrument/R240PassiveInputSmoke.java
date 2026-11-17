package dev.slinkywinky.instrument;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicBoolean;

/** Same ASM method-entry weave, in a disposable class loader only. */
public final class R240PassiveInputSmoke {
    private static void check(boolean pass,String explanation){
        if(!pass)throw new AssertionError("R240 "+explanation);
    }
    private static final class FixtureLoader extends ClassLoader {
        FixtureLoader(){super(R240PassiveInputSmoke.class.getClassLoader());}
        Class<?> define(byte[] raw){
            return defineClass("dev.slinkywinky.instrument.R240InputFixture",
                               raw,0,raw.length);
        }
    }
    private static byte[] source()throws Exception {
        InputStream in=R240InputFixture.class.getResourceAsStream(
            "R240InputFixture.class");
        check(in!=null,"No source input fixture");
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        try {byte[] buf=new byte[1024];int read;
            while((read=in.read(buf))!=-1)out.write(buf,0,read);
        } finally {in.close();}
        return out.toByteArray();
    }
    private static boolean rejected(byte[] raw){
        try {OriginalInputFrameDiagnostic.weaveFixture(raw);return false;}
        catch(IllegalArgumentException expected){return true;}
    }
    public static void main(String[] args)throws Exception {
        check(args.length==0,"No original Minecraft or external process");
        byte[] raw=source(),baseline=raw.clone();
        byte[] candidate=OriginalInputFrameDiagnostic.weaveFixture(raw);
        check(candidate.length>raw.length &&
              java.util.Arrays.equals(raw,baseline),
              "Original input class bytes changed");
        check(rejected(candidate) && rejected(new byte[]{1,2,3}),
              "Repeated/invalid callback weaving admitted");
        final Class<?> type=new FixtureLoader().define(candidate);
        final Object instance=type.newInstance();
        final Method tick=type.getMethod("tick");
        final Field calls=type.getField("calls");
        OriginalInputFrameDiagnostic.fixtureBegin();
        for(int i=0;i<4;i++)tick.invoke(instance);
        check(calls.getInt(instance)==4,
              "Vanilla input method body was suppressed");
        final int[] counts=OriginalInputFrameDiagnostic.fixtureEnd();
        check(counts[0]==4 && counts[1]==0,"Entry counts wrong");
        tick.invoke(instance);
        check(calls.getInt(instance)==5 &&
              OriginalInputFrameDiagnostic.fixtureEnd()[0]==4,
              "Stopped observer blocked vanilla or accepted late callback");
        OriginalInputFrameDiagnostic.fixtureBegin();
        tick.invoke(instance);
        final AtomicBoolean foreignRan=new AtomicBoolean(false);
        Thread worker=new Thread(new Runnable(){
            @Override public void run(){
                try{tick.invoke(instance);foreignRan.set(true);}
                catch(Exception error){foreignRan.set(false);}
            }
        });
        worker.start();worker.join();
        final int[] wrong=OriginalInputFrameDiagnostic.fixtureEnd();
        check(foreignRan.get() && wrong[0]==1 && wrong[1]==1 &&
              calls.getInt(instance)==7,
              "Foreign thread input callback escaped or stole owner");
        System.out.println("R240_JAVA8_INPUT_DISPATCH_METHOD_BODY_UNCHANGED_PASS");
        System.out.println("R240_PASSIVE_OWNER_BOUND_ENTRY_AND_DRAIN_PASS");
        System.out.println("R240_ORIGINAL_INPUT_INTERCEPTION_REMAINS_DISABLED");
    }
}
