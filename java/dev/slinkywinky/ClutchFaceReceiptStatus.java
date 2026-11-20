package dev.slinkywinky;

/** R251: honest bounded read-only cursor-hit evidence classification. */
public final class ClutchFaceReceiptStatus {
    public static final String OBSERVED =
        "R251_CLUTCH_CURSOR_FACE_MATCH_OBSERVED_NOT_PLACEMENT";
    public static final String INCONCLUSIVE =
        "R251_CLUTCH_CURSOR_FACE_INCONCLUSIVE";
    private ClutchFaceReceiptStatus(){}
    public static String classify(int snapshots,int priorSamples,
                                  int hitBlocks,int matches) {
        if(snapshots!=20 || priorSamples<0 || priorSamples>=snapshots ||
           hitBlocks<0 || hitBlocks>priorSamples ||
           matches<0 || matches>hitBlocks)
            throw new IllegalStateException("Malformed R251 cursor evidence");
        return matches>0?OBSERVED:INCONCLUSIVE;
    }
}
