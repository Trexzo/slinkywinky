package dev.slinkywinky;

/** Pure Java 8 R246 negative controls; does not launch Minecraft. */
public final class ClutchRayReceiptStatusSmoke {
    private static void check(boolean valid, String message) {
        if(!valid)throw new AssertionError(message);
    }
    private static void refused(Runnable action, String message) {
        try {
            action.run();
        } catch(IllegalStateException expected) {
            return;
        }
        throw new AssertionError("Accepted invalid R246 evidence: " + message);
    }
    private static String status(int samples,int plans,int aims,
                                 int priors,int rays,int retained) {
        return ClutchRayReceiptStatus.classify(
            samples,plans,aims,priors,rays,retained);
    }
    public static void main(String[] args) {
        check(ClutchRayReceiptStatus.INCONCLUSIVE.equals(
            status(20,0,0,0,0,0)),"empty world cannot prove real ray");
        check(ClutchRayReceiptStatus.INCONCLUSIVE.equals(
            status(20,19,19,19,0,0)),"proposals without ray are inconclusive");
        check(ClutchRayReceiptStatus.VERIFIED.equals(
            status(20,6,6,4,1,0)),"one real prior ray is sufficient");
        check(ClutchRayReceiptStatus.VERIFIED.equals(
            status(20,19,19,19,19,5)),"multiple verified rays");
        refused(()->status(19,10,10,1,1,0),"not 20 observations");
        refused(()->status(20,21,21,1,1,0),"too many plans");
        refused(()->status(20,3,2,1,1,0),"aim receipt mismatch");
        refused(()->status(20,1,1,0,1,0),"ray without prior");
        refused(()->status(20,1,1,1,2,0),"double ray per prior");
        refused(()->status(20,1,1,1,1,2),"retained without prior");
        refused(()->status(20,1,1,20,1,0),"prior on first tick");
        refused(()->status(20,-1,-1,1,1,0),"negative count");
        System.out.println("R246_CLUTCH_RAY_EVIDENCE_JAVA8_PASS");
    }
}
