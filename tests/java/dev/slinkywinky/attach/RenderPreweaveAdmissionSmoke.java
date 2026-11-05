package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.InstrumentationProbe;
import dev.slinkywinky.instrument.RendererFrameDiagnostic;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R80: fully synthetic Java 8 JVMTI proxy. Tests clean early refusal vs.
 * uncertain passive-inspector installation/removal, with NO actual Attach,
 * native loading, Minecraft, class bytecode mutation or GL.
 */
public final class RenderPreweaveAdmissionSmoke {
    private static void check(boolean ok,String reason) {
        if(!ok)throw new AssertionError("R80 "+reason);
    }
    private static byte[] bytes(Class<?> type) throws Exception {
        final String resource="/"+type.getName().replace('.','/')+".class";
        try(InputStream input=type.getResourceAsStream(resource)) {
            if(input==null)throw new IllegalStateException("Missing generated "+resource);
            ByteArrayOutputStream output=new ByteArrayOutputStream();
            byte[] block=new byte[2048];int n;
            while((n=input.read(block))!=-1)output.write(block,0,n);
            return output.toByteArray();
        }
    }
    private enum Mode {
        UNRELATED, GENERATED_HASH_MISMATCH,
        REMOVE_FALSE, REMOVE_THROWS, ADD_THROWS, RETRANSFORM_UNSUPPORTED,
        VM_ERROR_ON_CLASS_DISCOVERY, R85_RESTORE_SYNTHETIC
    }
    private static final class Fake implements InvocationHandler {
        final Mode mode;
        ClassFileTransformer registered;
        int added,removed,retransformed;
        Fake(Mode mode){this.mode=mode;}
        Instrumentation instrumentation() {
            return (Instrumentation)Proxy.newProxyInstance(
                Instrumentation.class.getClassLoader(),
                new Class<?>[]{Instrumentation.class},this);
        }
        @Override public Object invoke(Object proxy,Method method,Object[] arguments)
                throws Throwable {
            final String name=method.getName();
            if("isRetransformClassesSupported".equals(name))
                return mode!=Mode.RETRANSFORM_UNSUPPORTED;
            if("getAllLoadedClasses".equals(name)) {
                if(mode==Mode.VM_ERROR_ON_CLASS_DISCOVERY)
                    throw new LinkageError("Synthetic unrecoverable VM discovery error");
                return mode==Mode.UNRELATED ?
                    new Class<?>[]{RenderPreweaveAdmissionSmoke.class} :
                    new Class<?>[]{Class.forName("ave"),Class.forName("bfk")};
            }
            if("isModifiableClass".equals(name))return true;
            if("addTransformer".equals(name)) {
                added++;
                if(mode==Mode.ADD_THROWS)
                    throw new IllegalStateException("Simulated uncertain registration");
                check(registered==null,"Unexpected duplicate passive inspector");
                registered=(ClassFileTransformer)arguments[0];
                return null;
            }
            if("removeTransformer".equals(name)) {
                removed++;
                check(registered==arguments[0],"Removed unexpected inspector");
                if(mode==Mode.REMOVE_THROWS)
                    throw new IllegalStateException("Simulated uncertain removal");
                if(mode==Mode.REMOVE_FALSE)return false;
                registered=null;
                return true;
            }
            if("retransformClasses".equals(name)) {
                check(registered!=null,"Retransformation before inspector installed");
                Class<?>[] chosen=(Class<?>[])arguments[0];
                check(chosen.length==(mode==Mode.R85_RESTORE_SYNTHETIC?1:2),
                      "Unexpected original admission/restore read count");
                for(Class<?> type:chosen) {
                    final byte[] source=bytes(type);
                    byte[] modified=registered.transform(type.getClassLoader(),
                        type.getName().replace('.','/'),type,
                        type.getProtectionDomain(),source);
                    check(modified==null,
                          "Passive preflight/restore inspector modified class bytes");
                    retransformed++;
                }
                return null;
            }
            throw new AssertionError("Unexpected pre-weave instrumentation method: "+name);
        }
    }

    private static boolean denied(Fake fake,String option) {
        try {
            InstrumentationProbe.agentmain(option,fake.instrumentation());
        } catch(IllegalStateException expected) {
            return true;
        }
        return false;
    }
    private static void caseClean(AtomicBoolean gate,Mode mode,String modeArg,
                                  int expectedAdds,int expectedRemoves) {
        gate.set(false);
        Fake fake=new Fake(mode);
        check(denied(fake,modeArg),"Clean pre-weave refusal unexpectedly succeeded");
        check(!gate.get(),"Clean pre-weave failure stranded global admission");
        check(fake.added==expectedAdds && fake.removed==expectedRemoves,
              "Wrong passive inspector lifecycle");
        check(fake.registered==null,
              "Clean failed preflight leaked passive inspector");
    }
    private static void caseUncertain(AtomicBoolean gate,Mode mode,String option) {
        gate.set(false);
        Fake fake=new Fake(mode);
        check(denied(fake,option),
              "Unconfirmed passive registration/removal passed: "+option);
        check(gate.get(),"Uncertain passive inspector released admission: "+option);
        final int addedBefore=fake.added;
        check(denied(fake,"minecraft-render-callback-r75"),
              "Concurrent renderer hook bypassed uncertain preflight gate");
        check(fake.added==addedBefore,
              "Second attachment touched instrumentation after unresolved failure");
    }
    private static void caseUnexpectedVmError(AtomicBoolean gate) {
        gate.set(false);
        Fake fake=new Fake(Mode.VM_ERROR_ON_CLASS_DISCOVERY);
        boolean errorObserved=false;
        try {
            InstrumentationProbe.agentmain("minecraft-render-preflight",
                                           fake.instrumentation());
        } catch(LinkageError expected) {
            errorObserved=true;
        }
        check(errorObserved,"Simulated JVM error was unexpectedly swallowed");
        check(gate.get(),"Unexpected JVM error improperly released R73 admission");
        check(fake.added==0,"VM error unexpectedly registered class inspector");
        check(denied(fake,"minecraft-render-callback-r75"),
              "Next Attach admitted following unrecoverable preflight error");
    }

