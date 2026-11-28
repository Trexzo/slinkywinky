package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.ObservedState;

/** Deterministic read-only mapped field and state-transition assertions. */
public final class ObservationSmoke {
    public static class AlternateEntity extends AttachTarget.DummyEntity { }
    public static class AlternatePlayer extends AttachTarget.DummyPlayer { }
    private static long mappingBuilds() throws Exception {
        java.lang.reflect.Field counter=ObservedState.class.getDeclaredField("mappingBuilds");
        counter.setAccessible(true);
        return counter.getLong(null);
    }
    private static void check(boolean ok,String message) {
        if(!ok)throw new AssertionError(message);
    }
    public static void main(String[] args) throws Exception {
        AttachTarget minecraft=AttachTarget.getMinecraft();
        ObservedState.admit(AttachTarget.class);
        ObservedState.Frame first=ObservedState.sample();
        check(first.flags==7,"Expected world, player and target");
        check(first.worldToken>0&&first.playerToken>first.worldToken,
              "R371 game owner JNI identity must use distinct primitive tokens");
        check(first.xyz[0]==1.25 && first.xyz[2]==2.0,"Player position");
        check(Math.abs(first.xyz[1]-(64+1.62))<.001,"Eye height was not included");
        check(first.xyz[3]==4.5 && first.xyz[4]==65 && first.xyz[5]==-2,"Target position");
        check(first.angles[0]==40 && first.angles[1]==-8,"Camera angles");
        long firstBuilds=mappingBuilds();
        check(firstBuilds==3,"Expected exactly player, ray and target mappings");
        for(int n=0;n<200;n++) {
            ObservedState.Frame repeat=ObservedState.sample();
            check(repeat.flags==7 && repeat.generation==first.generation &&
                  repeat.worldToken==first.worldToken &&
                  repeat.playerToken==first.playerToken,
                  "Repeated observations must preserve JNI identity");
        }
        check(mappingBuilds()==firstBuilds,"Mappings must not rebind on each tick");
        minecraft.objectMouseOver.entityHit=new AlternateEntity();
        ObservedState.Frame changedTarget=ObservedState.sample();
        check(changedTarget.flags==7 && mappingBuilds()==firstBuilds+1,
              "Different target runtime class must trigger one rebind");
        minecraft.objectMouseOver.entityHit=null;
        ObservedState.Frame noTarget=ObservedState.sample();
        check(noTarget.flags==3 && noTarget.generation==first.generation &&
              noTarget.worldToken==first.worldToken &&
              noTarget.playerToken==first.playerToken,
              "Target loss cannot change JNI world/player identity");
        minecraft.thePlayer=new AlternatePlayer();
        ObservedState.Frame substitutedPlayer=ObservedState.sample();
        check(substitutedPlayer.flags==3 &&
              substitutedPlayer.generation>noTarget.generation &&
              substitutedPlayer.worldToken>noTarget.playerToken &&
              substitutedPlayer.playerToken>substitutedPlayer.worldToken &&
              mappingBuilds()==firstBuilds+2,
              "Player subclass change must rebuild binding and advance session");
        minecraft.theWorld=null; minecraft.thePlayer=null;
        ObservedState.Frame menu=ObservedState.sample();
        check(menu.flags==0 && menu.generation>first.generation &&
              menu.worldToken>substitutedPlayer.playerToken,
              "Disconnect must revoke old native source tokens");
        minecraft.theWorld=new Object();minecraft.thePlayer=new AttachTarget.DummyPlayer();
        ObservedState.Frame reentry=ObservedState.sample();
        check(reentry.flags==3 && reentry.generation>menu.generation &&
              reentry.worldToken>menu.playerToken,
              "World reentry must allocate previously unused JNI tokens");
        long lastGeneration=reentry.generation;
        minecraft.theWorld=null;
        boolean inconsistent=false;
        try { ObservedState.sample(); }
        catch(IllegalStateException expected){inconsistent=true;}
        check(inconsistent,"Inconsistent player without world must be rejected");
        minecraft.theWorld=new Object();
        ObservedState.Frame resumed=ObservedState.sample();
        check(resumed.generation==lastGeneration+1 &&
              resumed.worldToken>reentry.playerToken,
              "Rejected inconsistent snapshot cannot allocate native identity");
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
        ObservedState.Frame restarted=ObservedState.sample();
        check(restarted.flags==3&&
              restarted.worldToken>resumed.playerToken,
              "R371 reattach cannot reuse native source world/player IDs");
        ObservedState.release();
        check(mappingBuilds()==0,"Release must clear class-binding cache");
        System.out.println("OBSERVATION_CACHE_REBIND_AND_RELEASE_PASS");
        System.out.println("OBSERVATION_FIELDS_AND_TRANSITIONS_PASS");
    }
}
