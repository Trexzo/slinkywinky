package dev.slinkywinky.attach;

/** Original-free Java8 test-only bytecode fixture with two click exits. */
public final class R362SyntheticClickTarget {
    public int observed;
    public void aw(){
        if(observed==0){
            observed=1;
            return;
        }
        observed=2;
    }
    public void s(){observed+=10;}
    public void ax(){observed+=20;}
}
