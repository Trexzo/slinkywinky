package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189CombatClock;
import dev.slinkywinky.instrument.SourceAttackTickFence;
import java.lang.reflect.Field;
import java.lang.reflect.Constructor;

/** R360 true original-name 1.8.9 JVM clock sampling, no game/packet action. */
public final class Vanilla189CombatClockSmoke {
    static void yes(boolean condition,String name){
        if(!condition)throw new AssertionError(name);
    }
    static Object fixture(String n)throws Exception{
        Constructor<?> c=Class.forName(n).getDeclaredConstructor();
        c.setAccessible(true);return c.newInstance();
    }
    static void set(Object value,String ownerName,String name,int n)throws Exception{
        for(Class<?> c=value.getClass();c!=null;c=c.getSuperclass()){
            if(c.getName().equals(ownerName)){
                Field f=c.getDeclaredField(name);f.setAccessible(true);
                f.setInt(value,n);return;
            }
        }
        throw new AssertionError("Fixture owner "+ownerName+" missing");
    }
    static Vanilla189CombatClock.Sample read(Object p){
        try{return Vanilla189CombatClock.read(p);}
        catch(ReflectiveOperationException bad){throw new IllegalStateException(bad);}
    }
    static void rejects(Runnable attempt,String why){
        boolean refused=false;
        try{attempt.run();}
        catch(IllegalStateException|IllegalArgumentException ex){refused=true;}
        yes(refused,why);
    }
    public static void main(String[] args)throws Exception{
        Object player=fixture("pr"),ray=fixture("bex");
        set(player,"pk","W",17);
        set(player,"pr","bl",16);
        yes(read(null)==null,"No player cannot create an attack phase");
        Vanilla189CombatClock.Sample original=read(player);
        yes(original.playerTicks==17&&original.lastAttackerTime==16,
            "Source Entity pk.W and LivingBase pr.bl independently sampled");
        SourceAttackTickFence fence=new SourceAttackTickFence();
        yes(!fence.observe(1,original),"Attach first sample never fakes attack");
        set(player,"pk","W",18);
        set(player,"pr","bl",18);
        yes(fence.observe(1,read(player)),
            "Observed same-tick changed timestamp admits one candidate e15");
        yes(!fence.observe(1,read(player)),"Duplicate game tick cannot repeat attack");
        set(player,"pk","W",19);
        yes(!fence.observe(1,read(player)),"Stale attacker timestamp denied");
        set(player,"pk","W",20);
        set(player,"pr","bl",20);
        yes(fence.observe(1,read(player)),"Second fresh same-tick attack admitted");
        yes(!fence.observe(2,read(player)),"New world generation cannot replay prior attack");
        set(player,"pk","W",21);
        yes(!fence.observe(2,read(player)),"World switch needs new attack");
        set(player,"pk","W",22);
        set(player,"pr","bl",22);
        yes(fence.observe(2,read(player)),"Fresh attack in new world admitted");
        fence.reset();
        yes(!fence.observe(2,read(player)),"Manual reset requires re-baseline");
        set(player,"pk","W",23);
        set(player,"pr","bl",24);
        rejects(()->read(player),"Attack timestamp ahead of player refused");
        set(player,"pr","bl",22);
        yes(!fence.observe(2,read(player)),"No attack after clock repair");
        set(player,"pk","W",22);
        rejects(()->fence.observe(2,read(player)),"Clock rollback denied");
        set(player,"pk","W",23);
        set(player,"pr","bl",-1);
        rejects(()->read(player),"Negative last attack invalid");
        set(player,"pr","bl",22);
        yes(!fence.observe(2,read(player)),"Denied samples cannot rearm");
        fence.close();
        rejects(()->fence.observe(2,read(player)),"Closed fence denied");
        Vanilla189CombatClock.release();
        yes(read(ray).playerTicks==18,
            "Real player subtype inherits exact Entity and LivingBase fields");
        Vanilla189CombatClock.release();
        System.out.println("R360_ORIGINAL_VANILLA_PLAYER_CLOCK_AND_ONESHOT_E15_PROXY_PASS");
    }
}
