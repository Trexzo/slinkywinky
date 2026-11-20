package dev.slinkywinky;

/**
 * R250: immutable, owner-thread-only block-under-cursor evidence.
 * This is NOT a Minecraft click, block placement, rotation, or packet hook.
 * A match only proves that an observed ray result matches a previous plan.
 */
public final class ClutchFaceSampler implements AutoCloseable {
    public static final class Pos {
        public final int x,y,z;
        public Pos(int x,int y,int z){this.x=x;this.y=y;this.z=z;}
    }
    public static final class Hit {
        public final Pos block;
        public final int face; // Original 1.8.9 EnumFacing ordinal, 0..5.
        public Hit(Pos block,int face){this.block=block;this.face=face;}
    }
    public interface Port {
        boolean onClientThread();
        Object worldIdentity();
        Object playerIdentity();
        /** null means a valid observation with no block cursor hit. */
        Hit cursorBlockHit();
    }
    public static final class Request {
        public final long generation,tick;
        public final Pos target;
        public final boolean centered;
        public Request(long generation,long tick,Pos target,boolean centered){
            this.generation=generation;this.tick=tick;
            this.target=target==null?null:new Pos(target.x,target.y,target.z);
            this.centered=centered;
        }
    }
    public static final class Receipt {
        public final long generation,tick;
        public final boolean hitBlock,matchedTarget,matchedFace;
        public final Pos observed,adjacent;
        public final int observedFace;
        private Receipt(Request r,Hit hit,boolean target,boolean face,Pos next){
            generation=r.generation;tick=r.tick;hitBlock=hit!=null;
            matchedTarget=target;matchedFace=face;
            observed=hit==null?null:new Pos(hit.block.x,hit.block.y,hit.block.z);
            observedFace=hit==null?-1:hit.face;
            adjacent=next;
        }
        /** Read-only evidence of block+face; never proof of a successful press. */
        public boolean matchesPlannedFace(){return hitBlock&&matchedTarget&&matchedFace&&adjacent!=null;}
    }
    private final Thread owner=Thread.currentThread();
    private Object world,player;
    private long generation,lastTick;
    private boolean closed;
    private static boolean valid(Pos p) {
        return p!=null && Math.abs((long)p.x)<1000000L &&
            Math.abs((long)p.y)<1000000L &&
            Math.abs((long)p.z)<1000000L;
    }
    private void require(boolean condition) {
        if(!condition){close();throw new IllegalStateException("Clutch face evidence refused");}
    }
    private void same(Port port){
        require(port.onClientThread() &&
                port.worldIdentity()==world && port.playerIdentity()==player);
    }
    public Receipt capture(Port port,Request request) {
        require(!closed && Thread.currentThread()==owner &&
                port!=null && port.onClientThread() &&
                port.worldIdentity()!=null && port.playerIdentity()!=null &&
                request!=null && request.generation>0 &&
                request.tick>0 && request.tick>lastTick &&
                valid(request.target));
        if(world==null) {
            world=port.worldIdentity();player=port.playerIdentity();
            generation=request.generation;
        }
        same(port);
        require(request.generation==generation);
        try {
            Hit hit=port.cursorBlockHit();
            same(port);
            require(hit==null || (valid(hit.block) &&
                hit.face>=0 && hit.face<=5));
            boolean matched=hit!=null &&
                hit.block.x==request.target.x &&
                hit.block.y==request.target.y &&
                hit.block.z==request.target.z;
            boolean face=matched && ((hit.face==1)==request.centered);
            Pos next=null;
            if(face) {
                final int[] dx={0,0,0,0,-1,1};
                final int[] dy={-1,1,0,0,0,0};
                final int[] dz={0,0,-1,1,0,0};
                final long x=(long)hit.block.x+dx[hit.face];
                final long y=(long)hit.block.y+dy[hit.face];
                final long z=(long)hit.block.z+dz[hit.face];
                require(Math.abs(x)<1000000L && Math.abs(y)<1000000L &&
                        Math.abs(z)<1000000L);
                next=new Pos((int)x,(int)y,(int)z);
            }
            lastTick=request.tick;
            return new Receipt(request,hit,matched,face,next);
        }catch(RuntimeException error){
            close();throw error;
        }
    }
    @Override public void close() {
        closed=true;world=null;player=null;
    }
}
