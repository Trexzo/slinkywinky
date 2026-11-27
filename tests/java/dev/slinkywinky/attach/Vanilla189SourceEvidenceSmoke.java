package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189SourceEvidence;
import java.lang.reflect.Field;

/** Synthetic strict-name field capture, cache reuse, teardown and negatives. */
public final class Vanilla189SourceEvidenceSmoke {
    static void yes(boolean value,String what){
        if(!value)throw new AssertionError(what);
    }
    static void rejects(Runnable attempt,String why){
        boolean refused=false;
        try{attempt.run();}
        catch(IllegalStateException|IllegalArgumentException ex){refused=true;}
        yes(refused,why);
    }
    static Vanilla189SourceEvidence.Sample read(Object player,Object target){
        try{return Vanilla189SourceEvidence.read(player,target);}
        catch(ReflectiveOperationException failure){
            throw new IllegalStateException("Source candidate mapping refused",failure);
        }
    }
    static void set(Object subject,String name,Object value){
        try{
            Field f=subject.getClass().getField(name);
            f.set(subject,value);
        }catch(Exception fail){throw new IllegalStateException(fail);}
    }
    public static void main(String[] args)throws Exception{
        Object player=Class.forName("pr").newInstance();
        Object target=Class.forName("bex").newInstance();
        yes(read(player,new Object())==null,"Nonplayer ray is not a candidate");
        yes(read(null,target)==null,"No player means no candidate");
        Vanilla189SourceEvidence.Sample first=read(player,target);
        yes(first!=null&&first.x==10&&first.y==5&&first.z==-4&&
            first.lastAttackTick==18,"Exact original field evidence captured");
        Field c=target.getClass().getDeclaredField("c");
        c.setDouble(target,11);
        yes(first.x==10&&read(player,target).x==11,
            "Receipt owns immutable primitive copy; cache reads latest field");
        c.setDouble(target,Double.NaN);
        rejects(()->read(player,target),"Nonfinite target cannot reach source JNI");
        c.setDouble(target,30000001);
        rejects(()->read(player,target),"Unbounded source target rejected");
        c.setDouble(target,10);
        Vanilla189SourceEvidence.release();
        yes(read(player,target).lastAttackTick==18,
            "Release cleans mapping and allows fresh strict read");
        Vanilla189SourceEvidence.release();
        System.out.println("R358_VANILLA189_ORIGINAL_SOURCE_FIELDS_READONLY_PASS");
    }
}
