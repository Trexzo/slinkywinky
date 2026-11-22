package dev.slinkywinky;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.InvalidPathException;

/** R285: independent TEMP-only roster source admission, no game API. */
public final class RosterIsolationGate {
    private RosterIsolationGate(){}
    public static boolean admitted(boolean sprintObservation,
                                   boolean actionTest,boolean autoWorldTest,
                                   String gameDir,String tmpDir,String username) {
        if(!sprintObservation || actionTest || autoWorldTest ||
           gameDir==null || tmpDir==null ||
           !"R278Offline".equals(username))
            return false;
        try {
            Path temp=Paths.get(tmpDir).toRealPath();
            Path home=Paths.get(gameDir).toRealPath();
            if(!Files.isDirectory(temp) || !Files.isDirectory(home) ||
               !home.startsWith(temp))
                return false;
            // Prevent a similarly named parent, nested directory or
            // symlink outside the true temporary root from bypassing
            // the isolated gameDir admission.
            Path relative=temp.relativize(home);
            if(relative.getNameCount()!=1)return false;
            String folder=relative.getFileName().toString();
            return folder.startsWith("slinkywinky-r278-independent-forge-") &&
                   folder.length()>"slinkywinky-r278-independent-forge-".length();
        }catch(IOException | InvalidPathException | SecurityException rejected) {
            return false;
        }
    }
}
