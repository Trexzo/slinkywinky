package dev.slinkywinky.forge;

import dev.slinkywinky.SourcePacketReadOnlyQueue;
import io.netty.channel.Channel;
import io.netty.channel.ChannelDuplexHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.ChannelPromise;
import java.util.concurrent.atomic.AtomicInteger;
import java.lang.reflect.Field;
import net.minecraft.client.Minecraft;
import net.minecraft.client.network.NetHandlerPlayClient;
import net.minecraft.network.NetworkManager;
import net.minecraft.network.Packet;
import net.minecraft.network.play.client.C02PacketUseEntity;
import net.minecraft.network.play.client.C03PacketPlayer;
import net.minecraft.network.play.client.C0BPacketEntityAction;
import net.minecraft.network.play.client.C0DPacketCloseWindow;
import net.minecraft.network.play.client.C0EPacketClickWindow;
import net.minecraft.network.play.client.C16PacketClientStatus;
import net.minecraft.network.play.server.S06PacketUpdateHealth;
import net.minecraft.network.play.server.S08PacketPlayerPosLook;
import net.minecraft.network.play.server.S12PacketEntityVelocity;
import net.minecraft.network.play.server.S13PacketDestroyEntities;
import net.minecraft.network.play.server.S14PacketEntity;
import net.minecraft.network.play.server.S18PacketEntityTeleport;
import net.minecraft.network.play.server.S0CPacketSpawnPlayer;
import net.minecraft.network.play.server.S0FPacketSpawnMob;
import net.minecraft.network.play.server.S19PacketEntityStatus;
import net.minecraft.network.play.server.S29PacketSoundEffect;

/**
 * R339: genuinely observed decoded Minecraft 1.8.9 packet classes.
 * A ChannelDuplexHandler is attached BEFORE "packet_handler", solely in the
 * local singleplayer client channel. Handler never calls JNI or Minecraft
 * world getters and NEVER alters, releases, cancels, delays or retains packets.
 *
 * Netty event-loop -> SourcePacketReadOnlyQueue primitive copy -> later
 * Minecraft ClientTick.END. This does NOT recreate source packet timing.
 */
public final class ForgeSourcePacketTap implements AutoCloseable {
    private static final AtomicInteger NEXT=new AtomicInteger();
    private final Thread owner=Thread.currentThread();
    private final String handlerName="slinky_source_readonly_"+NEXT.incrementAndGet();
    private Object world,player;
    private NetworkManager manager;
    private Channel channel;
    private SourcePacketReadOnlyQueue queue;
    private volatile boolean installed;
    private boolean closed;

