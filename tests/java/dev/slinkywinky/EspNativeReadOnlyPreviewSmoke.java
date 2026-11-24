package dev.slinkywinky;
import java.util.*;
/** R301 real JNI to three source controllers, no Forge/game process. */
public final class EspNativeReadOnlyPreviewSmoke {
    static EspWorldSnapshotSampler.Position at(double x,double y,double z){
        return new EspWorldSnapshotSampler.Position(x,y,z);
    }
    static final UUID OWNER=UUID.fromString("00000000-0000-0000-0000-000000000001");
    static final UUID OTHER=UUID.fromString("00000000-0000-0000-0000-000000000002");
    static final class Fixture implements EspWorldSnapshotSampler.Port {
        Object world=new Object(),player=new Object();
        List<EspWorldSnapshotSampler.Tile> tiles=new ArrayList<EspWorldSnapshotSampler.Tile>();
        List<EspWorldSnapshotSampler.Drop> drops=new ArrayList<EspWorldSnapshotSampler.Drop>();
        List<EspWorldSnapshotSampler.Player> players=new ArrayList<EspWorldSnapshotSampler.Player>();
        public boolean onClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public EspWorldSnapshotSampler.Position ownerPosition(){return at(0,0,0);}
        public List<EspWorldSnapshotSampler.Tile> tiles(){return tiles;}
        public List<EspWorldSnapshotSampler.Drop> drops(){return drops;}
        public List<EspWorldSnapshotSampler.Player> players(){return players;}
        Fixture(){
            tiles.add(new EspWorldSnapshotSampler.Tile(at(1,0,0),54));
            tiles.add(new EspWorldSnapshotSampler.Tile(at(1,0,1),130));
            tiles.add(new EspWorldSnapshotSampler.Tile(at(30,0,0),138));
            drops.add(new EspWorldSnapshotSampler.Drop(at(1,0,0),at(1,0,0),
                264,3,false,true,true));
            drops.add(new EspWorldSnapshotSampler.Drop(at(50,0,0),at(50,0,0),
                264,3,false,true,true));
            players.add(new EspWorldSnapshotSampler.Player(
                OWNER,at(0,0,0),at(0,0,0),true,false,20,20,0,0));
            players.add(new EspWorldSnapshotSampler.Player(
                OTHER,at(2,0,0),at(2,0,0),false,false,10,20,2,1));
        }
    }
    static void check(boolean x,String what){if(!x)throw new AssertionError(what);}
    static void refuse(Runnable r,String what){
        boolean ok=false;try{r.run();}catch(IllegalStateException expected){ok=true;}
        check(ok,what);
    }
    static void refuseArgument(Runnable r,String what){
        boolean ok=false;
        try{r.run();}catch(IllegalArgumentException expected){ok=true;}
        check(ok,what);
    }
    public static void main(String[] args)throws Exception{
        Fixture f=new Fixture();
        EspWorldSnapshotSampler javaSampler=new EspWorldSnapshotSampler();
        EspNativeReadOnlyPreview nativePreview=new EspNativeReadOnlyPreview();
        EspWorldSnapshotSampler.Receipt one=javaSampler.observe(f,10);
        EspNativeReadOnlyPreview.Result result=nativePreview.observe(one,10,10,10);
        check(result.rawTiles==3&&result.rawDrops==2&&result.rawPlayers==2,
              "native input provenance");
        check(result.chestRows==2&&result.itemRows==1&&result.playerRows==1,
              "three real source controllers");
        EspNativeReadOnlyPreview.ChestPlan draw=
            nativePreview.chestPlan(result,0.25,1.0,2.0);
        check(draw.draws.size()==2&&draw.worldToken==result.worldToken&&
              draw.tick==result.tick&&draw.scale==0.5f,
              "source plan receipt");
        check(draw.draws.get(0).blockId==54&&draw.draws.get(1).blockId==130&&
              Math.abs(draw.draws.get(0).centerX-1.25)<0.00001 &&
              Math.abs(draw.draws.get(0).centerY+0.5)<0.00001 &&
              Math.abs(draw.draws.get(0).centerZ+1.5)<0.00001,
              "R303 JNI camera relative centers");
        EspNativeReadOnlyPreview.EntityPlan entities=
            nativePreview.entityPlan(result,0.25,0.5,1.0,0.5);
        check(entities.items.size()==1&&entities.players.size()==1&&
              entities.worldToken==result.worldToken&&
              entities.tick==result.tick,
              "R304 source native Item and Player render rows");
        check(entities.items.get(0).itemId==264&&
              entities.items.get(0).quantity==3&&
              Math.abs(entities.items.get(0).x-0.75)<0.00001 &&
              Math.abs(entities.items.get(0).y+0.5)<0.00001 &&
              Math.abs(entities.items.get(0).z+1.0)<0.00001,
              "R304 native item coordinates and source count");
        check(Math.abs(entities.players.get(0).x-1.75)<0.00001&&
              Math.abs(entities.players.get(0).health-(12f/22f))<0.00001&&
              Math.abs(entities.players.get(0).absorption-0.2f)<0.00001&&
              entities.players.get(0).hurtTime==1,
              "R304 native player health, absorption and hurt");
        refuseArgument(new Runnable(){public void run(){
            nativePreview.entityPlan(result,0,0,0,-0.1);
        }},"R304 negative partialTicks");
        refuseArgument(new Runnable(){public void run(){
            nativePreview.entityPlan(result,0,0,0,Double.NaN);
        }},"R304 NaN partialTicks");
        refuse(new Runnable(){public void run(){nativePreview.observe(one,10,10,10);}},
               "replay denied");
        f.drops.add(new EspWorldSnapshotSampler.Drop(at(1.1,0,0),at(1.1,0,0),
            264,4,false,true,true));
        f.players.add(new EspWorldSnapshotSampler.Player(
            UUID.fromString("00000000-0000-0000-0000-000000000003"),
            at(4,0,0),at(4,0,0),false,false,20,20,0,0));
        EspWorldSnapshotSampler.Receipt updated=javaSampler.observe(f,11);
        EspNativeReadOnlyPreview.Result updatedNative=
            nativePreview.observe(updated,10,10,10);
        EspNativeReadOnlyPreview.EntityPlan merged=
            nativePreview.entityPlan(updatedNative,0,0,0,1);
        check(merged.items.size()==2&&merged.items.get(0).quantity==7&&
              merged.items.get(1).quantity==0,
              "R304 source list-order count merge, zeroed contributing row");
        check(merged.players.size()==2&&
              Math.abs(merged.players.get(0).x-4)<0.000001&&
              Math.abs(merged.players.get(1).x-2)<0.000001,
              "R304 farthest-first native PlayerESP source ordering");
        refuse(new Runnable(){public void run(){
            nativePreview.entityPlan(result,0,0,0,1);
        }},"R304 stale tick entity plan");
        f.world=new Object();
        EspWorldSnapshotSampler.Receipt newWorld=javaSampler.observe(f,0);
        EspNativeReadOnlyPreview.Result next=nativePreview.observe(newWorld,10,10,10);
        refuse(new Runnable(){public void run(){
            nativePreview.chestPlan(result,0,0,0);
        }},"R303 old world draw refused");
        refuse(new Runnable(){public void run(){
            nativePreview.entityPlan(updatedNative,0,0,0,1);
        }},"R304 old world entity plan refused");
        refuseArgument(new Runnable(){public void run(){
            nativePreview.chestPlan(next,Double.NaN,0,0);
        }},"R303 nonfinite camera");
        check(next.generation>result.generation&&next.worldToken!=result.worldToken,
              "native world swap");
        final boolean[] offThread={false};
        Thread stranger=new Thread(new Runnable(){
            public void run(){
                try{nativePreview.observe(newWorld,10,10,10);}
                catch(IllegalStateException expected){offThread[0]=true;}
            }
        });
        stranger.start();stranger.join();
        check(offThread[0],"native session thread");
        final boolean[] drawForeign={false};
        Thread drawStranger=new Thread(new Runnable(){
            public void run(){
                try{nativePreview.chestPlan(next,0,0,0);}
                catch(IllegalStateException expected){drawForeign[0]=true;}
            }
        });
        drawStranger.start();drawStranger.join();
        check(drawForeign[0],"R303 draw owner thread");
        final boolean[] entityForeign={false};
        Thread entityStranger=new Thread(new Runnable(){
            public void run(){
                try{nativePreview.entityPlan(next,0,0,0,0.5);}
                catch(IllegalStateException expected){entityForeign[0]=true;}
            }
        });
        entityStranger.start();entityStranger.join();
        check(entityForeign[0],"R304 entity plan owner thread");
        nativePreview.close();javaSampler.close();
        System.out.println("R301_JAVA8_REAL_NATIVE_THREE_ESP_CONTROLLERS_PASS");
    }
}
