package dev.slinkywinky;

import java.lang.reflect.Field;

/** R356 read-only 1.8.9 original OtherPlayerMP coordinates, value-only. */
public final class SourceCandidatePosition189 {
    private SourceCandidatePosition189() {}
    public static final class Point {
        public final double x,y,z;
        private Point(double xx,double yy,double zz){x=xx;y=yy;z=zz;}
    }
    /** Caller owns the client thread; no entity ref crosses JNI. */
    public static Point read(Object entity,Class<?> sourceOwner,
                             Field x,Field y,Field z){
        if(entity==null||sourceOwner==null||
           !sourceOwner.isInstance(entity)||
           x==null||y==null||z==null)
            throw new IllegalArgumentException("Untrusted source remote entity/fields");
        for(Field f:new Field[]{x,y,z})
            if(f.getDeclaringClass()!=sourceOwner||f.getType()!=Double.TYPE)
                throw new IllegalArgumentException("Wrong owner/type source field");
        try {
            double xx=x.getDouble(entity),yy=y.getDouble(entity),zz=z.getDouble(entity);
            if(!Double.isFinite(xx)||!Double.isFinite(yy)||!Double.isFinite(zz)||
               Math.abs(xx)>30000000d||Math.abs(yy)>30000000d||
               Math.abs(zz)>30000000d)
                throw new IllegalStateException("Nonfinite/unbounded remote position");
            return new Point(xx,yy,zz);
        }catch(IllegalAccessException inaccessible){
            throw new IllegalStateException("Source position inaccessible",inaccessible);
        }
    }
}