    private void assertOwner(){
        if(closed||Thread.currentThread()!=owner)
            throw new IllegalStateException("Closed/foreign Netty tap controller");
    }
    public SourcePacketReadOnlyQueue ingress(){
        assertOwner();
        return installed?queue:null;
    }
    private static final class Observer extends ChannelDuplexHandler {
        // R352: the private candidate JNI uses GetIntField on S19, not
        // getEntity(World). Only exact mapped int fields are admissible.
        // No World, game Entity, or JNI calls occur on Netty.
        private static final Field SOURCE_S19_ID=resolveS19EntityId();
        private static Field resolveS19EntityId(){
            final String[] mappedNames={"entityId","field_149164_a","a"};
            for(String name:mappedNames){
                try{
                    Field field=S19PacketEntityStatus.class.getDeclaredField(name);
                    if(field.getType()!=Integer.TYPE)continue;
                    field.setAccessible(true);
                    return field;
                }catch(ReflectiveOperationException|SecurityException missing){
                    // Try only the other original-candidate mapping names.
                }
            }
            return null;
        }
        private final SourcePacketReadOnlyQueue ingress;
        private Observer(SourcePacketReadOnlyQueue queue){ingress=queue;}
        private void observe(int direction,Object msg){
            if(!(msg instanceof Packet))return;
            int kind=SourcePacketReadOnlyQueue.UNKNOWN;
            int entity=0;
            boolean entityKnown=false,moving=false;
            double x=0,y=0,z=0;
            if(direction==SourcePacketReadOnlyQueue.IN){
                if(msg instanceof S19PacketEntityStatus){
                    kind=SourcePacketReadOnlyQueue.ENTITY_STATUS;
                    // R352: candidate's exact private integer field is
                    // admissible without the public getEntity(World) API.
                    // Unknown/default/nonpositive IDs remain untrusted.
                    if(SOURCE_S19_ID!=null){
                        try{
                            final int statusEntityId=SOURCE_S19_ID.getInt(msg);
                            if(statusEntityId>0){
                                entity=statusEntityId;
                                entityKnown=true;
                            }
                        }catch(IllegalAccessException invalid){
                            throw new IllegalStateException(
                                "S19 source field inaccessible after admission",invalid);
                        }
                    }
                }else if(msg instanceof S06PacketUpdateHealth){
                    kind=SourcePacketReadOnlyQueue.UPDATE_HEALTH;
                }else if(msg instanceof S29PacketSoundEffect){
                    kind=SourcePacketReadOnlyQueue.SOUND;
                }else if(msg instanceof S12PacketEntityVelocity){
                    kind=SourcePacketReadOnlyQueue.VELOCITY;
                    entity=((S12PacketEntityVelocity)msg).getEntityID();
                    entityKnown=true;
                }else if(msg instanceof S08PacketPlayerPosLook){
                    kind=SourcePacketReadOnlyQueue.TELEPORT;
                }else if(msg instanceof S18PacketEntityTeleport){
                    S18PacketEntityTeleport tele=(S18PacketEntityTeleport)msg;
                    kind=SourcePacketReadOnlyQueue.ENTITY_ABSOLUTE_TELEPORT;
                    entity=tele.getEntityId();entityKnown=true;moving=true;
                    x=tele.getX()/32.0d;y=tele.getY()/32.0d;z=tele.getZ()/32.0d;
                }else if(msg instanceof S0CPacketSpawnPlayer){
                    S0CPacketSpawnPlayer spawn=(S0CPacketSpawnPlayer)msg;
                    kind=SourcePacketReadOnlyQueue.PLAYER_SPAWN_ABSOLUTE;
                    entity=spawn.getEntityID();entityKnown=true;moving=true;
                    x=spawn.getX()/32.0d;y=spawn.getY()/32.0d;z=spawn.getZ()/32.0d;
                }else if(msg instanceof S0FPacketSpawnMob){
                    S0FPacketSpawnMob spawn=(S0FPacketSpawnMob)msg;
                    kind=SourcePacketReadOnlyQueue.MOB_SPAWN_ABSOLUTE;
                    entity=spawn.getEntityID();entityKnown=true;moving=true;
                    x=spawn.getX()/32.0d;y=spawn.getY()/32.0d;z=spawn.getZ()/32.0d;
                }else if(msg instanceof S14PacketEntity.S15PacketEntityRelMove||
                         msg instanceof S14PacketEntity.S17PacketEntityLookMove){
                    // No public entityId accessor on MCP 1.8.9 S14;
                    // record a conservative global packet provenance barrier.
                    kind=SourcePacketReadOnlyQueue.RELATIVE_UNRESOLVED;
                }else if(msg instanceof S13PacketDestroyEntities){
                    // Multiple IDs, no Packet objects retained on Netty.
                    // Invalidate packet-only claims conservatively.
                    kind=SourcePacketReadOnlyQueue.DESTROY_UNRESOLVED;
                }
            }else{
                if(msg instanceof C03PacketPlayer){
                    C03PacketPlayer movement=(C03PacketPlayer)msg;
                    kind=SourcePacketReadOnlyQueue.MOVEMENT;
                    moving=movement.isMoving();
                    if(moving){
                        x=movement.getPositionX();
                        y=movement.getPositionY();
                        z=movement.getPositionZ();
                    }
                }else if(msg instanceof C02PacketUseEntity){
                    if(((C02PacketUseEntity)msg).getAction()==C02PacketUseEntity.Action.ATTACK)
                        kind=SourcePacketReadOnlyQueue.ATTACK;
                }else if(msg instanceof C0BPacketEntityAction){
                    if(((C0BPacketEntityAction)msg).getAction()==
                       C0BPacketEntityAction.Action.STOP_SPRINTING)
                        kind=SourcePacketReadOnlyQueue.STOP_SPRINT;
                }else if(msg instanceof C16PacketClientStatus){
                    if(((C16PacketClientStatus)msg).getStatus()==
                       C16PacketClientStatus.EnumState.OPEN_INVENTORY_ACHIEVEMENT)
                        kind=SourcePacketReadOnlyQueue.INV_OPEN;
                }else if(msg instanceof C0EPacketClickWindow){
                    kind=SourcePacketReadOnlyQueue.INV_CLICK;
                }else if(msg instanceof C0DPacketCloseWindow){
                    kind=SourcePacketReadOnlyQueue.INV_CLOSE;
                }
            }
            ingress.offer(direction,kind,entity,entityKnown,moving,
                          x,y,z,System.nanoTime());
        }
        @Override public void channelRead(ChannelHandlerContext ctx,Object msg)
                throws Exception {
            try{observe(SourcePacketReadOnlyQueue.IN,msg);}
            catch(Throwable captureFailure){ingress.poison();}
            // EXACT same decoded Minecraft packet reference, forwarded once.
            ctx.fireChannelRead(msg);
        }
        @Override public void write(ChannelHandlerContext ctx,Object msg,
                                    ChannelPromise promise)throws Exception {
            try{observe(SourcePacketReadOnlyQueue.OUT,msg);}
            catch(Throwable captureFailure){ingress.poison();}
            // No interception, replacement, hold, cancellation or promise edits.
            ctx.write(msg,promise);
        }
    }
    /** ForgeGradle test-only passive observer construction; no Minecraft state. */
    static ChannelDuplexHandler diagnosticObserver(SourcePacketReadOnlyQueue q){
        if(q==null)throw new IllegalArgumentException("Missing passive packet queue");
        return new Observer(q);
    }
    /**
     * Call only from Minecraft client tick. Attaches asynchronously on
     * Netty event loop, never blocks a frame while mutating the pipeline.
     * Returns true if the observed channel/world epoch was replaced.
     */
    public boolean reconcile(Minecraft mc){
        assertOwner();
        final Object w=mc!=null&&mc.theWorld!=null&&mc.thePlayer!=null&&
            mc.isCallingFromMinecraftThread()&&mc.theWorld.isRemote&&
            mc.isSingleplayer()?mc.theWorld:null;
        final Object p=w==null?null:mc.thePlayer;
        final NetHandlerPlayClient handler=w==null?null:mc.getNetHandler();
        final NetworkManager candidate=handler==null?null:handler.getNetworkManager();
        final Channel candidateChannel=candidate!=null&&candidate.isLocalChannel()?
            candidate.channel():null;
        final Channel usable=candidateChannel!=null&&candidateChannel.isOpen()?
            candidateChannel:null;
        if(world==w&&player==p&&manager==candidate&&channel==usable)return false;
        final boolean replaced=world!=null||player!=null||queue!=null;
        retireAttached();
        if(w==null||p==null||usable==null)return replaced;
        world=w;player=p;manager=candidate;channel=usable;
        final SourcePacketReadOnlyQueue nextQueue=new SourcePacketReadOnlyQueue();
        queue=nextQueue;
        installed=false;
        final Observer observer=new Observer(nextQueue);
        try{
            usable.eventLoop().execute(new Runnable(){
                @Override public void run(){
                    if(!nextQueue.healthy()||!usable.isOpen())return;
                    try{
                        ChannelPipeline pipeline=usable.pipeline();
                        if(pipeline.get("packet_handler")==null||
                           pipeline.get(handlerName)!=null){
                            nextQueue.poison();
                            return;
                        }
                        pipeline.addBefore("packet_handler",handlerName,observer);
                        installed=true;
                    }catch(Throwable failure){
                        nextQueue.poison();
                    }
                }
            });
        }catch(RuntimeException shutdown){
            nextQueue.poison();
        }
        return true;
    }
    private void retireAttached(){
        final Channel old=channel;
        final SourcePacketReadOnlyQueue oldQueue=queue;
        installed=false;
        world=player=null;manager=null;channel=null;queue=null;
        if(oldQueue!=null)oldQueue.close();
        if(old!=null){
            try{
                old.eventLoop().execute(new Runnable(){
                    @Override public void run(){
                        ChannelPipeline pipeline=old.pipeline();
                        if(pipeline.get(handlerName)!=null)
                            pipeline.remove(handlerName);
                    }
                });
            }catch(RuntimeException channelShutdown){
                // Closing the channel destroys its pipeline. Old ingress
                // is ALREADY closed; no late packet can reach JNI.
            }
        }
    }
    @Override public void close(){
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("Foreign packet tap shutdown");
        if(closed)return;
        closed=true;
        retireAttached();
    }
}
