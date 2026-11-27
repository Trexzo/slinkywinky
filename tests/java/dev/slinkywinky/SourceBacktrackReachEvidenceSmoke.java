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
        // R350: source candidate variant=1 (1.8.9) does NOT add eye height.
        final double vanilla189EyeY=
            SourceBacktrackReachEvidence.candidateEyeY(1,0d);
        final double oldIncorrectEyeY=1.62d;
        check(vanilla189EyeY==0d,"1.8.9 source raw posY");
        check(SourceBacktrackReachEvidence.candidateEyeY(1,63d)==63d,
              "1.8.9 source Y invariant under translation");
        check(Math.abs(SourceBacktrackReachEvidence.candidateEyeY(0,63d)-61.38d)<0.000001d,
              "1.7.10 candidate uses -1.62 scalar, never 1.8.9 height");
        check(!SourceBacktrackReachEvidence.currentIsCloserThanShown(
            0,vanilla189EyeY,0,2,1.62,0,2.012,1.62,0),
            "1.8.9 source threshold refuses 0.0093 distance advantage");
        check(SourceBacktrackReachEvidence.currentIsCloserThanShown(
            0,oldIncorrectEyeY,0,2,1.62,0,2.012,1.62,0),
            "R349 old +1.62 camera eye incorrectly allowed same scene");
        boolean refusedVariant=false;
        try{SourceBacktrackReachEvidence.candidateEyeY(2,0);}
        catch(IllegalArgumentException expected){refusedVariant=true;}
        check(refusedVariant,"unknown candidate variant must be rejected");
        refusedVariant=false;
        try{SourceBacktrackReachEvidence.candidateEyeY(1,Double.NaN);}
        catch(IllegalArgumentException expected){refusedVariant=true;}
        check(refusedVariant,"nonfinite player eye observation rejected");
        System.out.println("R350_SOURCE_VARIANT_EYE_PARITY_PASS");
        System.out.println("R349_SOURCE_BACKTRACK_REACH_GEOMETRY_PASS");
    }
}
