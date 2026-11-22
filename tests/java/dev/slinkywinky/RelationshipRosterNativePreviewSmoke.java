package dev.slinkywinky;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public final class RelationshipRosterNativePreviewSmoke {
    private static void ck(boolean yes,String what) {
        if(!yes)throw new AssertionError("R282: "+what);
    }
    private static void refused(Runnable r,String what) {
        boolean denied=false;
        try{r.run();}catch(IllegalArgumentException expected){denied=true;}
        catch(IllegalStateException expected){denied=true;}
        ck(denied,what);
    }
    private static final class Fake implements RelationshipRosterSampler.Port {
        Object world=new Object(),owner=new Object();
        final UUID ownerId=new UUID(0,99);
        final List<RelationshipRosterSampler.Observation> list=
            new ArrayList<RelationshipRosterSampler.Observation>();
        Fake(){list.add(p(owner,ownerId,"Local",0));}
        static RelationshipRosterSampler.Observation p(
                Object object,UUID id,String name,double x) {
            return new RelationshipRosterSampler.Observation(
                object,id,name,0,0,x,0,40,false,false,false);
        }
        public boolean onClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object ownerIdentity(){return owner;}
        public UUID ownerUuid(){return ownerId;}
        public List<RelationshipRosterSampler.Observation> players(){return list;}
    }
    public static void main(String[] args)throws Exception {
        final Fake source=new Fake();
        source.list.add(Fake.p(new Object(),new UUID(0x2000,1),"§9Friend",0));
        source.list.add(Fake.p(new Object(),new UUID(0x5000,2),"§cEnemy",0));
        final RelationshipRosterSampler sampler=new RelationshipRosterSampler();
        final RelationshipRosterNativePreview nativePreview=
            new RelationshipRosterNativePreview();
        final RelationshipRosterNativePreview.Settings settings=
            new RelationshipRosterNativePreview.Settings(
                false,2,false,false,0,false);
        final RelationshipRosterSampler.Receipt first=sampler.observe(source,10);
        RelationshipRosterNativePreview.Result result=nativePreview.observe(first,settings);
        ck(result.remote.size()==2 && result.generation==first.generation,
           "actual JNI native event0 result");
        ck(result.remote.get(0).rejected && !result.remote.get(1).rejected,
           "source version-nibble equality semantics, not invented UUID rule");
        ck(result.remote.get(0).uuid.equals(first.remote.get(0).uuid),
           "returned UUID exact");
        refused(new Runnable(){public void run() {
            nativePreview.observe(first,settings);
        }},"same receipt cannot accumulate movement twice");
        final RelationshipRosterNativePreview.Settings moveSettings=
            new RelationshipRosterNativePreview.Settings(
                false,1,false,false,0,false);
        // Moving should bypass mode1 if movement accumulates enough.
        source.list.set(1,Fake.p(new Object(),
            new UUID(0x2000,1),"§9Friend",2.0));
        final RelationshipRosterSampler.Receipt move=sampler.observe(source,11);
        result=nativePreview.observe(move,moveSettings);
        ck(!result.remote.get(0).rejected,"source accumulated movement");
        source.world=new Object();
        final RelationshipRosterSampler.Receipt swapped=sampler.observe(source,0);
        ck(nativePreview.observe(swapped,settings).remote.get(0).rejected,
           "new native world scope resets history");
        refused(new Runnable(){public void run() {
            nativePreview.observe(first,settings);
        }},"old world token refused after swap");
        refused(new Runnable(){public void run() {
            new RelationshipRosterNativePreview.Settings(
                true,4,false,false,0,false);
        }},"malformed UUID mode");
        final boolean[] foreignAccepted={false};
        Thread thread=new Thread(new Runnable(){public void run(){
            try{nativePreview.observe(swapped,settings);foreignAccepted[0]=true;}
            catch(IllegalStateException expected){}
        }});
        thread.start();thread.join();
        ck(!foreignAccepted[0],"foreign Java thread refused");
        nativePreview.close();sampler.close();
        refused(new Runnable(){public void run(){
            nativePreview.observe(swapped,settings);
        }},"closed JNI observer refused");
        System.out.println("R282_JAVA8_NATIVE_ANTIBOT_ROSTER_ROUNDTRIP_PASS");
    }
}
