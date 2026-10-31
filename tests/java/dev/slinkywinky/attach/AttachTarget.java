package dev.slinkywinky.attach;
/** Disposable mock of mapped 1.8.9 field names, not a Minecraft implementation. */
public final class AttachTarget {
    private static final AttachTarget INSTANCE=new AttachTarget();
    public static class DummyEntity {
        public double posX=4.5,posY=65,posZ=-2;
    }
    public static final class DummyPlayer extends DummyEntity {
        public float rotationYaw=40,rotationPitch=-8;
    }
    public static final class Hit {
        public DummyEntity entityHit=new DummyEntity();
    }
    public Object theWorld=new Object();
    public DummyPlayer thePlayer=new DummyPlayer();
    public Hit objectMouseOver=new Hit();
    public static AttachTarget getMinecraft(){return INSTANCE;}
    public void tick(){ }
    public static void main(String[] args) throws Exception {
        System.out.println("SLINKYWINKY_ATTACH_TARGET_READY");
        System.out.flush();
        for(int i=0;i<1000;i++){INSTANCE.tick();Thread.sleep(30L);}
    }
}
