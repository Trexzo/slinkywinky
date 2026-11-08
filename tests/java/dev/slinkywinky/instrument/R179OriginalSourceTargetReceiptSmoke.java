package dev.slinkywinky.instrument;

/** R179 disposable Java 8 source-slider vs original-WGL hover receipt tests. */
public final class R179OriginalSourceTargetReceiptSmoke {
    private static void require(boolean yes,String why){
        if(!yes)throw new AssertionError("R179 "+why);
    }
    private static boolean ok(int[] target,int[] physical){
        return RendererWglEvidence.validOriginalSourceTargetEvidence(
            target,physical,64);
    }
    public static void main(String[] args){
        require(args.length==0,"No runtime handles accepted");
        int[] background={64,63,0,0,0};
        require(ok(new int[]{64,60,0},background),
                "Original background source-visible session refused");
        int[] active={64,63,4,3,1};
        require(ok(new int[]{64,60,1},active),
                "Exact source hit within real physical hover counts refused");
        require(ok(new int[]{64,60,3},active),
                "Valid source foreground hit upper bound refused");
        require(!ok(null,background),"Missing native source receipt admitted");
        require(!ok(new int[]{64,60},background),"Truncated native array admitted");
        require(!ok(new int[]{64,60,0,0},background),
                "Additional native source fields admitted");
        require(!ok(new int[]{63,60,0},background),
                "Native source frame count drift admitted");
        require(!ok(new int[]{64,59,0},background),
                "Source geometry missing on one draw was admitted");
        require(!ok(new int[]{64,61,0},background),
                "Overlong source-visible schedule was admitted");
        require(!ok(new int[]{64,60,-1},background),
                "Negative exact slider hover hit admitted");
        require(!ok(new int[]{64,60,1},background),
                "Background sample forged a source hit");
        require(!ok(new int[]{64,60,4},active),
                "More exact source hits than generic numeric hits accepted");
        require(!ok(new int[]{64,60,60},active),
                "Impossible source hit count exceeded physical source");
        require(!ok(new int[]{64,60,Integer.MAX_VALUE},active),
                "Java source receipt overflow accepted");
        require(!ok(new int[]{64,60,1},new int[]{64,62,4,3,1}),
                "Wrong R176 hover-sample horizon accepted");
        require(!ok(new int[]{64,60,1},new int[]{64,63,4,0,1}),
                "Source hit without a numeric control hit admitted");
        require(!ok(new int[]{64,60,1},new int[]{64,63,0,1,0}),
                "Numeric hit without actual foreground admitted");
        require(!RendererWglEvidence.validOriginalSourceTargetEvidence(
                     new int[]{64,60,0},background,63),
                "Wrong caller draw horizon accepted");
        System.out.println("R179_JAVA8_EXACT_SOURCE_SLIDER_NATIVE_RECEIPT_PASS");
        System.out.println("R179_MATCHED_R176_FOREGROUND_NUMERIC_NEGATIVES_PASS");
        System.out.println("R179_SOURCE_GEOMETRY_NO_GUI_CLICK_PERMISSION");
    }
}
