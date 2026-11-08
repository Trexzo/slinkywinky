package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.RendererWglEvidence;
import java.lang.reflect.Method;
public final class R158ProfileReceiptSmoke {
    private static void check(boolean condition) {
        if(!condition)throw new AssertionError("R158 Java profile receipt");
    }
    private static boolean valid(Method verify,String receipt,boolean block)
            throws Exception {
        return ((Boolean)verify.invoke(null,receipt,block)).booleanValue();
    }
    public static void main(String[] args) throws Exception {
        final Method verify=RendererWglEvidence.class.getDeclaredMethod(
            "validProfileReceipt",String.class,boolean.class);
        verify.setAccessible(true);
        final String good=
            "# SlinkyWinky standalone profile; no secrets or account data\n"+
            "version=1\n"+
            "auto_clicker_enabled=false\n"+
            "auto_block_enabled=true\n"+
            "sprint_enabled=true\n"+
            "backtrack_ms=100\n";
        check(valid(verify,good,true));
        check(!valid(verify,good,false));
        check(!valid(verify,good.replace("sprint_enabled=true\n",""),true));
        check(!valid(verify,good.replace("version=1","version=2"),true));
        check(!valid(verify,good.replace("backtrack_ms=100\n",""),true));
        check(!valid(verify,good.substring(0,good.length()-1),true));
        check(!valid(verify,"",true));
        check(!valid(verify,null,true));
        System.out.println("R158_JAVA8_PROFILE_RECEIPT_CONSISTENCY_PASS");
    }
}
