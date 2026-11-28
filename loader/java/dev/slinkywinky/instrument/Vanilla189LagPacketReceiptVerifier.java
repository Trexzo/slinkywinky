package dev.slinkywinky.instrument;

/**
 * R370: strict validation of the real native C++ Lag Range event3 verdict.
 * This only verifies immutable primitive read-only telemetry. No packet
 * object, channel, future, or interception action is ever represented.
 */
public final class Vanilla189LagPacketReceiptVerifier {
    public static final class Status {
        public final boolean hypotheticalHold,attackRelease,sprintRelease,positionRecorded;
        private Status(boolean hold,boolean attack,boolean sprint,boolean position){
            hypotheticalHold=hold;attackRelease=attack;
            sprintRelease=sprint;positionRecorded=position;
        }
    }
    private Vanilla189LagPacketReceiptVerifier(){}
    private static boolean bit(double value,String name){
        if(!Double.isFinite(value)||(value!=0d&&value!=1d))
            throw new IllegalStateException("R370 untrusted JNI native e3 "+name);
        return value==1d;
    }
    public static Status verify(int sourceKind,boolean movementPosition,
            double holdValue,double attackValue,double sprintValue,
            double positionValue){
        if(sourceKind<0||sourceKind>3)
            throw new IllegalArgumentException("Invalid source packet type");
        boolean hold=bit(holdValue,"hold");
        boolean attack=bit(attackValue,"attack release");
        boolean sprint=bit(sprintValue,"sprint release");
        boolean position=bit(positionValue,"movement position");
        if((attack&&sourceKind!=2)||(sprint&&sourceKind!=3)||
           (position&&(sourceKind!=1||!movementPosition))||
           (hold&&(attack||sprint||position))||
           (attack&&sprint))
            throw new IllegalStateException("R370 native e3 state contradicts source packet category");
        return new Status(hold,attack,sprint,position);
    }
}
