package dev.slinkywinky;
/** R349 value-only original candidate reach geometry. No Minecraft state or actions. */
public final class SourceBacktrackReachEvidence {
    private static final double SOURCE_THRESHOLD=0.01d;
    private SourceBacktrackReachEvidence() {}
    public static boolean currentIsCloserThanShown(
            double eyeX,double eyeY,double eyeZ,
            double currentX,double currentY,double currentZ,
            double shownX,double shownY,double shownZ) {
        final double[] values={eyeX,eyeY,eyeZ,currentX,currentY,currentZ,shownX,shownY,shownZ};
        for(double value:values)
            if(!Double.isFinite(value)||Math.abs(value)>30000000.0d)return false;
        final double dx=currentX-eyeX,dy=currentY-eyeY,dz=currentZ-eyeZ;
        final double sx=shownX-eyeX,sy=shownY-eyeY,sz=shownZ-eyeZ;
        final double current=Math.sqrt(dx*dx+dy*dy+dz*dz);
        final double shown=Math.sqrt(sx*sx+sy*sy+sz*sz);
        return Double.isFinite(current)&&Double.isFinite(shown)&&
               shown-current>SOURCE_THRESHOLD;
    }
    /**
     * Original JNI IDs encode variant=0 as the 1.7.10 path (no BlockPos),
     * and variant=1 as the 1.8.9 path. The candidate Backtrack callback
     * only calls vec3_add_eye_height_sneak for variant=0; that candidate
     * helper's scalar is -1.62 (confirmed in private rdata.s). In 1.8.9,
     * the input to entity_reach_check is the raw player posY, NOT Minecraft
     * getEyeHeight() or an interpolated camera viewpoint.
     *
     * This reproduces the candidate code path, not guaranteed binary parity.
     */
    public static double candidateEyeY(int candidateVariant,double playerPositionY){
        if(candidateVariant<0||candidateVariant>1||
           !Double.isFinite(playerPositionY)||
           Math.abs(playerPositionY)>30000000.0d)
            throw new IllegalArgumentException("Invalid candidate variant/position");
        final double y=playerPositionY+(candidateVariant==0?-1.62d:0.0d);
        if(!Double.isFinite(y))throw new IllegalArgumentException("Bad source eye Y");
        return y;
    }

    /** Conservative recent-attack snapshot, not an original attack event. */
    public static boolean observedRecentAttack(int currentTick,int lastAttackerTick){
        return lastAttackerTick>0&&currentTick>=lastAttackerTick&&
               currentTick-lastAttackerTick<=20;
    }
}
