package dev.slinkywinky;
public final class SourceBacktrackReachEvidenceSmoke {
    private static void check(boolean yes,String what){if(!yes)throw new AssertionError(what);}
    private static boolean reach(double current,double shown){
        return SourceBacktrackReachEvidence.currentIsCloserThanShown(
            0,1.62,0,current,1.62,0,shown,1.62,0);
    }
    public static void main(String[] args){
        check(reach(3,3.011),"strict candidate 0.01 distance delta");
        check(!reach(3,3.009),"below source threshold");
        check(!reach(3,3),"unchanged shown/current position");
        check(!reach(4,3),"current farther than shown");
        check(!reach(3,-3),"opposite direction equal range");
        check(SourceBacktrackReachEvidence.currentIsCloserThanShown(
            10,0,10,12,0,10,12.02,0,10),"translated origin");
        check(!SourceBacktrackReachEvidence.currentIsCloserThanShown(
            0,0,0,Double.NaN,0,0,4,0,0),"NaN refused");
        check(!SourceBacktrackReachEvidence.currentIsCloserThanShown(
            0,0,0,3,0,0,Double.POSITIVE_INFINITY,0,0),"infinity refused");
        check(!SourceBacktrackReachEvidence.currentIsCloserThanShown(
            0,0,0,3,0,0,30000001,0,0),"unbounded coordinates");
        check(SourceBacktrackReachEvidence.observedRecentAttack(100,80),"inclusive 20 tick gate");
        check(!SourceBacktrackReachEvidence.observedRecentAttack(100,79),"expired attack");
        check(!SourceBacktrackReachEvidence.observedRecentAttack(100,101),"future attack");
        check(!SourceBacktrackReachEvidence.observedRecentAttack(10,0),"unknown prior attack");
        check(!SourceBacktrackReachEvidence.observedRecentAttack(Integer.MAX_VALUE,1),"overflow safe");
        System.out.println("R349_SOURCE_BACKTRACK_REACH_GEOMETRY_PASS");
    }
}
