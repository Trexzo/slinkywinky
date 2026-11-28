package dev.slinkywinky.attach;

import dev.slinkywinky.SourcePacketReadOnlyQueue;
import dev.slinkywinky.SourceRuntimePreview;
import dev.slinkywinky.instrument.Vanilla189OutboundPacketSampler;
import dev.slinkywinky.instrument.Vanilla189OutboundPrimitiveQueue;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.util.concurrent.atomic.AtomicBoolean;

/** Source-free original-name 1.8.9 packet sample -> bounded FIFO -> REAL JNI e3. */
public final class Vanilla189OutboundPacketSmoke {
    static void yes(boolean v,String s){if(!v)throw new AssertionError(s);}
    static Object instance(String n)throws Exception{
        Constructor<?> c=Class.forName(n).getDeclaredConstructor();c.setAccessible(true);
        return c.newInstance();
    }
    static void set(Object subject,String owner,String name,Object value)throws Exception{
        for(Class<?> c=subject.getClass();c!=null;c=c.getSuperclass()){
            if(c.getName().equals(owner)){
                Field f=c.getDeclaredField(name);f.setAccessible(true);
                f.set(subject,value);return;
            }
        }
        throw new AssertionError("Original packet test field absent");
    }
    static void reject(Runnable r,String s){
        boolean denied=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException ex){denied=true;}
        yes(denied,s);
    }
    static SourceRuntimePreview.Receipt tick(SourceRuntimePreview source){
        return source.observe(1001,1002,1,1,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,0,1,0,0,0,0,0,0,0,0,0},
            new double[]{9,10,10,0,0,150});
    }
    static SourceRuntimePreview.Receipt packet(SourceRuntimePreview source,
                                                SourcePacketReadOnlyQueue.Observation o){
        return source.observe(1001,1002,1,1,
            SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{o.kind==SourcePacketReadOnlyQueue.MOVEMENT?1:0,
                o.sequence,o.moving?1:0,1},
            new double[]{0,o.x,o.y,o.z});
    }
    public static void main(String[] args)throws Exception{
        Class<?> ff=Class.forName("ff"),ip=Class.forName("ip");
        Object move=instance("ip"),unknown=instance("fm"),subclass=instance("iq");
        Vanilla189OutboundPacketSampler sampler=
            new Vanilla189OutboundPacketSampler(ff,ip);
        Vanilla189OutboundPacketSampler.Sample initial=sampler.sample(move);
        yes(initial.kind==SourcePacketReadOnlyQueue.MOVEMENT&&
            initial.positionKnown&&initial.x==8&&initial.y==0&&initial.z==0,
            "Strict original ip.g + ip.abc primitive snapshot");
        yes(sampler.sample(subclass).positionKnown,
            "C03 subtype inherits original field owner");
        yes(sampler.sample(unknown).kind==SourcePacketReadOnlyQueue.UNKNOWN,
            "Unknown ff packet must NOT be guessed as attack or sprint");
        reject(()->{
            try{sampler.sample(new Object());}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Non-ff object refused");
        set(move,"ip","a",Double.NaN);
        reject(()->{
            try{sampler.sample(move);}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Original source NaN denied");
        set(move,"ip","a",30000001d);
        reject(()->{
            try{sampler.sample(move);}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Unbounded movement denied");
        set(move,"ip","a",8d);
        set(move,"ip","g",false);
        yes(!sampler.sample(move).positionKnown&&sampler.sample(move).x==0,
            "Packet without movement flag cannot leak stale position");
        set(move,"ip","g",true);
        sampler.close();
        reject(()->{
            try{sampler.sample(move);}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Retired packet mapping cannot inspect later objects");

        final Vanilla189OutboundPrimitiveQueue queue=
            new Vanilla189OutboundPrimitiveQueue(ff,ip);
        final AtomicBoolean offered=new AtomicBoolean();
        Thread netty=new Thread(()->{
            boolean a=queue.observe(move,System.nanoTime());
            boolean b=queue.observe(unknown,System.nanoTime());
            offered.set(a&&b);
        },"original-NetworkManager-sender");
        netty.start();netty.join(5000L);
        yes(!netty.isAlive()&&offered.get(),"Original packet-sender thread offers primitives");
        set(move,"ip","a",-22d); // packet mutated AFTER capture
        SourcePacketReadOnlyQueue.Batch batch=queue.drain(64);
        yes(batch.rows.size()==2&&batch.rows.get(0).sequence==1&&
            batch.rows.get(1).sequence==2&&
            batch.rows.get(0).x==8&&
            batch.rows.get(0).kind==SourcePacketReadOnlyQueue.MOVEMENT&&
            batch.rows.get(1).kind==SourcePacketReadOnlyQueue.UNKNOWN,
            "FIFO owns immutable primitive copies, no Packet or Netty refs");
        yes(batch.dropped==0&&batch.totalCaptured==2,"No unexpected loss");
        SourceRuntimePreview source=new SourceRuntimePreview();
        SourceRuntimePreview.Receipt e18=tick(source);
        yes(e18.sourceEvent==18&&e18.at(2)==1&&e18.at(3)==1,
            "Original-name sender samples have a genuine JNI native e18");
        SourceRuntimePreview.Receipt movement=packet(source,batch.rows.get(0));
        yes(movement.sourceEvent==3&&movement.at(2)==1,
            "Actual C++ Lag Range event3 evaluates movement sample");
        SourceRuntimePreview.Receipt other=packet(source,batch.rows.get(1));
        yes(other.sourceEvent==3&&other.at(2)==1,
            "Unknown packet receives ONLY other category");
        queue.close();source.close();
        reject(()->queue.drain(1),"Retired source queue cannot replay");
        yes(!queue.observe(move,System.nanoTime()),"No packet after retire");

        Vanilla189OutboundPrimitiveQueue overflow=
            new Vanilla189OutboundPrimitiveQueue(ff,ip);
        for(int i=0;i<SourcePacketReadOnlyQueue.CAPACITY;++i)
            yes(overflow.observe(unknown,System.nanoTime()),"Bounded primitive ingress accepts capacity");
        yes(!overflow.observe(unknown,System.nanoTime())&&!overflow.healthy(),
            "256+1 packet overflow must poison ENTIRE stream");
        reject(()->overflow.drain(64),"Source JNI refuses incomplete packet observations");
        overflow.close();
        System.out.println("R365_ORIGINAL_OUTBOUND_PACKET_PRIMITIVE_NATIVE_E3_READONLY_PASS");
    }
}
