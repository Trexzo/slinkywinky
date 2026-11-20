package dev.slinkywinky;

/**
 * R246: evidence classifier for the opt-in Forge Clutch ray observation.
 * A completed candidate scan does not prove the prior-world ray path ran.
 * This class reports only diagnostic acceptance, never gameplay parity.
 */
public final class ClutchRayReceiptStatus {
    public static final String VERIFIED =
        "R246_REAL_WORLD_CLUTCH_PRIOR_RAY_READONLY_PASS";
    public static final String INCONCLUSIVE =
        "R246_REAL_WORLD_CLUTCH_PRIOR_RAY_INCONCLUSIVE";

    private ClutchRayReceiptStatus() { }

    public static String classify(int samples, int nativePlans,
                                  int aimReceipts, int priorRequests,
                                  int raycasts, int retainedPrior) {
        // Exactly 20 successful client-thread, same-world samples are
        // necessary before accepting the terminal diagnostic.
        if(samples != 20 ||
           nativePlans < 0 || nativePlans > samples ||
           aimReceipts != nativePlans ||
           priorRequests < 0 || priorRequests >= samples ||
           raycasts < 0 || raycasts > priorRequests ||
           retainedPrior < 0 || retainedPrior > priorRequests)
            throw new IllegalStateException("Inconsistent R246 Clutch evidence");
        // No prior ray means the R244/R245 path has not been exercised,
        // regardless of how many native candidate plans succeeded.
        return raycasts > 0 ? VERIFIED : INCONCLUSIVE;
    }
}
