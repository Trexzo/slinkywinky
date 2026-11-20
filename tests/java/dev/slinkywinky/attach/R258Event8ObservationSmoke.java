package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.ObservedState;
import dev.slinkywinky.instrument.R258Event8ReadOnlyObservation;
import java.lang.reflect.Constructor;
import java.util.concurrent.atomic.AtomicBoolean;

/** R258: use the existing read-only Java8 Minecraft-field sampler on
 * a disposal-only mapped fixture, never invoke game actions.
 */
public final class R258Event8ObservationSmoke {
    private static void check(boolean condition,String why){
        if(!condition)throw new AssertionError("R258: "+why);
    }
    private static ObservedState.Frame forged(
            long time,long generation,int flags,double[] xyz,float[] camera)
            throws Exception {
        Constructor<ObservedState.Frame> ctor=
            ObservedState.Frame.class.getDeclaredConstructor(
                long.class,long.class,int.class,double[].class,float[].class);
        ctor.setAccessible(true);
        return ctor.newInstance(time,generation,flags,xyz,camera);
    }
    private static void refuse(R258Event8ReadOnlyObservation view,
                               ObservedState.Frame frame) {
        boolean denied=false;
        try {view.capture(frame);}
        catch(IllegalArgumentException expected){denied=true;}
        check(denied,"invalid frame should be rejected");
    }
    public static void main(String[] args) throws Exception {
        check(args.length==0,"disposable mapped fixture only");
        AttachTarget mc=AttachTarget.getMinecraft();
        ObservedState.release();
        // Set a stable initial world/player/target. Neither the
        // test nor the R258 bridge modifies their gameplay state.
        mc.theWorld=new Object();
        mc.thePlayer=new AttachTarget.DummyPlayer();
        mc.objectMouseOver.entityHit=new AttachTarget.DummyEntity();
        ObservedState.admit(AttachTarget.class);
        final R258Event8ReadOnlyObservation view=
            new R258Event8ReadOnlyObservation();
        ObservedState.Frame first=ObservedState.sample();
        final R258Event8ReadOnlyObservation.Receipt a=view.capture(first);
        check(a.sequence==1 && a.flags==7 && a.worldAndPlayer() &&
              a.targetSelected() && !a.hasAllSourceEvent8Inputs(),"first mapped frame");
        check(a.eyeX==1.25 && a.eyeZ==2.0 && a.targetX==4.5,
              "exact source-sampled geometry retained in typed readonly copy");
        mc.objectMouseOver.entityHit=null;
        final R258Event8ReadOnlyObservation.Receipt b=
            view.capture(ObservedState.sample());
        check(b.sequence==2 && b.flags==3 && b.generation==a.generation,
              "target loss should not simulate a new world");
        mc.theWorld=null;mc.thePlayer=null;
        final R258Event8ReadOnlyObservation.Receipt c=
            view.capture(ObservedState.sample());
        check(c.sequence==3 && c.flags==0 && c.generation>a.generation,
              "disconnect generation");
        mc.theWorld=new Object();mc.thePlayer=new AttachTarget.DummyPlayer();
        final R258Event8ReadOnlyObservation.Receipt d=
            view.capture(ObservedState.sample());
        check(d.sequence==4 && d.flags==3 && d.generation>c.generation,
              "reentry generation");
        refuse(view,forged(d.monotonicMs,d.generation,2,
                           new double[6],new float[2]));
        refuse(view,forged(d.monotonicMs,d.generation,4,
                           new double[6],new float[2]));
        refuse(view,forged(d.monotonicMs,d.generation-1,3,
                           new double[6],new float[2]));
        refuse(view,forged(d.monotonicMs,d.generation,3,
                           new double[]{Double.NaN,0,0,0,0,0},new float[2]));
        refuse(view,forged(d.monotonicMs,d.generation,3,
                           new double[6],new float[]{Float.POSITIVE_INFINITY,0}));
        check(view.accepted()==4,"invalid data consumed source sequence");

        final AtomicBoolean rejected=new AtomicBoolean(false);
        Thread foreign=new Thread(new Runnable() {
            @Override public void run() {
                try {view.capture(first);}
                catch(IllegalStateException expected) {rejected.set(true);}
            }
        },"r258-foreign-owner");
        foreign.start();foreign.join(5000);
        check(!foreign.isAlive() && rejected.get(),"foreign sampling admitted");
        for(R258Event8ReadOnlyObservation.Receipt receipt:
            new R258Event8ReadOnlyObservation.Receipt[]{a,b,c,d}){
            // ABI strings intentionally contain no locations, entity IDs,
            // account information, module settings or output actions.
            System.out.println(receipt.envelope());
        }
        check(view.accepted()==4,"wrong event-8 source total");
        view.close();
        boolean closedDenied=false;
        try {view.capture(first);}
        catch(IllegalStateException expected){closedDenied=true;}
        check(closedDenied && view.accepted()!=4,
              "closed owner accepted stale data");
        ObservedState.release();
        System.out.println("R258_JAVA8_MAPPED_SOURCE_OBSERVATION_AND_LIFECYCLE_PASS");
        System.out.println("R258_NO_REACH_HITBOX_GHOSTHAND_INPUT_ASSUMPTIONS_PASS");
    }
    private R258Event8ObservationSmoke(){}
}
