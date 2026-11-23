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
        refuse(new Runnable(){public void run(){nativePreview.observe(one,10,10,10);}},
               "replay denied");
        f.world=new Object();
        EspWorldSnapshotSampler.Receipt newWorld=javaSampler.observe(f,0);
        EspNativeReadOnlyPreview.Result next=nativePreview.observe(newWorld,10,10,10);
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
        nativePreview.close();javaSampler.close();
        System.out.println("R301_JAVA8_REAL_NATIVE_THREE_ESP_CONTROLLERS_PASS");
    }
}
