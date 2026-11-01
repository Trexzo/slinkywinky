package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.ObservedState;

/** Deterministic read-only mapped field and state-transition assertions. */
public final class ObservationSmoke {
    private static void check(boolean ok,String message) {
        if(!ok)throw new AssertionError(message);
    }
    public static void main(String[] args) throws Exception {
        AttachTarget minecraft=AttachTarget.getMinecraft();
        ObservedState.admit(AttachTarget.class);
        ObservedState.Frame first=ObservedState.sample();
        check(first.flags==7,"Expected world, player and target");
        check(first.xyz[0]==1.25 && first.xyz[2]==2.0,"Player position");
        check(Math.abs(first.xyz[1]-(64+1.62))<.001,"Eye height was not included");
        check(first.xyz[3]==4.5 && first.xyz[4]==65 && first.xyz[5]==-2,"Target position");
        check(first.angles[0]==40 && first.angles[1]==-8,"Camera angles");
        minecraft.objectMouseOver.entityHit=null;
        ObservedState.Frame noTarget=ObservedState.sample();
        check(noTarget.flags==3 && noTarget.generation==first.generation,
              "Target loss cannot create a new player session");
        minecraft.theWorld=null; minecraft.thePlayer=null;
        ObservedState.Frame menu=ObservedState.sample();
        check(menu.flags==0 && menu.generation>first.generation,"Disconnect generation");
        minecraft.theWorld=new Object();minecraft.thePlayer=new AttachTarget.DummyPlayer();
        ObservedState.Frame reentry=ObservedState.sample();
        check(reentry.flags==3 && reentry.generation>menu.generation,
              "World reentry must advance generation");
        minecraft.theWorld=null;
        boolean inconsistent=false;
        try { ObservedState.sample(); }
        catch(IllegalStateException expected){inconsistent=true;}
        check(inconsistent,"Inconsistent player without world must be rejected");
        minecraft.theWorld=new Object();
        minecraft.thePlayer.posX=Double.NaN;
        boolean refused=false;
        try { ObservedState.sample(); }
        catch(IllegalStateException expected){refused=true;}
        check(refused,"Nonfinite player location not rejected");
        ObservedState.release();
        boolean rejectedAfterRelease=false;
        try { ObservedState.sample(); }
        catch(IllegalStateException expected) {rejectedAfterRelease=true;}
        check(rejectedAfterRelease,"Released sampler must not retain an admitted world");
        minecraft.thePlayer.posX=1.25;
        ObservedState.admit(AttachTarget.class);
        check(ObservedState.sample().flags==3,"Sampler must cleanly restart after release");
        ObservedState.release();
        System.out.println("OBSERVATION_FIELDS_AND_TRANSITIONS_PASS");
    }
}
