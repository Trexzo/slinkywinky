package dev.slinkywinky.instrument;

/** R182: original-JVM read-only focus chronological receipt format tests. */
public final class R182OriginalFocusCycleReceiptSmoke {
    private static void check(boolean condition,String why) {
        if(!condition)throw new AssertionError("R182 "+why);
    }
    private static boolean valid(int[] cycle,int[] source,int[] hover) {
        return RendererWglEvidence.validRetainedFocusCycleEvidence(
            cycle,source,hover,64);
    }
    public static void main(String[] args) {
        check(args.length==0,"No original game inputs or handles accepted");
        final int[] background={64,63,0,0,0};
        final int[] clearSource={64,60,0};
        check(valid(new int[]{0,0,0},clearSource,background),
              "Background-only read-only evidence should remain valid");
        check(!valid(new int[]{1,1,1},clearSource,background),
              "Forged focus reacquisition admitted without foreground");
        final int[] hover={64,63,6,4,1};
        final int[] source={64,60,3};
        check(valid(new int[]{0,0,0},source,hover),
              "An absent focus cycle must not pretend success");
        check(valid(new int[]{1,0,0},source,hover),
              "An observed foreground loss without regain should be allowed");
        check(valid(new int[]{1,1,1},source,hover),
              "Verified loss, regain, post-source hit rejected");
        check(valid(new int[]{2,2,3},source,hover),
              "Repeated exact source focus cycles rejected");
        check(!valid(null,source,hover),"Null focus array accepted");
        check(!valid(new int[]{0,0},source,hover),"Short focus array accepted");
        check(!valid(new int[]{0,0,0,0},source,hover),"Extra focus field accepted");
        check(!valid(new int[]{-1,0,0},source,hover),"Negative loss accepted");
        check(!valid(new int[]{1,-1,0},source,hover),"Negative regain accepted");
        check(!valid(new int[]{1,1,-1},source,hover),"Negative post-hit accepted");
        check(!valid(new int[]{0,1,0},source,hover),"Regain without loss accepted");
        check(!valid(new int[]{1,0,1},source,hover),"Hit without regain accepted");
        check(!valid(new int[]{1,1,4},source,hover),"Post-hit greater than source accepted");
        check(!valid(new int[]{7,1,1},source,hover),
              "More focus losses than actual foreground samples accepted");
        check(!valid(new int[]{33,32,1},
                     new int[]{64,60,3},
                     new int[]{64,63,63,3,0}),
              "Too many transitions for the original 63 samples accepted");
        check(!valid(new int[]{1,1,1},new int[]{64,60,5},hover),
              "Source counts greater than numeric hit evidence accepted");
        check(!valid(new int[]{1,1,1},source,background),
              "Focus cycle outside original foreground source admitted");
        check(!RendererWglEvidence.validRetainedFocusCycleEvidence(
                   new int[]{1,1,1},source,hover,63),
              "Wrong frame horizon accepted");
        System.out.println("R182_JAVA8_ORIGINAL_FOCUS_CYCLE_RECEIPT_NEGATIVES_PASS");
        System.out.println("R182_NATIVE_COUNTER_SHAPE_NO_HARDWARE_CLAIM_PASS");
        System.out.println("R182_SOURCE_FOCUS_OBSERVATION_NEVER_ENABLES_CLICKS");
    }
}
