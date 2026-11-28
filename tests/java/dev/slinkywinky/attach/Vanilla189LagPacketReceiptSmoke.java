package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189LagPacketReceiptVerifier;

/** R370 only primitive JNI verdict consistency; never live packets. */
public final class Vanilla189LagPacketReceiptSmoke {
    static void yes(boolean v,String why){if(!v)throw new AssertionError(why);}
    static void deny(Runnable r,String why){
        boolean rejected=false;
        try{r.run();}catch(IllegalStateException|IllegalArgumentException ex){
            rejected=true;
        }
        yes(rejected,why);
    }
    public static void main(String[] args){
        Vanilla189LagPacketReceiptVerifier.Status hold=
            Vanilla189LagPacketReceiptVerifier.verify(1,true,1,0,0,0);
        yes(hold.hypotheticalHold&&!hold.attackRelease&&!hold.sprintRelease,
            "Native movement hold is only a hypothetical proposal");
        Vanilla189LagPacketReceiptVerifier.Status attack=
            Vanilla189LagPacketReceiptVerifier.verify(2,false,0,1,0,0);
        yes(attack.attackRelease&&!attack.hypotheticalHold,
            "Actual native attack-release receipt validated");
        Vanilla189LagPacketReceiptVerifier.Status sprint=
            Vanilla189LagPacketReceiptVerifier.verify(3,false,0,0,1,0);
        yes(sprint.sprintRelease,"Stop-sprint native release validated");
        Vanilla189LagPacketReceiptVerifier.Status position=
            Vanilla189LagPacketReceiptVerifier.verify(1,true,0,0,0,1);
        yes(position.positionRecorded,"Movement record must have source position");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(1,true,0,1,0,0),
            "Movement cannot report C02 attack release");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(2,false,0,0,1,0),
            "C02 cannot report stop-sprint");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(0,false,0,1,0,0),
            "Unknown packet cannot report attack release");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(3,false,0,1,0,0),
            "Stop-sprint packet cannot report attack release");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(1,false,0,0,0,1),
            "Movement without position cannot report position recorded");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(1,true,1,0,0,1),
            "Hypothetical hold cannot record sent position");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(2,false,1,1,0,0),
            "Hold cannot simultaneously release attack");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(2,false,0,Double.NaN,0,0),
            "NaN JNI result rejected");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(3,false,0,0,2,0),
            "Nonbinary JNI result rejected");
        deny(()->Vanilla189LagPacketReceiptVerifier.verify(4,false,0,0,0,0),
            "Out-of-domain packet kind rejected");
        System.out.println("R370_ORIGINAL_NATIVE_LAG_E3_RECEIPT_CONTRACT_PASS");
    }
}
