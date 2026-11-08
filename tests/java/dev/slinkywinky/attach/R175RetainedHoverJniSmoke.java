package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R175: disposable Java8 -> JNI -> genuine retained C++ WGL passive
 * numeric hover. No click transaction is emitted by the hover function.
 * An original Minecraft runtime can supply position to this SAME model
 * only after the exact original WGL window/foreground has been verified.
 */
public final class R175RetainedHoverJniSmoke {
    private static native boolean observeHover(int x,int y,
                         boolean foreground,int width,int height);
    private static native int[] hoverReceipt();
    // Returns aggregate counters ONLY, backed by the real first-party
    // C++ retained WGL model in the disposable JNI test DLL.
    private static native int[] hoverEvidence();
    private static void check(boolean value,String reason) {
        if(!value)throw new AssertionError("R175 "+reason);
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1 &&
              args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Only disposable, separate JNI WGL test DLL permitted");
        System.load(args[0]);
        check(!observeHover(10,10,true,676,630) && hoverReceipt()==null &&
              hoverEvidence()==null,
              "Hover was admitted without an actual retained native UI");
        check(R166RetainedPointerJniSmoke.begin(),
              "Disposable WGL owner creation failed");
        try {
            check(R167RecoveredSliderJniSmoke.navigate(2),
                  "Native Auto Block source row could not be focused");
            check(R167RecoveredSliderJniSmoke.navigate(4),
                  "Native recovered slider page could not expand");
            final int[] slider=R167RecoveredSliderJniSmoke.sliderTarget(500);
            check(slider!=null && slider.length==2,
                  "Recovered slider hit was not source validated");
            final int x=slider[0],y=slider[1];
            final String before=R166RetainedPointerJniSmoke.profile();
            check(before!=null && before.contains("\nblock_range="),
                  "Native profile snapshot missing");
            check(hoverReceipt()==null,"Default numeric hover was not empty");
            int[] start=hoverEvidence();
            check(start!=null && start.length==5 && start[0]==1 &&
                  start[1]==0 && start[2]==0 && start[3]==0 && start[4]==0,
                  "Initial native hover audit counters not zeroed");

            check(!observeHover(x,y,true,677,630) && hoverReceipt()==null,
                  "A wrong viewport reached native retained hover");
            final AtomicBoolean foreignDenied=new AtomicBoolean(false);
            Thread alien=new Thread(new Runnable(){
                @Override public void run(){
                    foreignDenied.set(
                        !observeHover(x,y,true,676,630) &&
                        hoverReceipt()==null);
                }
            });
            alien.start();alien.join();
            check(foreignDenied.get(),"Foreign JNI thread obtained WGL hover");

            check(observeHover(x,y,true,676,630),
                  "Source slider hover refused");
            int[] seen=hoverReceipt();
            check(seen!=null && seen.length==2 && seen[0]==1 &&
                  seen[1]>=0,"Native source numeric hover not retained");
            int[] first=hoverEvidence();
            check(first!=null && first.length==5 && first[0]==1 &&
                  first[1]==1 && first[2]==1 && first[3]==1 && first[4]==0,
                  "Source numeric hover did not increment exact WGL counters");
            check(observeHover(x,y,true,676,630),
                  "Repeated identical source hover should be idempotent");
            int[] repeat=hoverEvidence();
            check(repeat!=null && repeat[1]==2 && repeat[2]==2 &&
                  repeat[3]==2 && repeat[4]==0,
                  "Repeat source hover audit did not count accepted samples");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Native GL redraw failed with numeric tooltip present");
            seen=hoverReceipt();
            check(seen!=null && seen[0]==1,
                  "Numeric hover vanished after native GL redraw");
            check(before.equals(R166RetainedPointerJniSmoke.profile()),
                  "Physical hover unexpectedly changed engine profile");

            check(observeHover(-20,-20,true,676,630) &&
                  hoverReceipt()==null,
                  "Leaving viewport failed to clear source hover");
            check(observeHover(x,y,true,676,630) &&
                  hoverReceipt()!=null,
                  "Re-entering source slider failed to restore hover");
            check(observeHover(x,y,false,676,630) &&
                  hoverReceipt()==null,
                  "Lost foreground left stale visual hover active");
            check(observeHover(x,y,true,676,630) &&
                  hoverReceipt()!=null,
                  "Foreground recovery failed to restore passive hover");

            // A pending synthetic click transaction blocks hover mutation.
            // This does not assert exclusive original-game ownership.
            check(R166RetainedPointerJniSmoke.pointer(
                      0,x,y,2000,true,true,676,630),
                  "Synthetic press setup refused");
            check(!observeHover(-20,-20,true,676,630),
                  "Held gesture allowed frame-changing passive hover");
            check(hoverReceipt()!=null,
                  "Denied hover mutated a held pointer frame");
            check(R166RetainedPointerJniSmoke.pointer(
                      3,x,y,2010,true,true,676,630),
                  "Held source transaction cancellation refused");
            check(observeHover(-20,-20,true,676,630) &&
                  hoverReceipt()==null,
                  "Cancelled gesture could not clear visual hover");
            check(before.equals(R166RetainedPointerJniSmoke.profile()),
                  "Hover/cancel emitted a real configuration change");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Post-cancel WGL redraw failed");
            System.out.println("R175_JAVA8_JNI_RETAINED_WGL_PASSIVE_NUMERIC_HOVER_PASS");
            System.out.println("R175_FOREGROUND_THREAD_VIEWPORT_HELD_GESTURE_DENIAL_PASS");
            System.out.println("R175_PASSIVE_HOVER_NO_PROFILE_MUTATION_OR_CLICK_DELIVERY");
            System.out.println("R175_NO_ORIGINAL_MINECRAFT_INPUT_HOOK");
        } finally {
            check(R166RetainedPointerJniSmoke.end(),
                  "Disposable native WGL session close failed");
        }
        check(!observeHover(0,0,true,676,630) &&
              hoverReceipt()==null && hoverEvidence()==null,
              "Disposed native WGL owner leaked passive hover");
    }
}
