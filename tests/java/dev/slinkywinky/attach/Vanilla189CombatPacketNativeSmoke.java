package dev.slinkywinky.attach;

import dev.slinkywinky.SourcePacketReadOnlyQueue;
import dev.slinkywinky.SourceRuntimePreview;
import dev.slinkywinky.instrument.Vanilla189OutboundPacketSampler;
import dev.slinkywinky.instrument.Vanilla189OutboundPrimitiveQueue;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.util.concurrent.atomic.AtomicBoolean;

/** R369: source enum/category -> real native JNI event3, no packet IO. */
public final class Vanilla189CombatPacketNativeSmoke {
    static void yes(boolean v,String what){if(!v)throw new AssertionError(what);}
    static Object instance(String owner)throws Exception{
        Constructor<?> c=Class.forName(owner).getDeclaredConstructor();
        c.setAccessible(true);return c.newInstance();
    }
    static void action(Object packet,int ordinal)throws Exception{
        Object[] values=Class.forName("is$a").getEnumConstants();
        if(ordinal<0||ordinal>=values.length)throw new AssertionError("Wrong enum fixture ordinal");
        Field b=Class.forName("is").getDeclaredField("b");
        b.setAccessible(true);b.set(packet,values[ordinal]);
    }
    static void reject(Runnable run,String why){
        boolean rejected=false;
        try{run.run();}
        catch(IllegalArgumentException|IllegalStateException bad){rejected=true;}
        yes(rejected,why);
    }
    static SourceRuntimePreview.Receipt nativeTick(SourceRuntimePreview source){
        return source.observe(1001,1002,1,1,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,0,0,0,0,0,0,0,0,0},
            new double[]{9,10,10,0,0,150});
    }
    static SourceRuntimePreview.Receipt nativePacket(SourceRuntimePreview source,
                                               SourcePacketReadOnlyQueue.Observation o){
        int kind=o.kind==SourcePacketReadOnlyQueue.MOVEMENT?1:
            (o.kind==SourcePacketReadOnlyQueue.ATTACK?2:
             (o.kind==SourcePacketReadOnlyQueue.STOP_SPRINT?3:0));
        return source.observe(1001,1002,1,1,SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{kind,o.sequence,o.moving?1:0,1},
            new double[]{0,o.x,o.y,o.z});
    }
    public static void main(String[] args)throws Exception{
        final Class<?> ff=Class.forName("ff"),ip=Class.forName("ip");
        final Class<?> in=Class.forName("in"),is=Class.forName("is");
        final Class<?> enumClass=Class.forName("is$a");
        final Object useEntity=instance("in"),action=instance("is");
        final Object actionSubclass=instance("ix");
        final Object unknown=instance("fm"),movement=instance("ip");
        Vanilla189OutboundPacketSampler sampler=
            new Vanilla189OutboundPacketSampler(ff,ip,in,is,enumClass);
        yes(sampler.sample(useEntity).kind==SourcePacketReadOnlyQueue.ATTACK,
            "Original C02 is a use-entity attack-release category");
        yes(sampler.sample(action).kind==SourcePacketReadOnlyQueue.STOP_SPRINT,
            "Original C0B enum ordinal 4 is stop-sprint");
        yes(sampler.sample(actionSubclass).kind==SourcePacketReadOnlyQueue.STOP_SPRINT,
            "Original C0B subclass inherits ordinal source action");
        action(action,3);
        yes(sampler.sample(action).kind==SourcePacketReadOnlyQueue.UNKNOWN,
            "Start-sprint must NEVER be mistaken for stop-sprint");
        action(action,0);
        yes(sampler.sample(action).kind==SourcePacketReadOnlyQueue.UNKNOWN,
            "Sneak packet must remain unknown, not attack/sprint");
        action(action,4);
        yes(sampler.sample(unknown).kind==SourcePacketReadOnlyQueue.UNKNOWN,
            "Unrelated original ff packet remains UNKNOWN");
        yes(sampler.sample(movement).kind==SourcePacketReadOnlyQueue.MOVEMENT,
            "Original C03 movement remains unchanged");
        reject(()->{
            try{new Vanilla189OutboundPacketSampler(ff,ip,in,is,ff);}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Non-enum replacement denied before source observation");

        final Vanilla189OutboundPrimitiveQueue queue=
            new Vanilla189OutboundPrimitiveQueue(ff,ip,in,is,enumClass);
        final AtomicBoolean allOffered=new AtomicBoolean();
        Thread netty=new Thread(()->{
            boolean ok=queue.observe(movement,System.nanoTime())&&
                queue.observe(useEntity,System.nanoTime())&&
                queue.observe(action,System.nanoTime())&&
                queue.observe(unknown,System.nanoTime());
            allOffered.set(ok);
        },"original-outbound-network-manager-thread");
        netty.start();netty.join(5000L);
        yes(!netty.isAlive()&&allOffered.get(),
            "Real producer thread snapshots original outbound categories");
        action(action,3); // input packet MUTATED after primitive capture
        SourcePacketReadOnlyQueue.Batch batch=queue.drain(64);
        yes(batch.rows.size()==4&&batch.rows.get(0).sequence==1&&
            batch.rows.get(1).sequence==2&&
            batch.rows.get(2).sequence==3&&batch.rows.get(3).sequence==4,
            "Immutable strict FIFO order of four separate packet categories");
        yes(batch.rows.get(0).kind==SourcePacketReadOnlyQueue.MOVEMENT&&
            batch.rows.get(1).kind==SourcePacketReadOnlyQueue.ATTACK&&
            batch.rows.get(2).kind==SourcePacketReadOnlyQueue.STOP_SPRINT&&
            batch.rows.get(3).kind==SourcePacketReadOnlyQueue.UNKNOWN,
            "No original packet, enum object or later changes leak into FIFO");
        SourceRuntimePreview source=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt start=nativeTick(source);
        yes(start.sourceEvent==18&&start.at(2)==1&&start.at(3)==1,
            "Actual C++ Lag Range native event18 on owner thread");
        int[] expected={SourcePacketReadOnlyQueue.MOVEMENT,
            SourcePacketReadOnlyQueue.ATTACK,
            SourcePacketReadOnlyQueue.STOP_SPRINT,
            SourcePacketReadOnlyQueue.UNKNOWN};
        for(int i=0;i<4;i++){
            SourcePacketReadOnlyQueue.Observation row=batch.rows.get(i);
            yes(row.kind==expected[i],"Unexpected original packet category");
            SourceRuntimePreview.Receipt r=nativePacket(source,row);
            yes(r.sourceEvent==3&&r.at(2)==1&&(r.at(3)==0||r.at(3)==1),
                "Actual native C++ event3 must execute and return a bounded verdict");
        }
        // Native C++ real Lag Range event15 -> e18 hold-active -> C02
        // packet e3 should set released_on_attack (receipt[4]).
        SourceRuntimePreview attackSource=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt phase=attackSource.observe(
            5001,5002,1,1,SourceRuntimePreview.LAG_RANGE_ATTACK,
            new int[]{1,1},new double[0]);
        yes(phase.sourceEvent==15&&phase.at(2)==1&&phase.at(3)==1,
            "Original native Lag Range source attack phase owns this epoch");
        SourceRuntimePreview.Receipt attackTick=attackSource.observe(
            5001,5002,1,1,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,0,0,0,0,0,0,0,0,0},
            new double[]{9,10,10,0,0,150});
        yes(attackTick.sourceEvent==18&&attackTick.at(3)==1,
            "Actual C++ Lag Range attack phase consumed in eligible tick");
        SourceRuntimePreview.Receipt attackRelease=attackSource.observe(
            5001,5002,1,1,SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{2,1,0,1},new double[]{0,0,0,0});
        yes(attackRelease.sourceEvent==3&&attackRelease.at(4)==1,
            "Original C02 packet releases native C++ attack hold");
        attackSource.close();

        // Source sprint release requires an actual sprinting -> NOT
        // sprinting transition while the original C0B ordinal 4 arrives.
        SourceRuntimePreview sprintSource=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt sprintStart=sprintSource.observe(
            6001,6002,1,1,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,1,0,0,0,0,0,0,1,0},
            new double[]{9,10,10,0,0,150});
        yes(sprintStart.sourceEvent==18&&sprintStart.at(3)==1,
            "Native C++ Lag Range sprint precondition admitted");
        SourceRuntimePreview.Receipt sprintStop=sprintSource.observe(
            6001,6002,1,2,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,0,0,0,0,0,0,0,1,0},
            new double[]{9,10,10,0,0,150});
        yes(sprintStop.sourceEvent==18&&sprintStop.at(3)==1,
            "Native sprint stop transition occurred in real event18");
        SourceRuntimePreview.Receipt wrongAction=sprintSource.observe(
            6001,6002,1,2,SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{0,1,0,1},new double[]{0,0,0,0});
        yes(wrongAction.sourceEvent==3&&wrongAction.at(5)==0,
            "Non-stop-sprint C0B must never release the native packet window");
        SourceRuntimePreview.Receipt sprintRelease=sprintSource.observe(
            6001,6002,1,2,SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{3,2,0,1},new double[]{0,0,0,0});
        yes(sprintRelease.sourceEvent==3&&sprintRelease.at(5)==1,
            "Source C0B ordinal 4 releases real native C++ sprint window");
        sprintSource.close();

        queue.close();source.close();sampler.close();
        reject(()->queue.drain(1),"Retired source sender queue cannot replay");
        System.out.println("R369_REAL_ORIGINAL_COMBAT_CATEGORIES_NATIVE_E3_JNI_PASS");
    }
}
