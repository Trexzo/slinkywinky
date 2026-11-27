package dev.slinkywinky.attach;

import dev.slinkywinky.SourceRuntimePreview;
import dev.slinkywinky.instrument.Vanilla189CombatClock;
import dev.slinkywinky.instrument.SourceClickAttackFence;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;

/** R362 original-name clock -> actual JNI C++ e15 -> e18, no packet IO. */
public final class Vanilla189ClickNativeLagSmoke {
    static void yes(boolean condition,String name){
        if(!condition)throw new AssertionError(name);
    }
    static Object fixture()throws Exception{
        Constructor<?> c=Class.forName("pr").getDeclaredConstructor();
        c.setAccessible(true);return c.newInstance();
    }
    static void set(Object value,String ownerName,String field,int n)throws Exception{
        for(Class<?> c=value.getClass();c!=null;c=c.getSuperclass()){
            if(ownerName.equals(c.getName())){
                Field f=c.getDeclaredField(field);f.setAccessible(true);
                f.setInt(value,n);return;
            }
        }
        throw new AssertionError("Original clock owner missing");
    }
    static SourceRuntimePreview.Receipt tick(SourceRuntimePreview source,
                                            long generation,long sequence){
        return source.observe(1001,1002,generation,sequence,
            SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,0,0,0,0,0,0,0,0,0},
            new double[]{9,10,10,0,0,150});
    }
    static SourceRuntimePreview.Receipt attack(SourceRuntimePreview source,
                                            long generation,long sequence){
        return source.observe(1001,1002,generation,sequence,
            SourceRuntimePreview.LAG_RANGE_ATTACK,
            new int[]{1,1},new double[0]);
    }
    public static void main(String[] args)throws Exception{
        Object player=fixture();
        set(player,"pk","W",18);set(player,"pr","bl",17);
        SourceClickAttackFence click=new SourceClickAttackFence();
        SourceRuntimePreview nativeSource=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt first=tick(nativeSource,1,1);
        yes(first.sourceEvent==18&&first.at(2)==1&&first.at(3)==1,
            "Real native Lag Range e18 admits original source target");
        click.prime(1,Vanilla189CombatClock.read(player));
        yes(!click.onClick(1,Vanilla189CombatClock.read(player)),
            "No phantom event15 from click without changed attack");
        set(player,"pr","bl",18);
        yes(click.onClick(1,Vanilla189CombatClock.read(player)),
            "Only fresh original click combat clock may dispatch event15");
        SourceRuntimePreview.Receipt e15=attack(nativeSource,1,1);
        yes(e15.sourceEvent==15&&e15.at(2)==1&&e15.at(3)==1,
            "REAL JNI C++ e15 dispatched after click return");
        yes(!click.onClick(1,Vanilla189CombatClock.read(player)),
            "Duplicate original click cannot double-dispatch e15");
        SourceRuntimePreview.Receipt next=tick(nativeSource,1,2);
        yes(next.sourceEvent==18&&next.at(2)==1,
            "Subsequent real native e18 consumes event15 in same session");
        click.prime(2,Vanilla189CombatClock.read(player));
        yes(!click.onClick(1,Vanilla189CombatClock.read(player)),
            "Previous world source click denied");
        yes(!click.onClick(2,Vanilla189CombatClock.read(player)),
            "New world cannot replay prior attack");
        set(player,"pk","W",19);set(player,"pr","bl",19);
        yes(click.onClick(2,Vanilla189CombatClock.read(player)),
            "New generation fresh original click allowed");
        SourceRuntimePreview.Receipt newWorld=attack(nativeSource,2,1);
        yes(newWorld.sourceEvent==15&&newWorld.at(2)==1,
            "Native original source generation switch admitted");
        SourceRuntimePreview.Receipt second=tick(nativeSource,2,2);
        yes(second.sourceEvent==18&&second.at(2)==1,
            "Native event18 tracks new game world");
        click.close();nativeSource.close();Vanilla189CombatClock.release();
        boolean closed=false;
        try{attack(nativeSource,2,3);}
        catch(IllegalStateException expected){closed=true;}
        yes(closed,"Retired native JNI cannot execute old game attack");
        System.out.println("R362_REAL_ORIGINAL_CLICK_E15_THEN_NATIVE_E18_JNI_PASS");
    }
}
