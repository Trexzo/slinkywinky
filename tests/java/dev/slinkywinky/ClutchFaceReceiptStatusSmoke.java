package dev.slinkywinky;
public final class ClutchFaceReceiptStatusSmoke {
    static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    static void refuse(int a,int b,int c,int d){
        try{ClutchFaceReceiptStatus.classify(a,b,c,d);
            throw new AssertionError("accepted invalid R251 counters");}
        catch(IllegalStateException expected){}
    }
    public static void main(String[] args) {
        check(ClutchFaceReceiptStatus.INCONCLUSIVE.equals(
            ClutchFaceReceiptStatus.classify(20,0,0,0)),"no prior cannot pass");
        check(ClutchFaceReceiptStatus.INCONCLUSIVE.equals(
            ClutchFaceReceiptStatus.classify(20,7,4,0)),"no face match cannot pass");
        check(ClutchFaceReceiptStatus.OBSERVED.equals(
            ClutchFaceReceiptStatus.classify(20,7,4,1)),"real matching receipt");
        refuse(19,1,1,1);
        refuse(20,-1,0,0);
        refuse(20,20,1,1);
        refuse(20,1,2,1);
        refuse(20,1,1,2);
        refuse(20,0,0,1);
        System.out.println("R251_CLUTCH_CURSOR_COUNTER_NEGATIVES_PASS");
    }
}
