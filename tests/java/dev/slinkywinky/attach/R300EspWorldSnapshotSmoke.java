package dev.slinkywinky.attach;

import dev.slinkywinky.EspWorldSnapshotSampler;
import java.util.*;
/** Java8 standalone negative controls; does not load Minecraft or native code. */
public final class R300EspWorldSnapshotSmoke {
    static final EspWorldSnapshotSampler.Position Z=
        new EspWorldSnapshotSampler.Position(0,0,0);
    static final UUID A=UUID.fromString("00000000-0000-0000-0000-000000000001");
    static final UUID B=UUID.fromString("00000000-0000-0000-0000-000000000002");
    static final class Port implements EspWorldSnapshotSampler.Port {
        Object world=new Object(),owner=new Object(),swapWorld;
        boolean client=true;
        EspWorldSnapshotSampler.Position self=Z;
        List<EspWorldSnapshotSampler.Tile> tiles=new ArrayList<EspWorldSnapshotSampler.Tile>();
        List<EspWorldSnapshotSampler.Drop> drops=new ArrayList<EspWorldSnapshotSampler.Drop>();
        List<EspWorldSnapshotSampler.Player> players=new ArrayList<EspWorldSnapshotSampler.Player>();
        Port(){players.add(new EspWorldSnapshotSampler.Player(A,Z,Z,true,false,20,20,0,0));}
        public boolean onClientThread(){return client;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return owner;}
        public EspWorldSnapshotSampler.Position ownerPosition(){return self;}
        public List<EspWorldSnapshotSampler.Tile> tiles(){return tiles;}
        public List<EspWorldSnapshotSampler.Drop> drops(){
            if(swapWorld!=null)world=swapWorld;
            return drops;
        }
        public List<EspWorldSnapshotSampler.Player> players(){return players;}
    }
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static void deny(Runnable r,String why){
        boolean denied=false;
        try{r.run();}catch(IllegalStateException ok){denied=true;}
        check(denied,why);
    }
    public static void main(String[] ignored)throws Exception {
        final Port port=new Port();
        port.tiles.add(new EspWorldSnapshotSampler.Tile(Z,54));
        port.tiles.add(new EspWorldSnapshotSampler.Tile(Z,130));
        port.drops.add(new EspWorldSnapshotSampler.Drop(Z,Z,264,12,false,true,true));
        port.players.add(new EspWorldSnapshotSampler.Player(B,Z,Z,false,false,9,20,2,4));
        final EspWorldSnapshotSampler s=new EspWorldSnapshotSampler();
        final EspWorldSnapshotSampler.Receipt first=s.observe(port,10);
        check(first.worldToken>0&&first.worldToken!=first.playerToken&&
              first.generation>0,"tokens");
        check(first.tiles.get(0).blockId==54&&first.tiles.get(1).blockId==130&&
              first.drops.get(0).stackSize==12&&
              first.players.get(1).uuid.equals(B),"encounter order");
        port.tiles.clear();port.drops.clear();port.players.remove(1);
        check(first.tiles.size()==2&&first.drops.size()==1&&
              first.players.size()==2,"snapshot defensive copy");
        boolean immutable=false;
        try{first.tiles.clear();}catch(UnsupportedOperationException ok){immutable=true;}
        check(immutable,"unmodifiable rows");
        deny(new Runnable(){public void run(){s.observe(port,10);}},"same tick");
        deny(new Runnable(){public void run(){s.observe(port,9);}},"stale tick");
        EspWorldSnapshotSampler.Receipt second=s.observe(port,11);
        check(second.worldToken==first.worldToken&&second.generation==first.generation,
              "stable generation");
        port.self=new EspWorldSnapshotSampler.Position(Double.NaN,0,0);
        deny(new Runnable(){public void run(){s.observe(port,12);}},"NaN position");
        port.self=Z;
        port.players.add(new EspWorldSnapshotSampler.Player(A,Z,Z,false,false,2,20,0,0));
        deny(new Runnable(){public void run(){s.observe(port,12);}},"duplicate player ID");
        port.players.remove(1);
        port.swapWorld=new Object();
        deny(new Runnable(){public void run(){s.observe(port,12);}},"midread swap");
        port.swapWorld=null;
        EspWorldSnapshotSampler.Receipt changed=s.observe(port,0);
        check(changed.generation>first.generation&&
              changed.worldToken!=first.worldToken,"new world, tick reset");
        s.retire();
        check(s.generation()>changed.generation,"retired epoch");
        EspWorldSnapshotSampler.Receipt resumed=s.observe(port,0);
        check(resumed.worldToken!=changed.worldToken&&
              resumed.generation>s.generation()-1,"retired token renewal");
        port.client=false;
        deny(new Runnable(){public void run(){s.observe(port,1);}},"unowned port");
        port.client=true;
        final boolean[] denied={false};
        Thread foreign=new Thread(new Runnable(){
            public void run(){
                try{s.observe(port,1);}catch(IllegalStateException ok){denied[0]=true;}
            }
        });
        foreign.start();foreign.join();
        check(denied[0],"wrong thread");
        s.close();
        deny(new Runnable(){public void run(){s.observe(port,1);}},"closed sampler");
        System.out.println("R300_ESP_WORLD_OWNER_READONLY_SMOKE_PASS");
    }
}
