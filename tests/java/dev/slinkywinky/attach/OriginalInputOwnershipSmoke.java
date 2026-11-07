package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.OriginalKeyboardBridgeWeaver;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.zip.ZipFile;

/**
 * R119 fixture tests always run. An optional ORIGINAL 1.8.9 jar argument
 * additionally proves that queue-only weaving does not remove live game
 * keybinding routes. No game process, agent, native hook or input is used.
 */
public final class OriginalInputOwnershipSmoke {
    private OriginalInputOwnershipSmoke(){}
    private static void require(boolean valid,String message) {
        if(!valid)throw new AssertionError(message);
    }
    private static boolean denied(Runnable test) {
        try {test.run();return false;}
        catch(IllegalArgumentException expected) {return true;}
        catch(IllegalStateException expected) {return true;}
    }
    private static void synthetic() {
        byte[] fake=KeyboardInputCallsiteAuditSmoke.fixture(0);
        final OriginalInputOwnershipAudit.Report report=
            OriginalInputOwnershipAudit.inspectCandidate(fake);
        require(report.targetMethods==1 && report.keyboardNext==1,
                "Test fixture queue was not inspected");
        require(report.bindStateBeforeQueue==1 &&
                report.bindPressBeforeQueue==1 &&
                report.bindStateAfterQueue==1 &&
                report.bindPressAfterQueue==1,
                "Fixture pre/post gameplay routes not recognized");
        require(report.queueOnlyHasResidualGamePaths(),
                "Keyboard.next-only rewrite falsely deemed exclusive");
        require(denied(new Runnable(){
            @Override public void run(){report.refuseQueueOnlyModal();}
        }),"Residual game input admitted to modal");
        require(denied(new Runnable() {
            @Override public void run(){
                OriginalInputOwnershipAudit.inspectPinned(fake);
            }
        }),"Synthetic class passed original byte admission");
        require(denied(new Runnable() {
            @Override public void run() {
                OriginalInputOwnershipAudit.inspectCandidate(new byte[]{1,2,3});
            }
        }),"Truncated original game class admitted");
        byte[] bad=fake.clone();bad[0]=0;
        require(denied(new Runnable() {
            @Override public void run() {
                OriginalInputOwnershipAudit.inspectCandidate(bad);
            }
        }),"Broken class magic accepted");
        System.out.println(
            "R119_SYNTHETIC_PRE_AND_POST_KEYBINDING_MODAL_DENIAL_PASS");
        System.out.println("R119_UNVERIFIED_ORIGINAL_AND_MALFORMED_CLASS_DENIAL_PASS");
    }
    private static void original(String originalJar) throws Exception {
        Path path=Paths.get(originalJar);
        require(path.isAbsolute() && Files.isRegularFile(path),
                "Expected pinned original 1.8.9 jar");
        // Independently enforce pinned JAR size & SHA-1 before unpacking.
        RenderCallsiteAudit.main(new String[]{originalJar});
        byte[] raw;
        try(ZipFile zip=new ZipFile(path.toFile())) {
            raw=RenderCallsiteAudit.entry(zip,"ave.class");
        }
        byte[] original=raw.clone();
        final OriginalInputOwnershipAudit.Report r=
            OriginalInputOwnershipAudit.inspectPinned(raw);
        require(Arrays.equals(raw,original),"Audit mutated original source");
        require(r.rawKeyDown==17 && r.mouseNext==1 &&
                r.heldBindingPolls==13 && r.pressBindingPolls==3,
                "Original nonqueue control paths disappeared");
        require(r.queueOnlyHasResidualGamePaths(),"Raw source falsely safe");
        require(denied(new Runnable() {
            @Override public void run() {r.refuseQueueOnlyModal();}
        }),"Raw original admitted unsafe queue-only hook");

        // Existing R111 exact source-pinned weaver is intentionally
        // MEMORY ONLY: its single-call change leaves every other original
        // keyboard/mouse/binding callsite intact. This is the direct
        // negative control for attempting to enable it in the JVM.
        byte[] rewritten=OriginalKeyboardBridgeWeaver.weavePinnedOriginal(raw);
        final OriginalInputOwnershipAudit.Report after=
            OriginalInputOwnershipAudit.inspectCandidate(rewritten);
        require(after.keyboardNext==0 && after.bridgedNext==1,
                "One-call source weave not inspected");
        require(after.rawKeyDown==r.rawKeyDown &&
                after.mouseNext==r.mouseNext &&
                after.heldBindingPolls==r.heldBindingPolls &&
                after.pressBindingPolls==r.pressBindingPolls &&
                after.bindStateBeforeQueue==r.bindStateBeforeQueue &&
                after.bindStateAfterQueue==r.bindStateAfterQueue &&
                after.bindPressBeforeQueue==r.bindPressBeforeQueue &&
                after.bindPressAfterQueue==r.bindPressAfterQueue,
                "Unexpected control pathway moved during in-memory rewrite");
        require(after.queueOnlyHasResidualGamePaths(),
                "Queue-only patch incorrectly certified gameplay isolation");
        require(denied(new Runnable(){
            @Override public void run(){after.refuseQueueOnlyModal();}
        }),"Queue-only rewrite allowed modal gameplay contamination");
        require(Arrays.equals(raw,original),
                "Original source bytes changed during audit or rewrite");
        require(denied(new Runnable() {
            @Override public void run(){
                OriginalInputOwnershipAudit.inspectPinned(rewritten);
            }
        }),"Rewritten source was falsely authenticated as vanilla");
        byte[] tampered=raw.clone();tampered[tampered.length-1]^=1;
        require(denied(new Runnable(){
            @Override public void run(){
                OriginalInputOwnershipAudit.inspectPinned(tampered);
            }
        }),"Altered original class fingerprint admitted");
        System.out.println("R119_PINNED_VANILLA_S_INPUT_PATHS "+
                           r.summary());
        System.out.println(
            "R119_QUEUE_ONLY_BRIDGE_LEAVES_MOUSE_KEYDOWN_AND_HELD_KEYBINDS_PASS");
        System.out.println(
            "R119_ORIGINAL_SOURCE_ANNOTATION_NEGATIVE_CONTROLS_PASS");
        System.out.println("R119_NO_LIVE_GAME_INPUT_HOOK_OR_MODAL_ADMISSION");
    }
    public static void main(String[] args)throws Exception {
        require(args.length<=1,"At most one original jar argument");
        synthetic();
        if(args.length==1)original(args[0]);
    }
}
