package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.InstrumentationProbe;
import net.minecraft.launchwrapper.LaunchClassLoader;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.lang.reflect.Method;

/** Verifies that preflight resolves the same agent class from a child-first loader. */
public final class ClassLoaderSmoke {
    public static void main(String[] args) throws Exception {
        LaunchClassLoader loader = new LaunchClassLoader(ClassLoaderSmoke.class.getClassLoader());
        String resource = "dev/slinkywinky/attach/AttachTarget.class";
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (InputStream in = ClassLoaderSmoke.class.getClassLoader().getResourceAsStream(resource)) {
            if (in == null) throw new AssertionError("Missing test target bytes");
            byte[] buf = new byte[4096];
            int count;
            while ((count = in.read(buf)) != -1) bytes.write(buf, 0, count);
        }
        Class<?> target = loader.defineTarget("dev.slinkywinky.attach.AttachTarget",
                                              bytes.toByteArray());
        boolean initiallyInvisible = false;
        try { Class.forName(InstrumentationProbe.class.getName(), false, loader); }
        catch (ClassNotFoundException expected) { initiallyInvisible = true; }
        if (!initiallyInvisible) throw new AssertionError("No simulated visibility failure");
        Method verify = InstrumentationProbe.class.getDeclaredMethod(
            "verifyCallbackVisibility", Class.class);
        verify.setAccessible(true);
        verify.invoke(null, target);
        Class<?> resolved = Class.forName(InstrumentationProbe.class.getName(), false, loader);
        if (resolved != InstrumentationProbe.class)
            throw new AssertionError("Unexpected duplicated agent callback class");
        System.out.println("CLASSLOADER_DELEGATION_PASS");
    }
}
