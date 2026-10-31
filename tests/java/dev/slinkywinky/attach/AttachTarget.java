package dev.slinkywinky.attach;
public final class AttachTarget {
    public void tick() { /* Called repeatedly as a safe instrumentation target. */ }
    public static void main(String[] unused) throws Exception {
        System.out.println("SLINKYWINKY_ATTACH_TARGET_READY");
        System.out.flush();
        AttachTarget instance = new AttachTarget();
        for (int i = 0; i < 1000; ++i) {
            instance.tick();
            Thread.sleep(30L);
        }
    }
}
