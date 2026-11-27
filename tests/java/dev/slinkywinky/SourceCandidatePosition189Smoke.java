package dev.slinkywinky;

/** Real JNI e18/e3 decision differs for source XYZ vs Entity.serverPos. */
public final class SourceCandidatePosition189Smoke {
    private static void yes(boolean v,String s){if(!v)throw new AssertionError(s);}
    private static void denied(Runnable r,String s){
        boolean refused=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException ex){refused=true;}
        yes(refused,s);
    }
    private static final class Mcp {
        private double otherPlayerMPX=10,otherPlayerMPY=0,otherPlayerMPZ=0;
        private int serverPosX=-320;
    }
    private static final class Srg {
        private double field_71185_c=10,field_71182_d=0,field_71183_e=0;
    }
    private static final class Obf {
        private double c=10,d=0,e=0;
    }
    private static final class BadType {
        private double otherPlayerMPX=10,otherPlayerMPZ=0;
        private float otherPlayerMPY=0;
    }
    private static final class Invalid {
        private double otherPlayerMPX=Double.NaN,otherPlayerMPY=0,otherPlayerMPZ=0;
    }
    private static final class Huge {
        private double otherPlayerMPX=30000001,otherPlayerMPY=0,otherPlayerMPZ=0;
    }
    private static SourceCandidatePosition189.Point read(Object o){
        Class<?> c=o.getClass();
        return SourceCandidatePosition189.read(o,c,
            SourceCandidateField189.remoteX(c),
            SourceCandidateField189.remoteY(c),
            SourceCandidateField189.remoteZ(c));
    }
    private static boolean hold(double originalTargetX){
        SourceRuntimePreview source=new SourceRuntimePreview();
        try{
            final long world=901,player=902,generation=1,tick=1;
            final int[] bits={1,1,1,0,0,0,0,0,0,0,0,0};
            SourceRuntimePreview.Receipt initial=source.observe(
                world,player,generation,tick,SourceRuntimePreview.LAG_RANGE_TICK,
                bits,new double[]{9,10,originalTargetX,0,0,150});
            yes(initial.sourceEvent==18&&initial.at(3)==1,
                "Actual native source Lag Range e18 active");
            SourceRuntimePreview.Receipt packet=source.observe(
                world,player,generation,tick,SourceRuntimePreview.LAG_RANGE_PACKET,
                new int[]{1,1,1,1},new double[]{0,8,0,0});
            yes(packet.sourceEvent==3&&packet.at(2)==1,"Actual native e3 dispatched");
            return packet.at(3)==1;
        }finally{source.close();}
    }
    public static void main(String[] args){
        Mcp o=new Mcp();
        SourceCandidatePosition189.Point point=read(o);
        yes(point.x==10&&point.y==0&&point.z==0,
            "Candidate stores private OtherPlayerMP XYZ");
        yes(o.serverPosX/32d==-10&&o.serverPosX/32d!=point.x,
            "Original vanilla serverPos differs from candidate");
        yes(read(new Srg()).x==10&&read(new Obf()).x==10,
            "Typed MCP/SRG/obf original variants");
        yes(hold(point.x),"Actual native e18/e3 holds toward source +10");
        yes(!hold(o.serverPosX/32d),
            "Wrong prior vanilla position -10 reverses actual native decision");
        denied(()->read(new BadType()),"Wrong primitive mapping rejected");
        denied(()->read(new Invalid()),"NaN source position denied");
        denied(()->read(new Huge()),"Unbounded coordinate denied");
        denied(()->SourceCandidatePosition189.read(o,Srg.class,
                SourceCandidateField189.remoteX(Srg.class),
                SourceCandidateField189.remoteY(Srg.class),
                SourceCandidateField189.remoteZ(Srg.class)),
            "No other player's reflected owner admitted");
        System.out.println("R356_NATIVE_LAG_TARGET_POSITION_SOURCE_PARITY_PASS");
    }
}
