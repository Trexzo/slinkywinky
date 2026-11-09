package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R169: real C++ retained ClickGUI Aim Mode dropdown driven through
 * disposable Java 8 JNI/WGL. Preview-only; never executes gameplay.
 */
public final class R169DropdownJniSmoke {
    private static native int[] modeReceipt();
    private static native int[] aimControlTarget();
    private static native int[] aimOptionTarget(int option);
    private static void check(boolean value,String reason) {
        if(!value)throw new AssertionError("R169 "+reason);
    }
    private static int[] mode(int selected,boolean open) {
        int[] current=modeReceipt();
        check(current!=null && current.length==2 &&
              current[0]==selected && current[1]==(open?1:0),
              "Retained native dropdown receipt changed unexpectedly");
        return current;
    }
    private static boolean point(int phase,int x,int y,long when) {
        return R166RetainedPointerJniSmoke.pointer(
            phase,x,y,when,true,true,676,630);
    }
    private static boolean click(int[] xy,long when) {
        check(xy!=null && xy.length==2 && xy[0]>=0 && xy[0]<676 &&
              xy[1]>=0 && xy[1]<630,"Source click position invalid");
        return point(0,xy[0],xy[1],when) &&
               point(2,xy[0],xy[1],when+40);
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1 &&
              args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Only the segregated R166 test JNI DLL may be loaded");
        System.load(args[0]);
        check(modeReceipt()==null && aimControlTarget()==null &&
              aimOptionTarget(0)==null,"Unarmed popup JNI accepted input");
        check(R166RetainedPointerJniSmoke.begin(),"Disposable WGL create failed");
        final String originalProfile=R166RetainedPointerJniSmoke.profile();
        try {
            mode(0,false);
            check(aimControlTarget()==null && aimOptionTarget(2)==null,
                  "Collapsed Aim Assist exposed source control");
            check(R167RecoveredSliderJniSmoke.navigate(4),
                  "Source Aim Assist failed to expand");
            int[] dropdown=aimControlTarget();
            check(dropdown!=null && aimOptionTarget(2)==null,
                  "Inactive popup produced an option hit");
            final int x=dropdown[0],y=dropdown[1];

            check(!R166RetainedPointerJniSmoke.pointer(
                0,x,y,1000,false,true,676,630),
                "Missing synthetic exclusive input owner accepted");
            check(!point(2,x,y,1020),"Release after denied input accepted");
            check(!R166RetainedPointerJniSmoke.pointer(
                0,x,y,1100,true,false,676,630),
                "Missing synthetic foreground accepted");
            check(!R166RetainedPointerJniSmoke.pointer(
                0,x,y,1200,true,true,677,630),
                "Wrong source WGL viewport accepted");
            mode(0,false);

            final AtomicBoolean foreignRejected=new AtomicBoolean(false);
            Thread alien=new Thread(new Runnable() {
                @Override public void run() {
                    foreignRejected.set(modeReceipt()==null &&
                        aimControlTarget()==null && aimOptionTarget(2)==null &&
                        !point(0,x,y,1300));
                }
            });
            alien.start();alien.join();
            check(foreignRejected.get(),
                  "Foreign thread accessed owner-bound native popup");
            check(!point(0,x,y,1900),
                  "Unpainted expanded dropdown admitted a click");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "R184 dropdown source repaint after expand failed");
            check(click(dropdown,2000),"Aim Mode dropdown did not open");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "R184 native popup paint after open failed");
            mode(0,true);
            check(aimControlTarget()==null && aimOptionTarget(-1)==null &&
                  aimOptionTarget(4)==null,
                  "Popup admitted hidden control or invalid option");
            final int[] choice2=aimOptionTarget(2);
            check(choice2!=null && choice2.length==2,
                  "Source-validated third popup option not hittable");
            final int[] behind=R166RetainedPointerJniSmoke.toggleTarget();
            check(behind!=null && !point(0,behind[0],behind[1],2300),
                  "Popup allowed click-through to underlying module toggle");
            mode(0,true);

            // Aborted popup click cannot select a mode.
            check(point(0,choice2[0],choice2[1],2600),
                  "Valid popup press refused");
            check(point(3,choice2[0],choice2[1],2620),
                  "Popup cancellation refused");
            check(!point(2,choice2[0],choice2[1],2640),
                  "Cancelled popup release committed");
            mode(0,true);
            check(!R166RetainedPointerJniSmoke.pointer(
                2,choice2[0],choice2[1],2700,true,true,677,630),
                "Changed viewport erroneously committed popup");
            mode(0,true);

            check(click(choice2,3000),"Third Aim Mode option refused");
            mode(2,false);
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Native WGL redraw rejected post-dropdown render");
            mode(2,false);
            check(originalProfile.equals(R166RetainedPointerJniSmoke.profile()),
                  "Preview-only Aim Mode wrote gameplay profile");

            dropdown=aimControlTarget();
            check(click(dropdown,4000),"Reopen of Aim Mode failed");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "R184 native popup paint after reopening failed");
            mode(2,true);
            final int[] choice1=aimOptionTarget(1);
            check(click(choice1,4600),"Second source option refused");
            mode(1,false);
            check(aimOptionTarget(0)==null,
                  "Closed source popup retained an active option");
            check(originalProfile.equals(R166RetainedPointerJniSmoke.profile()),
                  "Reopened preview dropdown mutated native gameplay settings");
            System.out.println("R169_JAVA8_JNI_SOURCE_DROPDOWN_POINTER_SELECTION_PASS");
            System.out.println("R169_POPUP_MODAL_CLICK_THROUGH_AND_CANCEL_DENIAL_PASS");
            System.out.println("R169_PREVIEW_ONLY_MODE_DOES_NOT_TOUCH_GAMEPLAY_SETTINGS");
            System.out.println("R169_NO_ORIGINAL_CLIENT_INPUT_HOOK");
        } finally {
            check(R166RetainedPointerJniSmoke.end(),
                  "Disposable WGL GUI session failed to close");
        }
        check(modeReceipt()==null && aimControlTarget()==null &&
              aimOptionTarget(2)==null,
              "Disposed native GUI still accepts preview input");
    }
}
