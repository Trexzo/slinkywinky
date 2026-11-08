package dev.slinkywinky.instrument;

/**
 * R172: disposable Java8 read-only physical pointer receipt checks.
 * Does not load Minecraft, JNI, a real OS mouse or any class transformer.
 */
public final class R172OriginalPointerReceiptSmoke {
    private static void check(boolean accepted,String why) {
        if(!accepted)throw new AssertionError("R172 "+why);
    }
    private static boolean valid(int[] p,int width,int height) {
        return RendererWglEvidence.validOriginalPointerReceipt(
            p,width,height);
    }
    public static void main(String[] args) {
        check(args.length==0,"Input test cannot accept external runtime handles");
        check(valid(new int[]{0,0,1,0},676,630),"Inside top-left denied");
        check(valid(new int[]{675,629,1,31},676,630),"Inside bottom-right denied");
        check(valid(new int[]{-1,15,0,0},676,630),"Outside left denied");
        check(valid(new int[]{676,16,0,3},676,630),"Outside right denied");
        check(valid(new int[]{16,630,0,4},676,630),"Outside bottom denied");
        check(valid(new int[]{160,-1,0,0},676,630),"Outside top denied");
        check(!valid(null,676,630),"Null JNI sample admitted");
        check(!valid(new int[]{0,0,1},676,630),"Short JNI receipt admitted");
        check(!valid(new int[]{0,0,1,0,0},676,630),"Oversized JNI receipt admitted");
        check(!valid(new int[]{0,0,0,0},676,630),"Inside lied outside");
        check(!valid(new int[]{676,1,1,0},676,630),"Outside lied inside");
        check(!valid(new int[]{0,0,2,0},676,630),"Non-boolean inside flag accepted");
        check(!valid(new int[]{0,0,1,-1},676,630),"Negative button mask");
        check(!valid(new int[]{0,0,1,32},676,630),"Excess button bits accepted");
        check(!valid(new int[]{Integer.MIN_VALUE,0,0,0},676,630),"Impossible cursor x accepted");
        check(!valid(new int[]{0,Integer.MAX_VALUE,0,0},676,630),"Impossible cursor y accepted");
        check(!valid(new int[]{0,0,1,0},319,630),"Small viewport width admitted");
        check(!valid(new int[]{0,0,1,0},676,239),"Small viewport height admitted");
        check(!valid(new int[]{0,0,1,0},8193,630),"Wide viewport admitted");
        check(!valid(new int[]{0,0,1,0},676,8193),"Tall viewport admitted");
        check(!valid(new int[]{0,0,1,0},0,630),"Missing viewport admitted");
        // Evidence cannot be used as permission for gameplay input.
        System.out.println("R172_JAVA8_PHYSICAL_POINTER_RECEIPT_NEGATIVE_CONTROLS_PASS");
        System.out.println("R172_SOURCE_WINDOW_EVIDENCE_NOT_EXCLUSIVE_GAME_INPUT");
        System.out.println("R172_NO_GAME_ATTACH_NO_MOUSE_INTERCEPTION");
    }
}
