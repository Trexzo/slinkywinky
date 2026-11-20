package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.ObservedState;
import dev.slinkywinky.instrument.R258Event8ReadOnlyObservation;
import java.lang.reflect.Field;
/** R259 disposable exact-named vanilla ABI, not Minecraft gameplay. */
public final class R259CombatMappingSmoke {
    static void ok(boolean value,String where){
        if(!value)throw new AssertionError("R259 "+where);
    }
    static R258Event8ReadOnlyObservation.Receipt read(
            R258Event8ReadOnlyObservation view) throws Exception{
        R258Event8ReadOnlyObservation.Receipt v=
            view.capture(ObservedState.sample());
        ok(!v.hasAllSourceEvent8Inputs(),"no event8 action authority");
        System.out.println(v.envelope());
        System.out.println(v.r259Evidence());
        return v;
    }
    public static void main(String[] args) throws Exception{
        ok(args.length==0,"disposable only");
        Class<?> minecraft=Class.forName("ave");
        Class<?> entity=Class.forName("pk");
        Class<?> living=Class.forName("pr");
        Object mc=minecraft.getMethod("A").invoke(null);
        Field world=minecraft.getField("f"),player=minecraft.getField("h"),
              ray=minecraft.getField("s");
        Field sprint=entity.getField("sprinting");
        Field attacked=living.getField("lastAttackerTime");
        Object first=living.newInstance();
        world.set(mc,new Object());player.set(mc,first);
        sprint.setBoolean(first,false);attacked.setInt(first,0);
        ObservedState.release();ObservedState.admit(minecraft);
        R258Event8ReadOnlyObservation source=
            new R258Event8ReadOnlyObservation();
        R258Event8ReadOnlyObservation.Receipt a=read(source);
        ok(a.flags==7 && a.sprintKnown && !a.sprinting &&
           a.lastAttackerTimeKnown && a.lastAttackerTime==0,
           "pk.aw pr.bg initial");
        sprint.setBoolean(first,true);attacked.setInt(first,24);
        R258Event8ReadOnlyObservation.Receipt b=read(source);
        ok(b.sprintKnown && b.sprinting && b.lastAttackerTimeKnown &&
           b.lastAttackerTime==24 && b.generation==a.generation,
           "cached exact methods");
        Object base=entity.newInstance();
        sprint.setBoolean(base,true);player.set(mc,base);
        R258Event8ReadOnlyObservation.Receipt c=read(source);
        ok(c.sprintKnown && c.sprinting && !c.lastAttackerTimeKnown &&
           c.generation>a.generation,"unknown pr method is not zero");
        world.set(mc,null);player.set(mc,null);
        R258Event8ReadOnlyObservation.Receipt d=read(source);
        ok(d.flags==0 && !d.sprintKnown && !d.lastAttackerTimeKnown,
           "disconnected evidence revoked");
        world.set(mc,new Object());
        player.set(mc,living.newInstance());ray.set(mc,null);
        R258Event8ReadOnlyObservation.Receipt e=read(source);
        ok(e.flags==3 && e.sprintKnown && e.lastAttackerTimeKnown &&
           !e.sprinting && e.lastAttackerTime==0 &&
           e.generation>d.generation,"new world source values");
        ok(source.accepted()==5,"exact source sequence");
        source.close();ObservedState.release();
        boolean closed=false;
        try{source.capture(ObservedState.sample());}
        catch(IllegalStateException expected){closed=true;}
        ok(closed,"closed sampler must reject");
        System.out.println("R259_JAVA8_EXACT_PK_AW_PR_BG_SOURCE_EVIDENCE_PASS");
        System.out.println("R259_INCOMING_ATTACKER_TIME_NOT_OUTGOING_HIT_PASS");
    }
}
