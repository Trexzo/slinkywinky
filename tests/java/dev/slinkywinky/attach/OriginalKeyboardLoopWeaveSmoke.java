package dev.slinkywinky.attach;

import java.util.Arrays;
import java.util.List;

/** R109 synthetic ASM-only denials, no game, JNI, agent, input or Java Attach. */
public final class OriginalKeyboardLoopWeaveSmoke {
    private static void require(boolean ok,String detail) {
        if(!ok)throw new AssertionError(detail);
    }
    private static boolean denied(byte[] target,String sha256) throws Exception {
        try {
            OriginalKeyboardLoopWeaveAudit.weave(target,sha256);
            return false;
        }catch(RuntimeException expected){return true;}
    }
    public static void main(String[] args) throws Exception {
        byte[] original=KeyboardInputCallsiteAuditSmoke.fixture(0);
        byte[] copy=original.clone();
        String hash=RenderCallsiteAudit.sha256Hex(original);
        byte[] rewritten=OriginalKeyboardLoopWeaveAudit.weave(original,hash);
        require(Arrays.equals(copy,original),"Rewriter mutated its input");
        List<String> before=OriginalKeyboardLoopWeaveAudit.trace(original);
        List<String> after=OriginalKeyboardLoopWeaveAudit.trace(rewritten);
        require(before.size()==after.size(),"Rewriter changed trace length");
        int diff=0;
        for(int i=0;i<before.size();i++)
            if(!before.get(i).equals(after.get(i))) {
                diff++;
                require(before.get(i).contains("Keyboard:next:()Z") &&
                        after.get(i).contains("OfflineOnlyKeyboardOwner:next:()Z"),
                        "Unexpected instruction replacement");
            }
        require(diff==1,"Expected one test-only callsite substitution");
        require(denied(original,"0000"),"Wrong class fingerprint admitted");
        require(denied(rewritten,RenderCallsiteAudit.sha256Hex(rewritten)),
                "Already rewritten class admitted");
        for(int variant=1;variant<=9;variant++) {
            byte[] bad=KeyboardInputCallsiteAuditSmoke.fixture(variant);
            require(denied(bad,RenderCallsiteAudit.sha256Hex(bad)),
                    "Changed original keyboard path admitted: variant="+variant);
        }
        require(denied(new byte[]{1,2,3},"0000"),"Truncated bytes admitted");
        System.out.println("R109_SYNTHETIC_KEYBOARD_LOOP_WEAVE_ONE_CHANGE_AND_NEGATIVE_CONTROLS_PASS");
        System.out.println("R109_NO_PRODUCTION_JAVA_AGENT_OR_GAME_INPUT");
    }
    private OriginalKeyboardLoopWeaveSmoke(){}
}
