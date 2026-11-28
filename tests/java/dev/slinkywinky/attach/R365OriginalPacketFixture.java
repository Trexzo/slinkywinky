// R365 synthetic original class names: no Mojang game bytecode.
class ff {}
class ip extends ff {
    public boolean g=true;
    public double a=8,b=0,c=0;
}
class fm extends ff {} // unknown subtype: never guess Attack or Sprint
class iq extends ip {} // C03 subclass must inherit original fields

class ek {}
class bcy {public ek c=new ek();}
class ave {
    public bcy handler=new bcy();
    public bcy u(){return handler;}
}
