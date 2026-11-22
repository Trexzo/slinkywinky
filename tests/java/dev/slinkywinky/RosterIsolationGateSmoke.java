package dev.slinkywinky;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
public final class RosterIsolationGateSmoke {
    private static void ck(boolean yes,String why) {
        if(!yes)throw new AssertionError("R285 "+why);
    }
    private static boolean admit(String dir,String tmp,String user,
                                 boolean sprint,boolean action,boolean autoWorld) {
        return RosterIsolationGate.admitted(
            sprint,action,autoWorld,dir,tmp,user);
    }
    public static void main(String[] args)throws IOException {
        Path root=Files.createTempDirectory("r285-roster-gate-").toRealPath();
        Path allowed=Files.createDirectory(root.resolve(
            "slinkywinky-r278-independent-forge-123"));
        Path bare=Files.createDirectory(root.resolve(
            "slinkywinky-r278-independent-forge-"));
        Path wrong=Files.createDirectory(root.resolve(
            "slinkywinky-r237-independent-forge-123"));
        Path nested=Files.createDirectory(allowed.resolve("nested"));
        Path parent=Files.createDirectory(root.resolve("outer"));
        Path fakePrefix=Files.createDirectory(parent.resolve(
            "slinkywinky-r278-independent-forge-123"));
        final String tmp=root.toString(),user="R278Offline";
        try {
            ck(admit(allowed.toString(),tmp,user,true,false,false),
               "valid independent TEMP gameDir");
            ck(!admit(bare.toString(),tmp,user,true,false,false),
               "prefix without unique instance");
            ck(!admit(wrong.toString(),tmp,user,true,false,false),
               "old Clutch prefix is not implicit admission");
            ck(!admit(nested.toString(),tmp,user,true,false,false),
               "nested folder denied");
            ck(!admit(fakePrefix.toString(),tmp,user,true,false,false),
               "similarly named grandchild denied");
            ck(!admit(allowed.toString(),tmp,"R237Offline",true,false,false),
               "wrong dummy identity");
            ck(!admit(allowed.toString(),tmp,user,false,false,false),
               "observer required");
            ck(!admit(allowed.toString(),tmp,user,true,true,false),
               "action trial incompatible");
            ck(!admit(allowed.toString(),tmp,user,true,false,true),
               "auto-world trial incompatible");
            ck(!admit(root.resolve("missing").toString(),tmp,user,true,false,false),
               "absent gameDir cannot pass");
            ck(!admit(allowed.toString(),root.resolve("missing").toString(),
                      user,true,false,false),
               "absent TEMP root cannot pass");
        }finally{
            Files.delete(nested);Files.delete(allowed);
            Files.delete(bare);Files.delete(wrong);
            Files.delete(fakePrefix);Files.delete(parent);
            Files.delete(root);
        }
        System.out.println("R285_JAVA8_INDEPENDENT_ROSTER_TEMP_GATE_PASS");
    }
}
