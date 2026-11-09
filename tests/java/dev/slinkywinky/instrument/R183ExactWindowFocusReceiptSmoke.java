package dev.slinkywinky.instrument;

/** R183 pure Java 8 counter-shape negatives; no Win32/JNI/game input. */
public final class R183ExactWindowFocusReceiptSmoke {
    private static void check(boolean yes,String why) {
        if(!yes)throw new AssertionError("R183 "+why);
    }
    private static boolean valid(int[] os,int[] source,int[] hover,int[] compat) {
        return RendererWglEvidence.validExactWindowFocusEvidence(
            os,source,hover,compat,64);
    }
    public static void main(String[] args) {
        check(args.length==0,"No external original-client handles permitted");
        final int[] background={64,63,0,0,0};
        final int[] emptySource={64,60,0};
        final int[] emptyCompat={0,0,0};
        check(valid(new int[]{0,63,0,0,0,0},emptySource,
                    background,emptyCompat),
              "Historical original all-background session was invalidated");
        check(!valid(new int[]{0,0,63,1,1,1},emptySource,
                     background,emptyCompat),
              "Refused HWND window falsely certified recovery");
        final int[] hover={64,63,4,3,0};
        final int[] source={64,60,2};
        final int[] compat={1,1,1};
        final int[] real={4,59,0,1,1,1};
        check(valid(real,source,hover,compat),
              "Valid exact OS loss/regain/source hover rejected");
        check(valid(new int[]{3,59,1,1,1,1},source,hover,compat),
              "Refused sample may exist without forging a focus cycle");
        check(!valid(null,source,hover,compat),"Missing focus evidence admitted");
        check(!valid(new int[]{4,59,0},source,hover,compat),
              "Truncated OS evidence admitted");
        check(!valid(new int[]{4,59,0,1,1,1,0},source,hover,compat),
              "Excessive OS evidence array admitted");
        check(!valid(new int[]{4,58,0,1,1,1},source,hover,compat),
              "Missing source render sample admitted");
        check(!valid(new int[]{4,60,0,1,1,1},source,hover,compat),
              "Excess source render sample admitted");
        check(!valid(new int[]{5,58,0,1,1,1},source,hover,compat),
              "More exact foreground than pointer observations admitted");
        check(!valid(new int[]{4,59,0,0,1,1},source,hover,compat),
              "Reacquisition without foreground loss admitted");
        check(!valid(new int[]{4,59,0,1,0,1},source,hover,compat),
              "Post hit without verified regain admitted");
        check(!valid(new int[]{4,59,0,1,1,3},source,hover,compat),
              "More post hits than source-exact hover admitted");
        check(!valid(new int[]{4,59,0,4,1,1},source,hover,compat),
              "Four losses but one regain without refusals admitted");
        check(!valid(new int[]{4,59,0,5,4,1},source,hover,compat),
              "More losses than foreground samples admitted");
        check(!valid(new int[]{4,59,0,1,1,-1},source,hover,compat),
              "Negative post hit admitted");
        check(!valid(new int[]{4,59,-1,1,1,1},source,hover,compat),
              "Negative refusal count admitted");
        check(!valid(new int[]{-1,64,0,1,1,1},source,hover,compat),
              "Negative actual foreground count admitted");
        check(!valid(new int[]{4,59,0,1,1,1},source,
                     new int[]{64,62,4,3,0},compat),
              "Wrong original hover horizon admitted");
        check(!valid(real,source,hover,new int[]{1,1,0}),
              "OS post-hit bypassed legacy accepted native source receipt");
        System.out.println("R183_JAVA8_EXACT_OS_WINDOW_RECEIPT_NEGATIVES_PASS");
        System.out.println("R183_WIN32_REFUSAL_NOT_BACKGROUND_PASS");
        System.out.println("R183_CLICK_INPUT_CAPTURE_REMAINS_DISABLED");
    }
}
