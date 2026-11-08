package dev.slinkywinky.instrument;

/**
 * R178: deterministic Java 8 source-only R99 passive hover dwell.
 * This fixture cannot attach to Minecraft or invoke any native input.
 */
public final class R178RetainedHoverScheduleSmoke {
    private static void require(boolean pass,String reason) {
        if(!pass)throw new AssertionError("R178 "+reason);
    }
    public static void main(String[] args) {
        require(args.length==0,"External process handles not permitted");
        int sliderFrames=0;
        int actionCount=0;
        for(int frame=1;frame<=64;frame++) {
            int action=RendererWglEvidence.retainedHoverAction(frame);
            if(action>=0) {
                require(actionCount<4,"More than four inert source actions");
                final int[] expected={2,4,5,0};
                require(action==expected[actionCount],
                        "Original four source actions changed order");
                if(actionCount==2)
                    require(frame==61,"Preview action fired while slider window open");
                if(actionCount==3)
                    require(frame==62,"Tab switched before end of hover window");
                actionCount++;
            }
            boolean slider=RendererWglEvidence.sourceSliderVisibleDuringDraw(frame);
            if(slider) {
                require(frame>=3 && frame<=62,"Visible slider out of source window");
                sliderFrames++;
            }
            int[] expectedState={
                frame,frame>=62?1:0,frame>=62?17:1,
                RendererWglEvidence.retainedHoverActionCount(frame),
                0,frame>=61?1:0};
            require(RendererWglEvidence.expectedRetainedHoverState(
                       frame,expectedState),
                    "Runtime source action state rejected at frame "+frame);
            for(int i=0;i<expectedState.length;i++) {
                int[] mutated=expectedState.clone();
                mutated[i]=mutated[i]+1;
                require(!RendererWglEvidence.expectedRetainedHoverState(
                          frame,mutated),
                        "A same-length mutated native state was admitted at frame "
                            +frame+" field="+i);
            }
            require(!RendererWglEvidence.expectedRetainedHoverState(
                         frame,null),"Null state admitted");
            require(!RendererWglEvidence.expectedRetainedHoverState(
                         frame,new int[5]),"Truncated native state admitted");
            require(!RendererWglEvidence.expectedRetainedHoverState(
                         frame,new int[7]),"Extra fields admitted");
            if(frame==2)
                require(expectedState[1]==0 && expectedState[2]==1 &&
                        expectedState[5]==0,
                        "Expanded original Auto Block was not prepared");
            if(frame==61)
                require(expectedState[1]==0 && expectedState[2]==1 &&
                        expectedState[5]==1,
                        "Preview set before original tab transition");
            if(frame==62)
                require(expectedState[1]==1 && expectedState[2]==17 &&
                        expectedState[5]==1,
                        "Original R99 final state missing on source transition");
        }
        require(actionCount==4 && sliderFrames==60,
                "Source hover dwell or final navigation count not exact");
        require(RendererWglEvidence.retainedHoverAction(0)==-1 &&
                RendererWglEvidence.retainedHoverAction(65)==-1 &&
                RendererWglEvidence.retainedHoverActionCount(0)==-1 &&
                RendererWglEvidence.retainedHoverActionCount(65)==-1 &&
                !RendererWglEvidence.expectedRetainedHoverState(
                     0,new int[6]) &&
                !RendererWglEvidence.expectedRetainedHoverState(
                     65,new int[6]),
                "Out-of-range diagnostic frame admitted");
        require(!RendererWglEvidence.sourceSliderVisibleDuringDraw(1) &&
                !RendererWglEvidence.sourceSliderVisibleDuringDraw(2) &&
                !RendererWglEvidence.sourceSliderVisibleDuringDraw(63) &&
                !RendererWglEvidence.sourceSliderVisibleDuringDraw(64),
                "Original recovered slider incorrectly visible after source tab switch");
        System.out.println("R178_JAVA8_60_FRAME_SOURCE_SLIDER_HOVER_WINDOW_PASS");
        System.out.println("R178_FOUR_INERT_ACTIONS_AND_FINAL_TAB_SOURCE_STATE_PASS");
        System.out.println("R178_PER_FRAME_SNAPSHOT_MUTATION_NEGATIVES_PASS");
        System.out.println("R178_NO_MOUSE_MOVEMENT_OR_CLICK_OWNERSHIP");
    }
}
