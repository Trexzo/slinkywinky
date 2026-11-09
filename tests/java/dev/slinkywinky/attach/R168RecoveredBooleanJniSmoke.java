package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R168: Java8 -> segregated JNI -> actual retained C++ page-3
 * Auto Block / Conditions / Damaged Boolean -> native validated settings.
 * No live Minecraft class input ownership is claimed or installed.
 */
public final class R168RecoveredBooleanJniSmoke {
    private static native int pageOffset();
    private static native boolean page(int action);
    private static native int[] booleanTarget();
    private static void check(boolean okay,String reason) {
        if(!okay)throw new AssertionError("R168 "+reason);
    }
    private static boolean damaged() {
        final String receipt=R166RetainedPointerJniSmoke.profile();
        check(receipt!=null &&
              receipt.startsWith("# SlinkyWinky standalone profile;") &&
              receipt.contains("\nversion=1\n") && receipt.endsWith("\n"),
              "Owner-bound native profile invalid");
        boolean yes=receipt.contains("\nblock_requires_damage=true\n");
        boolean no=receipt.contains("\nblock_requires_damage=false\n");
        check(yes!=no,"Boolean profile lacks one unambiguous serialized value");
        return yes;
    }
    private static boolean gesture(int phase,int x,int y,long when) {
        return R166RetainedPointerJniSmoke.pointer(
            phase,x,y,when,true,true,676,630);
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1 &&
              args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Only the disposable native test DLL is admissible");
        System.load(args[0]);
        check(pageOffset()==-1 && !page(8) && booleanTarget()==null,
              "Unarmed source-page Boolean JNI interface admitted");
        check(R166RetainedPointerJniSmoke.begin(),
              "Disposable WGL retained menu could not be started");
        try {
            check(pageOffset()==-1 && !page(8) && !page(9) &&
                  booleanTarget()==null,
                  "Unfocused source module exposes page actions");
            check(R167RecoveredSliderJniSmoke.navigate(2),
                  "Auto Block source row could not be focused");
            check(pageOffset()==-1 && booleanTarget()==null,
                  "Collapsed source row exposed settings page");
            check(R167RecoveredSliderJniSmoke.navigate(4),
                  "Auto Block recovered source widgets failed to expand");
            check(pageOffset()==0 && !page(9) && booleanTarget()==null,
                  "Initial source page or backward boundary incorrect");
            check(!damaged(),"Native Auto Block Damaged default unexpected");

            check(page(8) && pageOffset()==5 && booleanTarget()==null,
                  "Page 2 failed or leaked page 3 source widget");
            check(page(8) && pageOffset()==10,
                  "Source page 3 admission refused");
            check(!page(8) && pageOffset()==10,
                  "Source settings page overrun was accepted");
            int[] coords=booleanTarget();
            check(coords!=null && coords.length==2 &&
                  coords[0]>=0 && coords[0]<676 &&
                  coords[1]>=0 && coords[1]<630,
                  "Recovered Damaged source hit not resolved");
            final int x=coords[0],y=coords[1];

            final AtomicBoolean crossThreadRefused=new AtomicBoolean(false);
            Thread foreign=new Thread(new Runnable(){
                @Override public void run() {
                    crossThreadRefused.set(
                        pageOffset()==-1 && !page(9) &&
                        booleanTarget()==null &&
                        !gesture(0,x,y,800));
                }
            });
            foreign.start();foreign.join();
            check(crossThreadRefused.get(),
                  "Foreign Java thread obtained retained page ownership");

            check(!R166RetainedPointerJniSmoke.pointer(
                      0,x,y,1000,false,true,676,630),
                  "Missing synthetic exclusivity accepted");
            check(!gesture(2,x,y,1020),
                  "Unmatched source Boolean release committed");
            check(!R166RetainedPointerJniSmoke.pointer(
                      0,x,y,1100,true,false,676,630),
                  "Missing synthetic foreground accepted");
            check(!R166RetainedPointerJniSmoke.pointer(
                      0,x,y,1200,true,true,677,630),
                  "Wrong source viewport admitted");
            check(!damaged(),"Rejected Boolean events modified native profile");
            check(!gesture(0,x,y,1900),
                  "Unpainted source-page Boolean click admitted");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "R184 source-page Boolean repaint failed");
            check(gesture(0,x,y,2000),"Recovered Boolean press denied");
            check(gesture(2,x,y,2040),"Recovered Boolean release denied");
            check(damaged(),"Source Boolean did not mutate native damage condition");
            check(R166RetainedPointerJniSmoke.redraw() && damaged(),
                  "Native redraw lost retained damage condition");

            check(gesture(0,x,y,2800),"Cancel setup refused");
            check(gesture(3,x,y,2810),"Source Boolean cancel refused");
            check(!gesture(2,x,y,2820),"Cancelled Boolean release committed");
            check(damaged(),"Cancelled Boolean changed native setting");

            check(gesture(0,x,y,3100),"Viewport loss setup refused");
            check(!R166RetainedPointerJniSmoke.pointer(
                      2,x,y,3120,true,true,677,630),
                  "Changed viewport committed Boolean release");
            check(!gesture(2,x,y,3130),
                  "Viewport-loss cancellation left held click active");
            check(damaged(),"Viewport loss corrupted native damage condition");

            check(gesture(0,x,y,3500),"Page-change setup refused");
            check(page(9) && pageOffset()==5 && booleanTarget()==null,
                  "Settings-page move failed to hide bound Boolean");
            check(!gesture(2,x,y,3550),
                  "Pending pre-page-change Boolean release committed");
            check(damaged(),"Stale source page toggled native damage condition");
            check(page(8) && pageOffset()==10 && booleanTarget()!=null,
                  "Original source page not restored");
            check(!gesture(0,x,y,3900),
                  "Restored unpainted source page admitted a stale click");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "R184 restored-page repaint failed");
            check(gesture(0,x,y,4000) && gesture(2,x,y,4040),
                  "Second Boolean gesture refused");
            check(!damaged(),"Second valid source gesture did not restore Boolean");
            check(page(9) && pageOffset()==5 && page(9) &&
                  pageOffset()==0 && !page(9) && booleanTarget()==null,
                  "Source page reverse navigation or lower boundary failed");
            check(!damaged(),"Page-navigation mutated native profile");
            System.out.println("R168_JAVA8_JNI_PAGE3_RECOVERED_BOOLEAN_NATIVE_PROFILE_PASS");
            System.out.println("R168_TWO_WAY_SOURCE_PAGE_BOUNDS_AND_STALE_CLICK_DENIAL_PASS");
            System.out.println("R168_WRONG_THREAD_FOREGROUND_VIEWPORT_CANCEL_GUARDS_PASS");
            System.out.println("R168_NO_ORIGINAL_MINECRAFT_INPUT_HOOK");
        } finally {
            check(R166RetainedPointerJniSmoke.end(),
                  "Disposable WGL owner could not close");
        }
        check(pageOffset()==-1 && !page(8) && booleanTarget()==null,
              "Disposed session still exposes source settings");
    }
}
