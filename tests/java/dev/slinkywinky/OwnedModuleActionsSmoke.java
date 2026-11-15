package dev.slinkywinky;

public final class OwnedModuleActionsSmoke {
    private static void check(boolean ok, String why) {
        if (!ok) throw new AssertionError("R212: " + why);
    }

    private static final class MockPort implements OwnedModuleActions.Port {
        Object world = new Object(), player = new Object(), movement = new Object();
        float forward = 0.2f, strafe = -0.1f;
        int jumpTicks = 5, writes = 0;
        @Override public Object worldIdentity() { return world; }
        @Override public Object playerIdentity() { return player; }
        @Override public Object movementIdentity() { return movement; }
        @Override public float readForward() { return forward; }
        @Override public float readStrafe() { return strafe; }
        @Override public void writeForward(float v) { ++writes; forward = v; }
        @Override public void writeStrafe(float v) { ++writes; strafe = v; }
        @Override public int readJumpTicks() { return jumpTicks; }
        @Override public void writeJumpTicks(int v) { ++writes; jumpTicks = v; }
    }

    private static OwnedModuleActions.Input frame(
            boolean allowed, boolean present, long generation,
            boolean slow, boolean replace, boolean jump) {
        return new OwnedModuleActions.Input(allowed, present, generation,
            slow, replace, 1.0f, -0.5f, jump, jump, 0,
            true /* controlled Port simulates post-slow/pre-consumption */);
    }

    public static void main(String[] args) throws Exception {
        final MockPort mock = new MockPort();
        final OwnedModuleActions owner = new OwnedModuleActions();
        OwnedModuleActions.Result result=owner.apply(
            frame(false,true,1,true,true,true),mock);
        check(!result.admitted && mock.writes==0,"no opt-in must be read-only");
        // R213: the real Forge PlayerTickEvent.END hook is too late
        // for No Slow. Reject its movement write but still admit the
        // independently owned zero-cooldown jump action.
        result=owner.apply(new OwnedModuleActions.Input(
            true,true,1,true,true,1.0f,-0.5f,
            true,true,0 /* jump */,false /* late tick */),mock);
        check(result.admitted && result.noSlowBlockedByPhase &&
              !result.noSlowWritten && result.jumpTicksWritten &&
              mock.forward==0.2f && mock.strafe==-0.1f &&
              mock.jumpTicks==0,"late PlayerTick applied a No Slow write");
        mock.jumpTicks=5;
        result=owner.apply(frame(true,true,1,true,true,true),mock);
        check(result.admitted && result.noSlowWritten && result.jumpTicksWritten,
              "admitted actions missing");
        check(mock.jumpTicks==0 && mock.forward==1.0f && mock.strafe==-0.5f,
              "wrong actual port writes");
        int writes=mock.writes;
        result=owner.apply(frame(true,true,1,false,false,false),mock);
        check(result.admitted && !result.noSlowWritten && !result.jumpTicksWritten,
              "disabled still writes");
        check(mock.forward==0.2f && mock.strafe==-0.1f,
              "disabled did not restore owned movement");
        check(mock.writes==writes+2,"unexpected extra cleanup writes");

        owner.apply(frame(true,true,1,true,true,false),mock);
        mock.forward=0.7f; // user/game changed this after our action
        owner.release(mock);
        check(mock.forward==0.7f && mock.strafe==-0.1f,
              "cleanup overwrote physical input or failed own strafe restore");
        owner.apply(frame(true,true,1,true,true,false),mock);
        Object oldPlayer=mock.player;
        mock.player=new Object();
        mock.forward=0.7f;mock.strafe=0.3f;
        owner.apply(frame(true,true,2,false,false,false),mock);
        check(mock.forward==0.7f && mock.strafe==0.3f,
              "world/player transition wrote into replacement");
        check(oldPlayer!=mock.player,"invalid world switch fixture");
        owner.apply(frame(true,true,2,true,true,false),mock);
        mock.movement=new Object();
        mock.forward=0.12f;mock.strafe=0.34f;
        owner.release(mock);
        check(mock.forward==0.12f && mock.strafe==0.34f,
              "changed movement identity was written on release");
        int count=mock.writes;
        result=owner.apply(frame(true,false,3,true,true,true),mock);
        check(!result.admitted && count==mock.writes,
              "absent world wrote gameplay state");

        boolean negative=false;
        try {
            owner.apply(new OwnedModuleActions.Input(true,true,3,true,true,
                Float.NaN,0,true,true,0),mock);
        } catch (IllegalArgumentException expected) { negative=true; }
        check(negative,"NaN proposal admitted");
        negative=false;
        try {
            owner.apply(new OwnedModuleActions.Input(true,true,3,true,true,
                0,0,true,true,2),mock);
        } catch (IllegalArgumentException expected) { negative=true; }
        check(negative,"nonzero jump-cooldown override admitted");
        final boolean[] foreignRejected={false};
        Thread foreign=new Thread(() -> {
            try {owner.release(mock);}
            catch (IllegalStateException expected){foreignRejected[0]=true;}
        });
        foreign.start();
        foreign.join();
        check(foreignRejected[0],"foreign thread released game effects");

        owner.apply(frame(true,true,3,true,true,false),mock);
        owner.close(mock);
        check(owner.isClosed(),"owner close did not latch");
        negative=false;
        try { owner.apply(frame(true,true,4,true,true,false),mock); }
        catch (IllegalStateException expected){negative=true;}
        check(negative,"closed action boundary accepted another frame");
        System.out.println("R212_OWNED_MINECRAFT_ACTIONS_PASS " +
            "permission_gate=1 reversible_slow=1 zero_jump_delay=1 " +
            "owner_guard=1 world_reentry=1 preserve_physical_input=1 " +
            "late_input_phase_refused=1");
    }
}
