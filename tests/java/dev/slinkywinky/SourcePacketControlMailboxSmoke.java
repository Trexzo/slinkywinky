package dev.slinkywinky;

/**
 * R346: a REAL native four-module JNI client tick -> immutable volatile
 * Netty-side inspection, without crossing JVM owner or changing packets.
 */
public final class SourcePacketControlMailboxSmoke {
    static void yes(boolean value,String what){if(!value)throw new AssertionError(what);}
    static void denied(Runnable job,String what){
        boolean rejected=false;
        try{job.run();}
        catch(IllegalArgumentException|IllegalStateException expected){rejected=true;}
        yes(rejected,what);
    }
    public static void main(String[] args)throws Exception{
        SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost();
        SourceWorldReadOnlyHostSmoke.Port port=new SourceWorldReadOnlyHostSmoke.Port();
        SourcePacketControlMailbox mailbox=new SourcePacketControlMailbox();
        long channel=1729;
        long now=System.nanoTime();
        SourceWorldReadOnlyHost.TickReceipt tick=host.observe(port);
        SourcePacketControlMailbox.Ticket first=mailbox.publish(
            channel,71,tick,now);
        yes(first.worldToken==tick.backtrack.world&&
            first.playerToken==tick.backtrack.player&&
            first.generation==tick.generation&&first.tick==tick.tick,
            "Source policy uses actual native JNI world/player generation/tick");
        yes(first.backtrackCandidate==(tick.backtrack.at(3)==1)&&
            first.lagRangeCandidate==(tick.lagRange.at(3)==1),
            "Native active tick status copied, not fabricated");
        yes(mailbox.inspect(channel,first.worldToken,first.generation,
                           now+50_000_000L)==first,
            "Exact channel and world generation admitted in ticket age window");
        yes(mailbox.inspect(channel+1,first.worldToken,first.generation,now)==null,
            "Cannot reuse a policy on another Netty channel");
        yes(mailbox.inspect(channel,first.worldToken+1,first.generation,now)==null,
            "Cannot reuse a policy in a different Minecraft world");
        yes(mailbox.inspect(channel,first.worldToken,first.generation+1,now)==null,
            "Cannot reuse a policy in another source JNI generation");
        yes(mailbox.inspect(channel,first.worldToken,first.generation,
                           now+SourcePacketControlMailbox.MAX_TICKET_AGE_NANOS+1)==null,
            "Expired source tick must never authorize a Netty action");
        yes(mailbox.inspect(channel,first.worldToken,first.generation,now-1)==null,
            "Ticket timestamp cannot be interpreted as future receipt");

        final boolean[] readFromNetty={false},foreignPublisherDenied={false};
        Thread netty=new Thread(()->{
            SourcePacketControlMailbox.Ticket observed=mailbox.inspect(
                channel,first.worldToken,first.generation,now+1);
            readFromNetty[0]=observed==first;
            try{mailbox.publish(channel,71,tick,now+2);}
            catch(IllegalStateException expected){foreignPublisherDenied[0]=true;}
        });
        netty.start();netty.join();
        yes(readFromNetty[0],
            "Netty reads safe primitive-only native ticket from foreign thread");
        yes(foreignPublisherDenied[0],
            "Netty cannot publish fake native source policy");
        denied(()->mailbox.publish(channel,71,tick,now+3),
               "One JNI source tick cannot publish twice");
        SourceWorldReadOnlyHost.TickReceipt second=host.observe(port);
        SourcePacketControlMailbox.Ticket next=mailbox.publish(
            channel,71,second,now+40_000_000L);
        yes(next.tick==first.tick+1&&next.sequence==first.sequence+1,
            "Strict monotonically advancing native client tick and mailbox seq");
        yes(mailbox.inspect(channel,first.worldToken,first.generation,
                           now+45_000_000L)==next,
            "Only the last immutable snapshot exists (one-slot overwrite)");

        mailbox.revoke();
        yes(mailbox.inspect(channel,next.worldToken,next.generation,now+45_000_000L)==null,
            "Owner disconnect revokes previously published source ticket");
        port.world=new Object();
        SourceWorldReadOnlyHost.TickReceipt changed=host.observe(port);
        SourcePacketControlMailbox.Ticket newWorld=mailbox.publish(
            channel+1,71,changed,now+60_000_000L);
        yes(newWorld.generation==next.generation+1,
            "New world source JNI session re-established");
        yes(mailbox.inspect(channel,next.worldToken,next.generation,
                           now+61_000_000L)==null,
            "World replacement refuses stale policy even before TTL");
        yes(mailbox.inspect(channel+1,newWorld.worldToken,newWorld.generation,
                           now+61_000_000L)==newWorld,
            "New channel and world generation can publish fresh native receipt");
        denied(()->mailbox.publish(channel,71,tick,now+70_000_000L),
               "Old-generation JNI receipt cannot be replayed after new world");

        mailbox.close();
        yes(mailbox.inspect(channel+1,newWorld.worldToken,
                            newWorld.generation,now+61_000_000L)==null,
            "Closed source mailbox has no residual Netty candidate");
        denied(()->mailbox.publish(channel,71,changed,now+71_000_000L),
               "Closed mailbox cannot publish");
        host.close();
        System.out.println("R346_JAVA8_REAL_JNI_TO_NETTY_EPOCH_TICKET_PASS");
    }
}
