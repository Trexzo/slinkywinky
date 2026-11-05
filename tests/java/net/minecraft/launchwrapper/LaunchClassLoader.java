package net.minecraft.launchwrapper;

import java.util.HashSet;
import java.util.Set;

/** Minimal child-first simulation for class-loader admission regression tests. */
public final class LaunchClassLoader extends ClassLoader {
    private final Set<String> exclusions = new HashSet<String>();
    public LaunchClassLoader(ClassLoader parent) { super(parent); }
    public void addClassLoaderExclusion(String prefix) {
        exclusions.add(prefix);
    }
    public Class<?> defineTarget(String name, byte[] bytes) {
        return defineClass(name, bytes, 0, bytes.length);
    }
    @Override protected Class<?> loadClass(String name, boolean resolve)
            throws ClassNotFoundException {
        if (name.startsWith("dev.slinkywinky.instrument.")) {
            boolean allowed = false;
            for (String prefix : exclusions) if (name.startsWith(prefix)) allowed = true;
            if (!allowed) throw new ClassNotFoundException("Simulated child-first exclusion: " + name);
        }
        return super.loadClass(name, resolve);
    }
}
