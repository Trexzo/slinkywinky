package dev.slinkywinky;

/** Java 8 -> real JNI shared-library smoke for all four source modules. */
public final class SourceRuntimePreviewSmoke {
    static void ok(boolean b,String why){if(!b)throw new AssertionError(why);}
    static void eq(double actual,double expected,String why){
        ok(Math.abs(actual-expected)<0.0000001,why+" actual="+actual);
    }
    static void deny(Runnable task,String why){
        boolean rejected=false;
        try{task.run();}catch(IllegalStateException|IllegalArgumentException expected){
            rejected=true;
        }
        ok(rejected,why);
    }
    static int[] backtrack(){return new int[]{1,1,1,100,90,21,42,1,1,0,1,1};}
    static double[] backtrackFields(){return new double[]{9,0,120,3,0,6,10};}
    static int[] lag(){return new int[]{1,1,1,1,0,0,1,1,0,0,0,1};}
    static double[] lagFields(){return new double[]{9,11,10,0,0,120};}
    static int[] timer(){return new int[]{1,1,1,0,1,0,0,1,0,1,0,0,0,0,1};}
    static double[] timerFields(){return new double[]{9,0,1,5,3,2,0};}
    static double[] timerFrame(){return new double[]{
        0.01,0, 0,1.62,0, 0,0,0, 0,0,2.8, 0,0,2.8, 0,0,1,0,0.1,0
    };}
    static int[] inv(){return new int[]{1,1,0,0,0,1,0,0,0,1,0,1};}

    public static void main(String[] ignored)throws Exception{
        final SourceRuntimePreview s=new SourceRuntimePreview();
        final long w=100,p=200,g=1;
        SourceRuntimePreview.Receipt r=s.observe(w,p,g,1,0,backtrack(),backtrackFields());
        eq(r.at(3),1,"Backtrack tick passed through real JNI");
        r=s.observe(w,p,g,1,1,new int[]{0,1,0,1},new double[]{10});
        eq(r.at(3),1,"Backtrack packet held");
        r=s.observe(w,p,g,1,1,new int[]{1,2,21,1},new double[]{10});
        eq(r.at(3),2,"Matching target feedback passthrough");
        r=s.observe(w,p,g,1,1,new int[]{1,3,999,1},new double[]{10});
        eq(r.at(3),1,"Unrelated player feedback still held");
        r=s.observe(w,p,g,2,2,lag(),lagFields());
        eq(r.at(3),1,"Lag Range armed");
        r=s.observe(w,p,g,2,3,new int[]{1,1,1,1},new double[]{10,3,0,0});
        eq(r.at(3),1,"Lag Range packet delayed");
        r=s.observe(w,p,g,2,3,new int[]{1,2,1,1},new double[]{121,4,0,0});
        eq(r.at(3),0,"Lag Range delay expired");
        r=s.observe(w,p,g,3,4,timer(),timerFields());
        eq(r.at(2),1,"Timer Range native tick callback");
        r=s.observe(w,p,g,3,5,new int[]{1},timerFrame());
        eq(r.at(3),1,"Timer Range native raycast burst proposed");
        eq(r.at(4),1,"Timer Range freeze-before timer proposal");
        r=s.observe(w,p,g,4,6,inv(),new double[]{3});
        eq(r.at(2),1,"Inv Walk native tick callback");
        r=s.observe(w,p,g,4,7,new int[]{1},new double[]{});
        eq(r.at(3),1,"Silent inventory open consumed in native preview");
        r=s.observe(w,p,g,4,7,new int[]{2},new double[]{});
        eq(r.at(3),3,"Inventory click queued after open");
        r=s.observe(w,p,g,4,8,new int[]{1,1},new double[]{});
        eq(r.at(3),0,"Inventory replay guard prevents premature flush");
        s.observe(w,p,g,5,6,inv(),new double[]{3});
        r=s.observe(w,p,g,5,8,new int[]{1,1},new double[]{});
        eq(r.at(3),1,"Confirmed inventory flush");
        deny(()->s.observe(w,p,g,4,1,new int[]{0,4,0,1},new double[]{10}),
             "Stale native event rejected");
        deny(()->s.observe(101,p,g,5,1,new int[]{0,4,0,1},new double[]{10}),
             "Same-generation world swap rejected");
        deny(()->s.observe(w,p,g,5,5,new int[]{1},new double[]{1}),
             "Malformed Java to native frame rejected");
        deny(()->s.observe(w,p,g,5,2,lag(),
             new double[]{Double.NaN,11,10,0,0,120}),"NaN packet frame rejected");
        final boolean[] foreign={false};
        Thread other=new Thread(()->{
            try{s.observe(w,p,g,6,1,new int[]{0,5,0,1},new double[]{10});}
            catch(IllegalStateException expected){foreign[0]=true;}
        });
        other.start();other.join();
        ok(foreign[0],"Owner-thread gate");
        r=s.observe(101,201,g+1,0,1,new int[]{0,4,0,1},new double[]{10});
        eq(r.at(3),0,"Fresh generation released Backtrack hold");
        r=s.observe(101,201,g+1,0,7,new int[]{2},new double[]{});
        eq(r.at(3),0,"Fresh generation dropped prior inventory queue");
        deny(()->s.observe(w,p,g,9,1,new int[]{0,5,0,1},new double[]{10}),
             "Prior generation rejected");
        s.close();
        deny(()->s.observe(101,201,g+1,1,1,new int[]{0,5,0,1},
             new double[]{10}),"Closed preview rejects calls");
        System.out.println("R335_JAVA8_REAL_SOURCE_BRIDGE_JNI_PASS");
    }
}
