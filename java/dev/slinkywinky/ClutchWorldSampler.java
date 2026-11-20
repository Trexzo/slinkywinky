package dev.slinkywinky;

import java.util.Arrays;

/**
 * R237: source-ordered, read-only Clutch world observation for Minecraft 1.8.9.
 * Java 8 only. This captures block IDs; it never places, clicks, rotates or
 * mutates world state. The native planner remains responsible for decisions.
 */
public final class ClutchWorldSampler implements AutoCloseable {
    private static final int[] PREFIX = {1,6,19,43,79,123,163,167};
    // Exact R235 first-party table, checked against the C++ header in CI.
    private static final int[] PACKED = {
        0x888, 0x887, 0x878, 0x788, 0x988, 0x889, 0x886, 0x877, 0x787, 0x987,
        0x868, 0x778, 0x978, 0x688, 0xa88, 0x879, 0x789, 0x989, 0x88a, 0x885,
        0x876, 0x786, 0x986, 0x867, 0x777, 0x977, 0x687, 0xa87, 0x768, 0x968,
        0x678, 0xa78, 0x588, 0xb88, 0x869, 0x779, 0x979, 0x689, 0xa89, 0x87a,
        0x78a, 0x98a, 0x88b, 0x884, 0x875, 0x785, 0x985, 0x866, 0x776, 0x976,
        0x686, 0xa86, 0x767, 0x967, 0x677, 0xa77, 0x587, 0xb87, 0x668, 0xa68,
        0x578, 0xb78, 0x488, 0xc88, 0x769, 0x969, 0x679, 0xa79, 0x589, 0xb89,
        0x86a, 0x77a, 0x97a, 0x68a, 0xa8a, 0x87b, 0x78b, 0x98b, 0x88c, 0x874,
        0x784, 0x984, 0x865, 0x775, 0x975, 0x685, 0xa85, 0x766, 0x966, 0x676,
        0xa76, 0x586, 0xb86, 0x667, 0xa67, 0x577, 0xb77, 0x487, 0xc87, 0x568,
        0xb68, 0x478, 0xc78, 0x669, 0xa69, 0x579, 0xb79, 0x489, 0xc89, 0x76a,
        0x96a, 0x67a, 0xa7a, 0x58a, 0xb8a, 0x86b, 0x77b, 0x97b, 0x68b, 0xa8b,
        0x87c, 0x78c, 0x98c, 0x774, 0x974, 0x684, 0xa84, 0x765, 0x965, 0x675,
        0xa75, 0x585, 0xb85, 0x666, 0xa66, 0x576, 0xb76, 0x486, 0xc86, 0x567,
        0xb67, 0x477, 0xc77, 0x569, 0xb69, 0x479, 0xc79, 0x66a, 0xa6a, 0x57a,
        0xb7a, 0x48a, 0xc8a, 0x76b, 0x96b, 0x67b, 0xa7b, 0x58b, 0xb8b, 0x77c,
        0x97c, 0x68c, 0xa8c, 0x575, 0xb75, 0x57b, 0xb7b
    };
    public static final class Pos {
        public final int x,y,z;
        public Pos(int x,int y,int z) { this.x=x;this.y=y;this.z=z; }
    }
    public interface Port {
        boolean onClientThread();
        Object worldIdentity();
        Object playerIdentity();
        int blockId(int x,int y,int z);
        /** Minecraft rayTraceBlocks(from,to,false,true,false); null = no hit. */
        Pos rayTrace(double[] from,double[] to);
    }
    public static final class Request {
        public final long generation,tick;
        private final double[] lo,hi,eye;
        public final int range;
        public final float minHeight;
        public final Pos previous;
        public double[] boxLo() {return lo==null?null:lo.clone();}
        public double[] boxHi() {return hi==null?null:hi.clone();}
        public double[] eye() {return eye==null?null:eye.clone();}
        public Request(long generation,long tick,double[] lo,double[] hi,
                       double[] eye,int range,float minHeight,Pos previous) {
            this.generation=generation;
            this.tick=tick;
            this.lo=lo==null?null:lo.clone();
            this.hi=hi==null?null:hi.clone();
            this.eye=eye==null?null:eye.clone();
            this.range=range;
            this.minHeight=minHeight;
            this.previous=previous;
        }
    }
    public static final class Snapshot {
        public final long generation,tick;
        public final int baseX,baseY,baseZ;
        public final int previousBlockId;
        public final boolean groundBlocked;
        private final int[] ids,ground;
        private final double[] eye;
        private Snapshot(Request r,int x,int y,int z,int previousBlockId,
                         boolean groundBlocked,int[] ids,int[] ground) {
            this.generation=r.generation; this.tick=r.tick;
            baseX=x;baseY=y;baseZ=z;
            this.previousBlockId=previousBlockId;
            this.groundBlocked=groundBlocked;
            this.ids=ids.clone();this.ground=ground.clone();
            this.eye=r.eye.clone();
        }
        public int[] candidateBlockIds() { return ids.clone(); }
        /** -1 means not read because the source exited first. */
        public int[] groundBlockIds() { return ground.clone(); }
    }
    public static final class RayReceipt {
        public final long tick;
        public final Pos hit; // null means verified absence, NOT unknown
        private RayReceipt(long tick,Pos hit) {this.tick=tick;this.hit=hit;}
    }

