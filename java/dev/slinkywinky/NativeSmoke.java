package dev.slinkywinky;

public final class NativeSmoke {
    public static void main(String[] args) {
        if (NativeEngine.abiVersion()!=1) throw new AssertionError("Wrong native ABI");
        try (NativeEngine engine = new NativeEngine(new int[] {1,2,3,4})) {
            final float[] config={1,12,1,4,15,15,90};
            float[] first=engine.step(100,1,true,false,false,true,0,0,
                                  new double[]{0,0,0},new double[]{2,0,0},config);
            if (first.length!=5 || first[0]!=1 || first[1]!=1 || Math.abs(first[3]+13.5f)>0.01f)
                throw new AssertionError("Unexpected C++ decision via JNI");
            float[] withoutTarget=engine.step(110,1,true,false,false,true,0,0,
                                  new double[]{0,0,0},null,config);
            if (withoutTarget[2]!=0) throw new AssertionError("Missing target incorrectly admitted");
            final boolean[] wrongThread={false};
            Thread worker=new Thread(() -> {
                try {
                    engine.step(115,1,true,false,false,true,0,0,
                                new double[]{0,0,0},null,config);
                } catch (IllegalStateException rejected) { wrongThread[0]=true; }
            });
            worker.start();
            try { worker.join(); } catch (InterruptedException interrupted) {
                Thread.currentThread().interrupt(); throw new AssertionError("Interrupted",interrupted);
            }
            if (!wrongThread[0]) throw new AssertionError("Foreign thread was not rejected");
            float[] second=engine.step(120,1,false,false,false,true,0,0,
                                  new double[]{0,0,0},new double[]{2,0,0},config);
            if (second[0]!=0 || second[1]!=0) throw new AssertionError("World leave was not gated");
        }
        System.out.println("PASS JNI Java 8-compatible native roundtrip and close");
    }
}
