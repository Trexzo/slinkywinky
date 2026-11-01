package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.InstrumentationProbe;
import dev.slinkywinky.instrument.ObservedState;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.atomic.AtomicBoolean;

/** R50: deterministic no-Minecraft attach-admission race regressions. */
public final class HookAdmissionSmoke {
    private static void check(boolean ok,String why) {
        if(!ok) throw new AssertionError(why);
    }
    private static Instrumentation fakeInstrumentation() {
        return (Instrumentation)Proxy.newProxyInstance(
            Instrumentation.class.getClassLoader(),new Class<?>[]{Instrumentation.class},
            new InvocationHandler() {
                @Override public Object invoke(Object proxy,Method method,Object[] args) {
                    String name=method.getName();
                    if("isRetransformClassesSupported".equals(name)) return true;
                    if("getAllLoadedClasses".equals(name))
                        return new Class<?>[]{AttachTarget.class};
                    if("isModifiableClass".equals(name)) return true;
                    throw new AssertionError("Unexpected JVM mutation before gate: "+name);
                }
            });
    }
    public static void main(String[] args) throws Exception {
        final Instrumentation fake=fakeInstrumentation();
        Field claimedField=InstrumentationProbe.class.getDeclaredField("hookInstalled");
        claimedField.setAccessible(true);
        AtomicBoolean claimed=(AtomicBoolean)claimedField.get(null);
        Field generation=ObservedState.class.getDeclaredField("generation");
        generation.setAccessible(true);
        Field oldWorld=ObservedState.class.getDeclaredField("oldWorld");
        oldWorld.setAccessible(true);
        long savedGeneration=generation.getLong(null);
        Object savedWorld=oldWorld.get(null);
        try {
            final Object sentinel=new Object();
            generation.setLong(null,917L);
            oldWorld.set(null,sentinel);
            claimed.set(true);
            boolean refused=false;
            try {
                InstrumentationProbe.agentmain("hook-probe|X:\\not-present\\slinkywinky_jni.dll",
                                               fake);
            } catch(IllegalStateException expected) {
                refused=expected.getMessage()!=null &&
                        expected.getMessage().contains("already active");
            }
            check(refused,"Concurrent/repeated attach must reject before JNI preflight");
            check(claimed.get(),"Active native hook ownership must remain claimed");
            boolean passiveRefused=false;
            try {
                InstrumentationProbe.agentmain("probe",fake);
            } catch(IllegalStateException expected) {
                passiveRefused=expected.getMessage()!=null &&
                               expected.getMessage().contains("already active");
            }
            check(passiveRefused,"Passive retransformation must not run over active hook");
            check(claimed.get(),"Passive attempt must not clear active hook claim");
            check(generation.getLong(null)==917L && oldWorld.get(null)==sentinel,
                  "Duplicate admission must not reset active observation state");
            claimed.set(false);
            boolean missingPath=false;
            try {
                InstrumentationProbe.agentmain("hook-probe",fake);
            } catch(IllegalArgumentException expected) {
                missingPath=expected.getMessage()!=null &&
                            expected.getMessage().contains("Missing exact JNI");
            }
            check(missingPath,"Missing JNI path must fail before any instrumentation");
            check(!claimed.get(),"Preflight failure must release unused admission");
            // The native agent accepts only two finite, explicitly listed extensions.
            Method parse=InstrumentationProbe.class.getDeclaredMethod(
                "parseExtendedLimit",String.class);
            parse.setAccessible(true);
            check(((Integer)parse.invoke(null,"120"))==120,"Short diagnostic bound");
            check(((Integer)parse.invoke(null,"600"))==600,"Long diagnostic bound");
            for(String invalid:new String[]{"0","19","20","121","601","-1",
                                            "unlimited","120|600"," 120"}) {
                boolean denied=false;
                try { parse.invoke(null,invalid); }
                catch(java.lang.reflect.InvocationTargetException failure) {
                    denied=failure.getCause() instanceof IllegalArgumentException;
                }
                check(denied,"Unlisted diagnostic duration was accepted: "+invalid);
            }
            System.out.println("HOOK_DURATION_POLICY_PASS");
            System.out.println("HOOK_ADMISSION_RACE_GUARD_PASS");
        } finally {
            claimed.set(false);
            generation.setLong(null,savedGeneration);
            oldWorld.set(null,savedWorld);
        }
    }
}