    private final Thread owner=Thread.currentThread();
    private Object world,player;
    private long generation,lastTick;
    private boolean closed,rayUsed;
    private Snapshot latest;

    public static int[] sourcePacked() {return PACKED.clone();}
    public static int[] sourcePrefixes() {return PREFIX.clone();}
    private static boolean valid(double v) {
        return Double.isFinite(v) && Math.abs(v)<1000000.0;
    }
    private static boolean vec(double[] v) {
        return v!=null && v.length==3 &&
            valid(v[0]) && valid(v[1]) && valid(v[2]);
    }
    private static boolean pos(Pos p) {
        return p!=null && Math.abs((long)p.x)<1000000L &&
            Math.abs((long)p.y)<1000000L &&
            Math.abs((long)p.z)<1000000L;
    }
    private void require(boolean okay) {
        if(!okay) {close();throw new IllegalStateException("Clutch observation refused");}
    }
    private void owner(Port port) {
        require(!closed && Thread.currentThread()==owner && port!=null &&
            port.onClientThread() &&
            port.worldIdentity()!=null && port.playerIdentity()!=null);
    }
    private void same(Port port) {
        require(port.worldIdentity()==world && port.playerIdentity()==player &&
            port.onClientThread());
    }
    private int read(Port port,int x,int y,int z) {
        require(Math.abs((long)x)<1000000L &&
                Math.abs((long)y)<1000000L &&
                Math.abs((long)z)<1000000L);
        same(port);
        int id=port.blockId(x,y,z);
        same(port); // rejects world switches occurring during the read
        require(id>=0 && id<=4095);
        return id;
    }
    public Snapshot capture(Port port,Request r) {
        owner(port);
        require(r!=null && r.generation>0 && r.tick>0 &&
            r.tick>lastTick && r.range>=0 && r.range<8 &&
            Float.isFinite(r.minHeight) && r.minHeight>=0.0f &&
            r.minHeight<=8.0f && vec(r.lo) && vec(r.hi) && vec(r.eye) &&
            r.lo[0]<=r.hi[0] && r.lo[1]<=r.hi[1] &&
            r.lo[2]<=r.hi[2] && (r.previous==null || pos(r.previous)));
        if(world==null) {
            world=port.worldIdentity();
            player=port.playerIdentity();
            generation=r.generation;
        }
        same(port);
        require(r.generation==generation);
        final double cx=(r.lo[0]+r.hi[0])*.5;
        final double cy=r.lo[1]-.99;
        final double cz=(r.lo[2]+r.hi[2])*.5;
        require(valid(cx) && valid(cy) && valid(cz));
        final int x=(int)Math.floor(cx),y=(int)Math.floor(cy),
                  z=(int)Math.floor(cz);
        final int[] ground=new int[8];
        Arrays.fill(ground,-1);
        final int[] ids=new int[PREFIX[r.range]];
        boolean blocked=false;
        try {
            if(r.minHeight>0.0f) {
                for(int depth=1;depth<=8;++depth) {
                    ground[depth-1]=read(port,x,y-depth+1,z);
                    if(ground[depth-1]!=0) {blocked=true;break;}
                    if(!(r.minHeight>(float)depth))break;
                }
            }
            int previousId=-1;
            if(!blocked) {
                if(r.previous!=null && r.previous.y<=cy)
                    previousId=read(port,r.previous.x,r.previous.y,r.previous.z);
                for(int i=0;i<ids.length;++i) {
                    final int packed=PACKED[i];
                    ids[i]=read(port,x+(packed&15)-8,
                        y+((packed>>4)&15)-8,z+((packed>>8)&15)-8);
                }
            }
            same(port);
            Snapshot snapshot=new Snapshot(r,x,y,z,previousId,blocked,ids,ground);
            latest=snapshot;lastTick=r.tick;rayUsed=false;
            return snapshot;
        } catch(RuntimeException error) {
            close();throw error;
        }
    }
    /**
     * Second phase only: the native source planner computes the randomized
     * prior-block aim before it can ask Java for raycast evidence.
     * A raycast is never invented from a prior block's coordinates.
     */
    public RayReceipt priorRay(Port port,Snapshot snapshot,double[] to) {
        owner(port);
        require(snapshot!=null && snapshot==latest && !rayUsed &&
            !snapshot.groundBlocked && snapshot.previousBlockId>0 &&
            vec(to));
        same(port);
        final double[] from=snapshot.eye.clone();
        final double dx=from[0]-to[0],dy=from[1]-to[1],dz=from[2]-to[2];
        require(dx*dx+dy*dy+dz*dz<4.49*4.49);
        rayUsed=true;
        try {
            Pos hit=port.rayTrace(from,to.clone());
            same(port);
            require(hit==null || pos(hit));
            return new RayReceipt(snapshot.tick,hit);
        } catch(RuntimeException error) {
            close();throw error;
        }
    }
    @Override public void close() {
        closed=true;world=null;player=null;latest=null;
    }
}
