package dev.slinkywinky.forgegui;

/** R214: isolated Java owner bridge to Chat 1's actual GL11 C++ menu and
 *  source-backed R210 request/R209 ArrayList. This is not a Minecraft hook.
 *  Only callable from the Forge-owned GuiScreen/client thread in TEMP test.
 */
public final class R214NativeMenuOverlay implements AutoCloseable {
    static {
        if(!Boolean.getBoolean("slinkywinky.r214NativeGuiOverlay"))
            throw new IllegalStateException("R214 GL11 bridge is opt-in only");
        System.loadLibrary("slinkywinky_r214_gui_overlay");
    }
    private static native boolean nativeOpen(long sequence,long generation,
                                             boolean requested,boolean active);
    private static native boolean nativePaint(int rawWidth,int rawHeight);
    private static native long[] nativeRequest(int rawWidth,int rawHeight);
    private static native boolean nativeAck(long id,long sourceSequence,
        long worldGeneration,boolean requested,boolean active);
    private static native long[] nativeView();
    private static native long[] nativeVisual();
    private static native boolean nativeClose();
    private static native int[] nativeTogglePoint(int width,int height);
    private static native boolean nativeMouseDown(int width,int height,
                                                int x,int y,long timeMs);
    private static native long[] nativeMouseUp(int width,int height,
                                               int x,int y,long timeMs);
    private static native boolean nativeMouseCancel();
    private final Thread owner=Thread.currentThread();
    private boolean closed;
    private int draws;

    public static final class Request {
        public final long id, observedSequence, generation;
        public final boolean enable;
        Request(long[] values) {
            if(values==null || values.length!=5 ||
               values[0]<=0 || values[1]<0 || values[1]>1 ||
               values[2]<=0 || values[3]<0 || values[4]<=0)
                throw new IllegalStateException("Malformed GL11 owner request");
            id=values[0];enable=values[1]==1;
            observedSequence=values[2];generation=values[3];
        }
    }
    public static final class View {
        public final long sourceSequence,drawCount;
        public final boolean requested,active;
        public final long activeRows;
        View(long[] values) {
            if(values==null || values.length!=5 || values[0]<=0 ||
               values[1]<0 || values[1]>1 || values[2]<0 || values[2]>1 ||
               values[3]<0 || values[3]>66 || values[4]<0)
                throw new IllegalStateException("Malformed native GL11 owner state");
            sourceSequence=values[0];requested=values[1]==1;
            active=values[2]==1;activeRows=values[3];
            drawCount=values[4];
            if(activeRows>0 && !active)
                throw new IllegalStateException("Inert module displayed active");
        }
    }
    /** R216: an actual GL11 switch-track framebuffer sample, taken only
     * after a successful render from a source-sequenced native acknowledgement.
     * The fifth field proves the local preview flag remains unchanged.
     */
    public static final class Visual {
        public final long sequence,frames;
        public final boolean requested,previewFlag;
        public final int rgb;
        Visual(long[] a) {
            if(a==null || a.length!=5 || a[0]<=0 || a[1]<0 || a[1]>1 ||
               a[2]<0 || a[2]>0xFFFFFF || a[3]<=0 || a[4]<0 || a[4]>1)
                throw new IllegalStateException("R216 GL11 pixel receipt invalid");
            sequence=a[0];requested=a[1]==1;rgb=(int)a[2];
            frames=a[3];previewFlag=a[4]==1;
        }
        private static int colorDistance(int a,int b) {
            return Math.abs(((a>>16)&255)-((b>>16)&255))+
                   Math.abs(((a>>8)&255)-((b>>8)&255))+
                   Math.abs((a&255)-(b&255));
        }
        public boolean matches(long sequence,boolean requested) {
            return this.sequence==sequence && this.requested==requested &&
                   !previewFlag && colorDistance(rgb,requested?0xF35D12:0x404040)<=50;
        }
    }
    public Visual visual() {
        assertOwner();
        Visual value=new Visual(nativeVisual());
        // One startup navigation draw is deliberately before Sprint focus;
        // every later nativePaint is exactly one authoritative GL11 pass.
        if(value.frames!=draws+1L)
            throw new IllegalStateException(
                "R217 duplicate/missing GL11 pass: native="+value.frames+
                " JavaPaints="+draws);
        return value;
    }
    public R214NativeMenuOverlay(R213NativeSprintBridge.Ack initial) {
        if(initial==null || initial.id!=0 ||
           initial.generation!=0 || initial.requested || initial.active ||
           !nativeOpen(1,0,false,false))
            throw new IllegalStateException("R214 native GL11 source startup refused");
    }
    private void assertOwner() {
        if(closed || Thread.currentThread()!=owner)
            throw new IllegalStateException("R214 foreign/closed GUI owner");
    }
    public boolean paint(int width,int height) {
        assertOwner();
        if(!nativePaint(width,height))return false;
        ++draws;return true;
    }
    public int paintedFrames(){assertOwner();return draws;}
    public Request request(int width,int height) {
        assertOwner();
        return new Request(nativeRequest(width,height));
    }
    /** R215: only after the EXACT native painted GL11 switch receives
     * both Minecraft-owned mouse button edges; a null release is DENIED.
     */
    public boolean mouseDown(int width,int height,int x,int y,long ms) {
        assertOwner();
        return nativeMouseDown(width,height,x,y,ms);
    }
    public Request mouseUp(int width,int height,int x,int y,long ms) {
        assertOwner();
        long[] edge=nativeMouseUp(width,height,x,y,ms);
        return edge==null?null:new Request(edge);
    }
    public void cancelMouse() {assertOwner();nativeMouseCancel();}
    /** Scripted *in-process* acceptance coordinates from actual source
     * painted geometry; physical mouse events never call this method.
     */
    public int[] fixtureSourceTogglePoint(int width,int height) {
        assertOwner();
        if(!Boolean.getBoolean("slinkywinky.r215SourceMouseAuto"))
            throw new IllegalStateException("Fixture coordinates not admitted");
        int[] pixel=nativeTogglePoint(width,height);
        if(pixel==null || pixel.length!=2 ||
           pixel[0]<0 || pixel[0]>=width || pixel[1]<0 || pixel[1]>=height)
            throw new IllegalStateException("Painted source Sprint toggle absent");
        return pixel;
    }
    public void acknowledge(Request request,R213NativeSprintBridge.Ack ack) {
        assertOwner();
        if(request==null || ack==null || request.id!=ack.id ||
           request.enable!=ack.requested ||
           request.generation>ack.generation ||
           !nativeAck(ack.id,request.observedSequence+1,ack.generation,
                      ack.requested,ack.active))
            throw new IllegalStateException("R214 native GL11 owner ack rejected");
    }
    public View view() {assertOwner();return new View(nativeView());}
    @Override public void close() {
        assertOwner();
        if(!nativeClose())
            throw new IllegalStateException("R214 GL11 cleanup denied");
        closed=true;
    }
}
