package dev.slinkywinky;
import java.lang.reflect.Field;

/** Same three candidate namespaces, exact types and fail-closed negatives. */
public final class SourceCandidateField189Smoke {
    static final class Mcp {
        private int lastAttackerTime=37;
        private double otherPlayerMPX=10,otherPlayerMPY=20,otherPlayerMPZ=30;
    }
    static final class Srg {
        private int field_142016_bo=37;
        private double field_71185_c=10,field_71182_d=20,field_71183_e=30;
    }
    static final class Obf189 {
        private int bl=37;
        private double c=10,d=20,e=30;
    }
    static final class WrongType {
        private long lastAttackerTime=37;
        private float otherPlayerMPX=10;
    }
    static final class Mixed {
        private int lastAttackerTime=37;
        private int field_142016_bo=38;
    }
    static final class WrongObfVariant {
        private int bo=37;
        private double f=10;
    }
    static void yes(boolean value,String what){
        if(!value)throw new AssertionError(what);
    }
    static void positive(Class<?> c)throws Exception{
        Field attack=SourceCandidateField189.lastAttackerTime(c);
        Field x=SourceCandidateField189.remoteX(c);
        Field y=SourceCandidateField189.remoteY(c);
        Field z=SourceCandidateField189.remoteZ(c);
        yes(attack!=null&&x!=null&&y!=null&&z!=null,
            "All four candidate fields resolve within a single 1.8.9 namespace");
        Object owner=c.newInstance();
        yes(attack.getInt(owner)==37,"Exact last attack int");
        yes(x.getDouble(owner)==10&&y.getDouble(owner)==20&&z.getDouble(owner)==30,
            "Typed remote target coordinate triplet");
    }
    public static void main(String[] args)throws Exception{
        positive(Mcp.class);positive(Srg.class);positive(Obf189.class);
        yes(SourceCandidateField189.lastAttackerTime(WrongType.class)==null,
            "Wrong attack field primitive type refused");
        yes(SourceCandidateField189.remoteX(WrongType.class)==null,
            "Wrong remote coordinate type refused");
        yes(SourceCandidateField189.lastAttackerTime(Mixed.class)==null,
            "Conflicting namespaces cannot pick arbitrary field");
        yes(SourceCandidateField189.lastAttackerTime(WrongObfVariant.class)==null,
            "Older-version attack obf name cannot be guessed for 1.8.9");
        yes(SourceCandidateField189.remoteZ(WrongObfVariant.class)==null,
            "Older-version remote mapping cannot be guessed for 1.8.9");
        yes(SourceCandidateField189.lastAttackerTime(Object.class)==null,
            "No phantom field in unknown owner");
        boolean invalid=false;
        try{SourceCandidateField189.remoteX(null);}
        catch(IllegalArgumentException expected){invalid=true;}
        yes(invalid,"Null owner rejected");
        System.out.println("R355_SOURCE_MINECRAFT_189_PRIVATE_FIELD_PARITY_PASS");
    }
}
