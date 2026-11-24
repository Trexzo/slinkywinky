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

        // R305: intentionally separate event 0, 18 and world/screen phase
        // admissions against the actual native C++ R297-R299 controllers.
        final Fixture sourceFixture=new Fixture();
        final EspWorldSnapshotSampler phasedSampler=new EspWorldSnapshotSampler();
        final EspNativeReadOnlyPreview phased=new EspNativeReadOnlyPreview(true);
        final EspWorldSnapshotSampler.Receipt tick0=phasedSampler.observe(sourceFixture,10);
        final EspNativeReadOnlyPreview.Result staged=phased.observe(tick0,10,10,10);
        check(staged.chestRows==-1&&staged.itemRows==-1&&staged.playerRows==-1,
              "R305 native staging does NOT run any controller");
        refuse(new Runnable(){public void run(){
            phased.sourceEvent(staged,18);
        }},"R305 event18-before-event0 rejected");
        refuse(new Runnable(){public void run(){
            phased.chestPlan(staged,0,0,0);
        }},"R305 plan denied before collection");
        final EspNativeReadOnlyPreview.Result chest=phased.sourceEvent(staged,0);
        check(chest.chestRows==2&&chest.itemRows==-1&&chest.playerRows==-1,
              "R305 real Chest event0 only");
        refuse(new Runnable(){public void run(){
            phased.sourceEvent(chest,0);
        }},"R305 duplicate event0 rejected");
        final EspNativeReadOnlyPreview.Result collected=phased.sourceEvent(chest,18);
        check(collected.chestRows==2&&collected.itemRows==1&&
              collected.playerRows==1,"R305 Item/Player event18");
        refuse(new Runnable(){public void run(){
            phased.entityPlan(collected,0,0,0,.5);
        }},"R305 render requires world event6");
        refuse(new Runnable(){public void run(){
            phased.sourceEvent(collected,7);
        }},"R305 event7-before-event6 rejected");
        final EspNativeReadOnlyPreview.Result world6=phased.sourceEvent(collected,6);
        check(phased.chestPlan(world6,0,0,0).draws.size()==2 &&
              phased.entityPlan(world6,0,0,0,.5).players.size()==1,
              "R305 real C++ rows readable after event6");
        refuse(new Runnable(){public void run(){
            phased.playerScreenPlan(world6,0,0,0,1);
        }},"R306 native screen query rejected before event7");
        final EspNativeReadOnlyPreview.Result screen7=phased.sourceEvent(world6,7);
        final EspNativeReadOnlyPreview.ScreenPlan screenRows=
            phased.playerScreenPlan(screen7,0,0,0,1);
        check(screenRows.players.size()==1&&
              Math.abs(screenRows.players.get(0).x-2)<0.000001 &&
              Math.abs(screenRows.players.get(0).health-(12f/22f))<0.00001,
              "R306 source-ordered native event7 screen rows/health");
        final double[] identity={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        final double[] scale={.2,0,0,0,0,.2,0,0,0,0,.2,0,0,0,0,1};
        final PlayerEspScreenProjection.Frame scene=
            new PlayerEspScreenProjection.Frame(
                screen7.worldToken,screen7.playerToken,
                screen7.generation,screen7.tick,
                identity,scale,new int[]{0,0,200,200},100,100);
        final java.util.List<PlayerEspScreenProjection.Rect> rects=
            scene.project(screenRows);
        check(rects.size()==1&&
              Math.abs(rects.get(0).left-67)<.00001&&
              Math.abs(rects.get(0).right-73)<.00001&&
              Math.abs(rects.get(0).top-32)<.00001&&
              Math.abs(rects.get(0).bottom-50)<.00001,
              "R306 exact column-major projection to scaled GUI rectangle");
        // R307: original eight corners recovered at RenderConstants
        // 1378976..1379168 are +/-.4, vertical 0..2.0, not R306's
        // diagnostic .3/1.8 dimensions. Original projected bounds remain
        // *unclamped* and are admitted by the two-corner rule.
        final java.util.List<PlayerEspScreenProjection.Rect> sourceRects=
            scene.projectSource(screenRows);
        check(sourceRects.size()==1&&
              Math.abs(sourceRects.get(0).left-66)<.0001&&
              Math.abs(sourceRects.get(0).right-74)<.0001&&
              Math.abs(sourceRects.get(0).top-30)<.0001&&
              Math.abs(sourceRects.get(0).bottom-50)<.0001,
              "R307 recovered source eight-corner projected bounds");
        check(PlayerEspScreenProjection.Frame.sourceVisibleBounds(
                -2f,-1f,90f,80f,100f,100f) &&
              !PlayerEspScreenProjection.Frame.sourceVisibleBounds(
                -10f,-10f,110f,110f,100f,100f) &&
              !PlayerEspScreenProjection.Frame.sourceVisibleBounds(
                110f,10f,140f,80f,100f,100f),
              "R307 original top-left OR bottom-right visibility differs from clipping");
        final java.util.List<PlayerEspScreenProjection.Segment> healthSegments=
            PlayerEspScreenProjection.sourceHealthSegments(sourceRects.get(0),1.25f);
        check(healthSegments.size()==3&&
              Math.abs(healthSegments.get(0).x-59.75)<.0001&&
              Math.abs(healthSegments.get(0).y1-30)<.0001&&
              Math.abs(healthSegments.get(0).y2-
                 (50f+(12f/22f)*(30f-50f)))<.0001&&
              healthSegments.get(0).red==0f&&
              healthSegments.get(1).red==1f&&
              Math.abs(healthSegments.get(1).green-.67f)<.00001&&
              Math.abs(healthSegments.get(1).y2-
                (healthSegments.get(0).y2+
                 .2f*(50f-(float)healthSegments.get(0).y2)))<.0001&&
              Math.abs(healthSegments.get(2).red-(1f-12f/22f))<.00001&&
              Math.abs(healthSegments.get(2).green-(12f/22f))<.00001,
              "R307 source black/absorption/health 3-stage bar");
        check(Math.abs(PlayerEspScreenProjection.sourceHurtMix(5,.5f)-.45f)<.00001&&
              PlayerEspScreenProjection.sourceHurtMix(20,0f)==1f&&
              PlayerEspScreenProjection.sourceHurtMix(0,.5f)==0f,
              "R307 source min(10,hurt-partial)*0.1 hurt transition");
        refuseArgument(new Runnable(){public void run(){
            PlayerEspScreenProjection.sourceHealthSegments(sourceRects.get(0),
                                                             Float.NaN);
        }},"R307 source geometry rejects nonfinite line width");

        final PlayerEspScreenProjection.Frame outside=
            new PlayerEspScreenProjection.Frame(
                screen7.worldToken,screen7.playerToken,
                screen7.generation,screen7.tick,
                identity,identity,new int[]{0,0,200,200},100,100);
        check(outside.project(screenRows).isEmpty(),
              "R306 outside viewport geometry conservatively skipped");
        final PlayerEspScreenProjection.Frame wrongWorld=
            new PlayerEspScreenProjection.Frame(
                screen7.worldToken+100,screen7.playerToken,
                screen7.generation,screen7.tick,
                identity,scale,new int[]{0,0,200,200},100,100);
        refuse(new Runnable(){public void run(){
            wrongWorld.project(screenRows);
        }},"R306 stale/foreign world projection denied");
        refuse(new Runnable(){public void run(){
            wrongWorld.projectSource(screenRows);
        }},"R307 source projection also denies foreign world");
        refuseArgument(new Runnable(){public void run(){
            phased.playerScreenPlan(screen7,0,0,0,Double.NaN);
        }},"R306 invalid partial frame denied");
        final double[] nanMatrix=identity.clone();
        nanMatrix[0]=Double.NaN;
        refuseArgument(new Runnable(){public void run(){
            new PlayerEspScreenProjection.Frame(
                screen7.worldToken,screen7.playerToken,
                screen7.generation,screen7.tick,
                nanMatrix,scale,new int[]{0,0,200,200},100,100);
        }},"R306 nonfinite matrix denied");
        final EspNativeReadOnlyPreview.Result another6=phased.sourceEvent(screen7,6);
        check(another6.itemRows==1,"R305 repeat render per physics tick");
        sourceFixture.drops.add(new EspWorldSnapshotSampler.Drop(
            at(1.1,0,0),at(1.1,0,0),264,4,false,true,true));
        final EspNativeReadOnlyPreview.Result nextStage=phased.observe(
            phasedSampler.observe(sourceFixture,11),10,10,10);
        refuse(new Runnable(){public void run(){
            phased.sourceEvent(nextStage,6);
        }},"R305 prior tick cannot authorize new render");
        refuseArgument(new Runnable(){public void run(){
            phased.playerScreenPlan(screen7,0,0,0,1);
        }},"R306 old tick screen rows denied");
        final EspNativeReadOnlyPreview.Result next0=phased.sourceEvent(nextStage,0);
        final EspNativeReadOnlyPreview.Result next18=phased.sourceEvent(next0,18);
        check(next18.itemRows==2,"R305 newly collected source rows");
        final EspNativeReadOnlyPreview.Result cleared=phased.sourceEvent(next18,26);
        check(cleared.chestRows==0&&cleared.itemRows==0&&cleared.playerRows==0,
              "R305 original event26 clears collected rows and policy");
        sourceFixture.world=new Object();
        final EspNativeReadOnlyPreview.Result swapStage=phased.observe(
            phasedSampler.observe(sourceFixture,0),10,10,10);
        refuse(new Runnable(){public void run(){
            phased.sourceEvent(next18,18);
        }},"R305 old world receipt revoked");
        final EspNativeReadOnlyPreview.Result swap0=phased.sourceEvent(swapStage,0);
        final EspNativeReadOnlyPreview.Result swap18=phased.sourceEvent(swap0,18);
        check(swap18.generation>next18.generation &&
              swap18.chestRows==2&&swap18.itemRows==2,
              "R305 new world generation collection");
        final boolean[] phaseForeign={false};
        Thread phaser=new Thread(new Runnable(){public void run(){
            try {phased.sourceEvent(swap18,6);}
            catch(IllegalStateException expected){phaseForeign[0]=true;}
        }});
        phaser.start();phaser.join();
        check(phaseForeign[0],"R305 source phase remains owner-thread bound");
        phased.close();phasedSampler.close();
        System.out.println("R307_RECOVERED_PLAYER_ESP_SOURCE_GEOMETRY_HEALTH_PASS");
        System.out.println("R306_JAVA8_REAL_NATIVE_EVENT7_SCREEN_PROJECTION_PASS");
        System.out.println("R305_JAVA8_REAL_NATIVE_ESP_PHASE_0_18_6_7_26_PASS");
        System.out.println("R301_JAVA8_REAL_NATIVE_THREE_ESP_CONTROLLERS_PASS");
    }
}
