package dev.slinkywinky;
import java.util.*;

public final class RelationshipRosterSamplerSmoke {
    private static void check(boolean ok,String what) {
        if(!ok)throw new AssertionError("R276: "+what);
    }
    private static void denied(Runnable r,String what) {
        boolean error=false;
        try{r.run();}catch(IllegalStateException expected){error=true;}
        catch(IllegalArgumentException expected){error=true;}
        check(error,what);
    }
    private static final class Fake implements RelationshipRosterSampler.Port {
        Object world=new Object(),owner=new Object();
        UUID ownerId=new UUID(0,99);
        boolean client=true,swapDuringRead=false;
        final List<RelationshipRosterSampler.Observation> list=
            new ArrayList<RelationshipRosterSampler.Observation>();
        Fake(){list.add(p(owner,ownerId,"Local"));}
        static RelationshipRosterSampler.Observation p(Object id,UUID uuid,String n){
            return new RelationshipRosterSampler.Observation(
                id,uuid,n,0,0,1,0,25,false,false,true);
        }
        public boolean onClientThread(){return client;}
        public Object worldIdentity(){return world;}
        public Object ownerIdentity(){return owner;}
        public UUID ownerUuid(){return ownerId;}
        public List<RelationshipRosterSampler.Observation> players(){
            if(swapDuringRead){world=new Object();swapDuringRead=false;}
            return list;
        }
    }
    public static void main(String[] args)throws Exception {
        final RelationshipRosterSampler s=new RelationshipRosterSampler();
        final Fake f=new Fake();
        UUID a=new UUID(0x2000,1),b=new UUID(0x5000,2);
        Object one=new Object(),two=new Object();
        f.list.add(Fake.p(one,a,"Same"));
        f.list.add(Fake.p(two,b,"Same"));
        RelationshipRosterSampler.Receipt r=s.observe(f,1);
        check(r.remote.size()==2 &&
              r.remote.get(0).uuid.equals(a)&&r.remote.get(1).uuid.equals(b),
              "ordered UUIDs disambiguate equal names");
        check(r.worldToken>0&&r.ownerToken>0&&r.worldToken!=r.ownerToken,
              "positive owner scope tokens");
        check(s.observe(f,1).worldToken==r.worldToken &&
              s.observe(f,2).generation==1,"same owner/tick");
        denied(new Runnable(){public void run(){s.observe(f,1);}},"stale tick");
        f.list.add(Fake.p(new Object(),a,"Duplicate"));
        denied(new Runnable(){public void run(){s.observe(f,3);}},"duplicate uuid");
        f.list.remove(f.list.size()-1);
        f.list.add(Fake.p(new Object(),f.ownerId,"Other"));
        denied(new Runnable(){public void run(){s.observe(f,3);}},"owner uuid reused");
        f.list.remove(f.list.size()-1);
        f.list.set(1,new RelationshipRosterSampler.Observation(
            one,a,"Same",Double.NaN,0,0,0,25,false,false,false));
        denied(new Runnable(){public void run(){s.observe(f,3);}},"nonfinite");
        f.list.set(1,Fake.p(one,a,"Same"));
        f.swapDuringRead=true;
        denied(new Runnable(){public void run(){s.observe(f,3);}},"midread world swap");
        check(s.generation()==1,"failed snapshot cannot commit");
        RelationshipRosterSampler.Receipt next=s.observe(f,3);
        check(next.generation==2 && next.worldToken!=r.worldToken,
              "world swap reissues owner token");
        f.owner=new Object();
        f.list.set(0,Fake.p(f.owner,f.ownerId,"Local"));
        check(s.observe(f,0).generation==3,"owner swap resets tick identity");
        f.client=false;
        denied(new Runnable(){public void run(){s.observe(f,1);}},"not client");
        f.client=true;
        final boolean[] otherThreadDenied={false};
        Thread thread=new Thread(new Runnable(){public void run(){
            try{s.observe(f,1);}catch(IllegalStateException expected){
                otherThreadDenied[0]=true;
            }
        }});
        thread.start();thread.join();
        check(otherThreadDenied[0],"different Java thread refused");
        try{r.remote.clear();throw new AssertionError("receipt mutable");}
        catch(UnsupportedOperationException expected){}
        f.list.clear();
        denied(new Runnable(){public void run(){s.observe(f,1);}},"owner absent");
        s.close();
        denied(new Runnable(){public void run(){s.observe(f,1);}},"closed");
        System.out.println("R276_JAVA8_ROSTER_OWNER_RECEIPT_PASS");
    }
}
