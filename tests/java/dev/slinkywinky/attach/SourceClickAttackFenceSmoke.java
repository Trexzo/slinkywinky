package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189CombatClock;
import dev.slinkywinky.instrument.SourceClickAttackFence;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.util.concurrent.atomic.AtomicBoolean;

/** JVM8 synthetic original-field clock, genuine click-return control. */
public final class SourceClickAttackFenceSmoke {
    private static void yes(boolean v,String s){if(!v)throw new AssertionError(s);}
    private static Object player()throws Exception {
        Constructor<?> c=Class.forName("pr").getDeclaredConstructor();
        c.setAccessible(true);return c.newInstance();
    }
    private static void set(Object value,String ownerName,String field,int n)throws Exception {
        for(Class<?> c=value.getClass();c!=null;c=c.getSuperclass()){
            if(ownerName.equals(c.getName())){
                Field f=c.getDeclaredField(field);f.setAccessible(true);
                f.setInt(value,n);return;
            }
        }
        throw new AssertionError("Original obfuscated fixture field missing");
    }
    private static Vanilla189CombatClock.Sample read(Object player)throws Exception {
        return Vanilla189CombatClock.read(player);
    }
    private static void denied(Runnable task,String what){
        boolean rejected=false;
        try{task.run();}catch(IllegalStateException|IllegalArgumentException ex){
            rejected=true;
        }
        yes(rejected,what);
    }
    public static void main(String[] args)throws Exception {
        Object player=player();
        set(player,"pk","W",18);set(player,"pr","bl",17);
        SourceClickAttackFence fence=new SourceClickAttackFence();
        yes(!fence.onClick(1,read(player)),"No candidate before an admitted tick");
        fence.prime(1,read(player));
        yes(!fence.onClick(1,read(player)),"Air/block/no new hit timestamp denied");
        set(player,"pr","bl",18);
        yes(fence.onClick(1,read(player)),"Actual click-return with new same-tick hit admitted");
        yes(!fence.onClick(1,read(player)),"Repeated click in same game tick denied");
        fence.prime(1,read(player));
        yes(!fence.onClick(1,read(player)),"Repriming does not duplicate hit");
        set(player,"pk","W",19);set(player,"pr","bl",19);
        yes(fence.onClick(1,read(player)),"One permitted clock increment within tick");
        yes(!fence.onClick(1,read(player)),"No duplicate callback for same hit");
        fence.prime(2,read(player));
        yes(!fence.onClick(1,read(player)),"Stale generation callback denied");
        yes(!fence.onClick(2,read(player)),"No old hit after world change");
        set(player,"pk","W",20);set(player,"pr","bl",20);
        yes(fence.onClick(2,read(player)),"New world fresh click admitted");
        fence.prime(2,read(player));
        set(player,"pk","W",23);set(player,"pr","bl",23);
        denied(()->{
            try{fence.onClick(2,read(player));}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Unexpected >1 player-tick jump denied");
        fence.prime(2,read(player));
        final AtomicBoolean foreign=new AtomicBoolean();
        Thread wrong=new Thread(()->{
            try{fence.onClick(2,null);}
            catch(IllegalStateException expected){foreign.set(true);}
        },"foreign-click-observer");
        wrong.start();wrong.join(5000L);
        yes(!wrong.isAlive()&&foreign.get(),"Foreign thread refused");
        fence.reset();
        yes(!fence.onClick(2,read(player)),"Retired fence cannot replay");
        fence.close();
        denied(()->fence.onClick(2,null),"Closed fence rejected");
        Vanilla189CombatClock.release();
        System.out.println("R362_ORIGINAL_CLICK_RETURN_HIT_FENCE_ONESHOT_PASS");
    }
}
