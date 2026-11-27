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
    /** Conservative recent-attack snapshot, not an original attack event. */
    public static boolean observedRecentAttack(int currentTick,int lastAttackerTick){
        return lastAttackerTick>0&&currentTick>=lastAttackerTick&&
               currentTick-lastAttackerTick<=20;
    }
}
