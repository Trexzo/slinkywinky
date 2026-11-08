package dev.slinkywinky.instrument;

import java.util.concurrent.atomic.AtomicInteger;

/** R174: read-only render owner and original-GUI/mouse overlap risk test. */
public final class R174OriginalInputOverlapSmoke {
    private static final int GUI=OriginalGuiScreenProbe.SCREEN_PRESENT;
    private static final int WORLD=OriginalGuiScreenProbe.NO_SCREEN;
    private static final int QUARANTINE=OriginalPointerArbitrationAudit.QUARANTINED;
    private static final int PRESSED=OriginalPointerArbitrationAudit.UNSAFE_PRESS;
    private static final int RELEASE=OriginalPointerArbitrationAudit.UNSAFE_RELEASE;
    private static final int REFUSED=OriginalPointerArbitrationAudit.REFUSED;
    private static void check(boolean value,String reason){
        if(!value)throw new AssertionError("R174 "+reason);
    }
    private static int[] point(int mask){
        return new int[]{180,90,1,mask};
    }
    private static int[] outside(int mask){
        return new int[]{-1,90,0,mask};
    }
    private static int observe(OriginalPointerArbitrationAudit model,
                               int screen,boolean focus,int[] pointer) {
        return model.observe(screen,focus,pointer,676,630);
    }
    public static void main(String[] args)throws Exception {
        check(args.length==0,"No input handles or original Minecraft paths");
        final OriginalPointerArbitrationAudit a=new OriginalPointerArbitrationAudit();
        check(!a.exclusiveOriginalInputOwner() && !a.mayDeliverMenuClick(),
              "Auditor falsely grants original input ownership");
        check(observe(a,GUI,true,point(1))==QUARANTINE,
              "Initially held click was treated as newly pressed");
        check(a.possibleVanillaOverlapPresses()==0,
              "Held click counted as an independent edge");
        check(observe(a,GUI,true,point(0))==
              OriginalPointerArbitrationAudit.RELEASED,
              "All-release sync refused");
        check(observe(a,GUI,true,point(1))==PRESSED,
              "New GUI press risk not observed");
        check(observe(a,GUI,true,point(1))==PRESSED,
              "Held press observation interpreted as release");
        check(observe(a,GUI,true,point(0))==RELEASE,
              "Observed release not paired with earlier press");
        check(a.possibleVanillaOverlapPresses()==1 &&
              a.possibleVanillaOverlapReleases()==1,
              "Expected one press/release risk pair");
        check(observe(a,GUI,true,point(0))==
              OriginalPointerArbitrationAudit.RELEASED,
              "Repeated up generated duplicate edge");
        check(observe(a,WORLD,true,point(1))==
              OriginalPointerArbitrationAudit.NO_GUI,
              "Gameplay press misclassified as GUI candidate");
        check(a.possibleVanillaOverlapPresses()==1,
              "Unowned gameplay click counted as candidate");
        check(observe(a,GUI,true,point(1))==QUARANTINE,
              "World-to-GUI transfer while held not quarantined");
        check(observe(a,GUI,true,point(0))==
              OriginalPointerArbitrationAudit.RELEASED,
              "World-to-GUI held state could not be released");
        check(observe(a,GUI,true,point(1))==PRESSED,
              "Second clean press was not detected");
        check(observe(a,GUI,false,null)==
              OriginalPointerArbitrationAudit.BACKGROUND,
              "Focus loss did not cancel pending press");
        check(a.cancelledGestures()==1,"Focus loss did not invalidate hold");
        check(observe(a,GUI,true,point(1))==QUARANTINE,
              "Focus reacquisition with held button wrongly resumed press");
        check(observe(a,GUI,true,point(0))==
              OriginalPointerArbitrationAudit.RELEASED,
              "Focus reacquisition failed all-release fence");
        check(observe(a,GUI,true,point(3))==QUARANTINE,
              "Mouse chord admitted as exclusive left click");
        check(observe(a,GUI,true,point(0))==
              OriginalPointerArbitrationAudit.RELEASED,
              "Chord release did not restore observation");
        check(observe(a,GUI,true,outside(1))==QUARANTINE,
              "Outside-window press became menu click");
        check(observe(a,GUI,true,point(0))==
              OriginalPointerArbitrationAudit.RELEASED,
              "Outside-window release could not resynchronize");
        check(observe(a,GUI,true,point(1))==PRESSED,
              "Fresh post-quarantine left press not observed");
        check(observe(a,WORLD,true,point(1))==
              OriginalPointerArbitrationAudit.NO_GUI,
              "Screen close with held button was not cancelled");
        check(a.cancelledGestures()==2,"Closing GUI failed to cancel press");
        check(!a.mayDeliverMenuClick() && !a.exclusiveOriginalInputOwner(),
              "Read-only audit promoted a GUI click by counting candidates");
        check(a.refusedSamples()==0,"Valid fixture counted refused input");

        final OriginalPointerArbitrationAudit b=new OriginalPointerArbitrationAudit();
        final int[] states={GUI,GUI,GUI,GUI,GUI,GUI,GUI,GUI};
        final int[] buttons={0,1,1,0,0,0,1,0};
        for(int i=0;i<8;i++){
            int state=observe(b,states[i],true,point(buttons[i]));
            check(state!=REFUSED,"Eight-frame source-only audit refused");
        }
        check(b.validSummary(8) && !b.validSummary(7) &&
              b.possibleVanillaOverlapPresses()==2 &&
              b.possibleVanillaOverlapReleases()==2 &&
              b.samples()==8 && b.refusedSamples()==0,
              "Finite eight-frame GUI risk summary invalid");
        final AtomicInteger cleanupSummary=new AtomicInteger(0);
        Thread cleanup=new Thread(new Runnable(){
            @Override public void run(){
                cleanupSummary.set(b.validSummary(8)?1:-1);
            }
        });
        cleanup.start();cleanup.join();
        check(cleanupSummary.get()==1,
              "Cross-thread post-drain cleanup could not verify summary");
        check(!b.mayDeliverMenuClick(),
              "An eight-frame audit incorrectly authorized input");

        final OriginalPointerArbitrationAudit c=new OriginalPointerArbitrationAudit();
        check(observe(c,GUI,true,point(0))!=REFUSED,
              "Initial owner-bound observation refused");
        AtomicInteger foreign=new AtomicInteger(123);
        Thread worker=new Thread(new Runnable(){
            @Override public void run(){
                foreign.set(observe(c,GUI,true,point(1)));
            }
        });
        worker.start();worker.join();
        check(foreign.get()==REFUSED && c.refusedSamples()==1 &&
              !c.validSummary(2),
              "Wrong-thread input sample silently accepted");
        check(observe(c,GUI,true,point(1))==PRESSED &&
              !c.mayDeliverMenuClick(),
              "Foreign producer was allowed to own a menu input");

        final OriginalPointerArbitrationAudit d=new OriginalPointerArbitrationAudit();
        check(observe(d,GUI,true,point(0))!=REFUSED,
              "New viewport could not be pinned");
        check(d.observe(GUI,true,point(1),677,630)==REFUSED &&
              observe(d,GUI,true,point(0))==REFUSED &&
              d.refusedSamples()==2,
              "Changed viewport did not permanently lock audit");
        final OriginalPointerArbitrationAudit e=new OriginalPointerArbitrationAudit();
        check(observe(e,GUI,true,new int[]{0,0,1,32})==REFUSED,
              "Untrusted mask crossed validator");
        final OriginalPointerArbitrationAudit f=new OriginalPointerArbitrationAudit();
        check(observe(f,GUI,false,point(0))==REFUSED,
              "Background sample leaked physical pointer");
        final OriginalPointerArbitrationAudit g=new OriginalPointerArbitrationAudit();
        check(observe(g,-1,true,point(0))==REFUSED,
              "Uncertain original GUI screen treated as gameplay");
        System.out.println("R174_JAVA8_GUI_INPUT_OVERLAP_PRESS_RELEASE_FOCUS_PASS");
        System.out.println("R174_VIEWPORT_SCREEN_CHORD_WRONG_THREAD_LOCKOUT_PASS");
        System.out.println("R174_EIGHT_FRAME_READ_ONLY_RISK_AUDIT_PASS");
        System.out.println("R174_NATIVE_CLICK_DELIVERY_REMAINS_DISABLED");
    }
}
