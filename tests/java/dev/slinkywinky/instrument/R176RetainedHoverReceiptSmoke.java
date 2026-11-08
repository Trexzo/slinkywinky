package dev.slinkywinky.instrument;

/**
 * R176: source-limited Java 8 hover-counter receipt format checks.
 * No JNI is loaded here, no Minecraft process is attached to.
 */
public final class R176RetainedHoverReceiptSmoke {
    private static void check(boolean ok,String message) {
        if(!ok)throw new AssertionError("R176 "+message);
    }
    private static boolean accepts(int[] counters,int frames) {
        return RendererWglEvidence.validRetainedHoverEvidence(
            counters,frames);
    }
    public static void main(String[] args) {
        check(args.length==0,"Test cannot accept arbitrary process IDs");
        check(accepts(new int[]{64,63,0,0,0},64),
              "Original background-only valid counters refused");
        check(accepts(new int[]{64,63,63,32,31},64),
              "Disjoint numeric and bind samples refused");
        check(accepts(new int[]{64,63,7,2,3},64),
              "Original foreground partial-hover counts refused");
        check(!accepts(null,64),"Missing native hover receipt accepted");
        check(!accepts(new int[4],64),"Truncated native counters accepted");
        check(!accepts(new int[6],64),"Extra native counters accepted");
        check(!accepts(new int[]{63,63,0,0,0},64),
              "Counter's WGL frame identity mismatch accepted");
        check(!accepts(new int[]{64,62,0,0,0},64),
              "Dropped native hover observation accepted");
        check(!accepts(new int[]{64,64,0,0,0},64),
              "Duplicate hover sample accepted");
        check(!accepts(new int[]{64,63,64,0,0},64),
              "Foreground more than hover samples accepted");
        check(!accepts(new int[]{64,63,-1,0,0},64),
              "Negative foreground accepted");
        check(!accepts(new int[]{64,63,3,4,0},64),
              "Numeric hits more than foreground accepted");
        check(!accepts(new int[]{64,63,3,0,4},64),
              "Bind hits more than foreground accepted");
        check(!accepts(new int[]{64,63,3,2,2},64),
              "Double-counting one foreground sample accepted");
        check(!accepts(new int[]{64,63,3,-1,0},64),
              "Negative numeric hits accepted");
        check(!accepts(new int[]{64,63,3,0,-1},64),
              "Negative bind hits accepted");
        check(!accepts(new int[]{1,0,0,0,0},0),
              "Invalid game frame horizon accepted");
        check(!accepts(new int[]{1,0,0,0,0},2049),
              "Unbounded game frame horizon accepted");
        check(!accepts(new int[]{64,63,3,Integer.MAX_VALUE,
                Integer.MAX_VALUE},64),
              "Integer overflow disguised as approved hover");
        System.out.println("R176_JAVA8_EXACT_WGL_HOVER_COUNTER_FORMAT_PASS");
        System.out.println("R176_COUNTER_NEGATIVES_AND_NO_CLICK_AUTHORIZATION_PASS");
        System.out.println("R176_STATIC_RECEIPT_NOT_PHYSICAL_TOOLTIP_PROOF");
    }
}