    /**
     * An uncertain weave removal must never invoke even the passive restore
     * transformer (its retransform could execute a still-registered weaver).
     * The synthetic class deliberately fails the pinned-original SHA when a
     * confirmed-removal test does reach the restoration verifier.
     */
    private static void caseRestoreSkippedOnUncertainWeaverRemoval() throws Exception {
        Method gated=RendererFrameDiagnostic.class.getDeclaredMethod(
            "restoreIfWeaverRemoved",boolean.class,Instrumentation.class,Class.class);
        gated.setAccessible(true);
        Fake fake=new Fake(Mode.R85_RESTORE_SYNTHETIC);
        Instrumentation instrument=fake.instrumentation();
        boolean refused=(Boolean)gated.invoke(null,false,instrument,
                                               RenderPreweaveAdmissionSmoke.class);
        check(!refused,"Unremoved weaver incorrectly certified restored");
        check(fake.added==0 && fake.removed==0 && fake.retransformed==0,
              "Uncertain weaving-removal triggered unsafe retransform");
        check(fake.registered==null,
              "Unexpected passive transformer after skipped restore");
        boolean restored=(Boolean)gated.invoke(null,true,instrument,
                                                RenderPreweaveAdmissionSmoke.class);
        check(!restored,"Generated fixture incorrectly passed pinned SHA");
        check(fake.added==1 && fake.removed==1 && fake.retransformed==1,
              "Confirmed weave removal did not attempt bounded passive restore");
        check(fake.registered==null,
              "Confirmed-removal passive inspector was not cleaned up");
    }

    public static void main(String[] args) throws Exception {
        Field field=InstrumentationProbe.class.getDeclaredField("hookInstalled");
        field.setAccessible(true);
        AtomicBoolean gate=(AtomicBoolean)field.get(null);
        check(!gate.get(),"R80 test starts with active hook");
        try {
            caseClean(gate,Mode.UNRELATED,"minecraft-render-callback-r75",0,0);
            caseClean(gate,Mode.UNRELATED,"minecraft-render-preflight",0,0);
            caseClean(gate,Mode.UNRELATED,
                      "minecraft-render-context-r78|X:\\missing\\slinkywinky_jni.dll",0,0);
            caseClean(gate,Mode.RETRANSFORM_UNSUPPORTED,
                      "minecraft-render-callback-r75",0,0);
            caseClean(gate,Mode.GENERATED_HASH_MISMATCH,
                      "minecraft-render-callback-r75",1,1);
            caseClean(gate,Mode.GENERATED_HASH_MISMATCH,
                      "minecraft-render-preflight",1,1);
            caseClean(gate,Mode.GENERATED_HASH_MISMATCH,
                      "minecraft-render-context-r78|X:\\missing\\slinkywinky_jni.dll",1,1);
            System.out.println("R80_CLEAN_ORIGINAL_SHA_REFUSAL_RELEASES_ADMISSION_PASS");
            caseUncertain(gate,Mode.REMOVE_FALSE,"minecraft-render-callback-r75");
            caseUncertain(gate,Mode.REMOVE_THROWS,"minecraft-render-callback-r75");
            caseUncertain(gate,Mode.ADD_THROWS,"minecraft-render-callback-r75");
            caseUncertain(gate,Mode.REMOVE_FALSE,"minecraft-render-preflight");
            caseUncertain(gate,Mode.REMOVE_THROWS,"minecraft-render-preflight");
            caseUncertain(gate,Mode.ADD_THROWS,"minecraft-render-preflight");
            caseUncertain(gate,Mode.REMOVE_FALSE,
                          "minecraft-render-context-r78|X:\\missing\\slinkywinky_jni.dll");
            System.out.println("R80_UNCERTAIN_PASSIVE_INSPECTOR_RETAINS_HOOK_GATE_PASS");
            System.out.println("R80_R73_R75_R78_PREWEAVE_FAILURE_ISOLATION_PASS");
            caseUnexpectedVmError(gate);
            System.out.println("R83_R73_VM_ERROR_PRESERVES_ADMISSION_PASS");
            caseRestoreSkippedOnUncertainWeaverRemoval();
            System.out.println("R85_UNCONFIRMED_WEAVE_REMOVAL_SKIPS_RETRANSFORM_PASS");
            System.out.println("R85_CONFIRMED_REMOVAL_PERMITS_PINNED_RESTORE_CHECK_PASS");
            System.out.println("R80_GENERATED_CLASSES_ONLY_NO_MINECRAFT_OR_GRAPHICS");
        } finally {
            // Isolated smoke JVM only; reset fake gate for other tests.
            gate.set(false);
        }
    }
    private RenderPreweaveAdmissionSmoke(){}
}
