// R365 synthetic original class names: no Mojang game bytecode.
class ff {}
class ip extends ff {
    public boolean g=true;
    public double a=8,b=0,c=0;
}
class fm extends ff {} // unknown subtype: never guess Attack or Sprint
class in extends ff {} // C02 use entity, original Lag Range attack packet category
enum is$a {START_SNEAKING,STOP_SNEAKING,STOP_SLEEPING,
            START_SPRINTING,STOP_SPRINTING,RIDING_JUMP,OPEN_INVENTORY}
class is extends ff {public is$a b=is$a.STOP_SPRINTING;}
class ix extends is {} // entity-action subclass
class iq extends ip {} // C03 subclass must inherit original fields

class ek {}
class bcy {public ek c=new ek();}
class ave {
    public bcy handler=new bcy();
    public bcy u(){return handler;}
}
