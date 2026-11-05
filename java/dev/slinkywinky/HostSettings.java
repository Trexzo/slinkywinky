package dev.slinkywinky;

import java.util.Arrays;

/** Immutable, validated Java 8 settings profile for the 19-field JNI contract. */
public final class HostSettings {
    public static final int WIDTH = 19;
    public static final int CLICKER=0, REQUIRE_MOUSE=1, CPS=2, AIM=3, AIM_MOUSE=4,
        AIM_RANGE=5, AIM_HSPEED=6, AIM_VSPEED=7, AIM_FOV=8,
        BLOCK=9, BLOCK_DAMAGE=10, BLOCK_RANGE=11, BLOCK_HURT=12,
        REACH=13, REACH_DISTANCE=14, REACH_TICKS=15, REACH_SPRINT=16,
        HIT_SELECT=17, HIT_MIN_HURT=18;
    private final float[] values;
    private static boolean isToggle(int i) {
        return i==CLICKER || i==REQUIRE_MOUSE || i==AIM || i==AIM_MOUSE ||
               i==BLOCK || i==BLOCK_DAMAGE || i==REACH || i==REACH_SPRINT || i==HIT_SELECT;
    }
    public static HostSettings safeDefaults() {
        // Candidate reference numeric settings; modules disabled until expressly enabled.
        return new HostSettings(new float[]{0,1,12,0,0,4,15,15,90,0,0,3.1f,5,0,3.05f,3,0,0,0});
    }
    public HostSettings(float[] input) {
        if(input==null || input.length!=WIDTH)
            throw new IllegalArgumentException("Expected 19 settings");
        float[] copy=input.clone();
        for(int i=0;i<copy.length;i++) {
            float v=copy[i];
            if(Float.isNaN(v)||Float.isInfinite(v)) throw new IllegalArgumentException("Nonfinite setting "+i);
            if(isToggle(i)) {
                if(v!=0 && v!=1) throw new IllegalArgumentException("Nonboolean setting "+i);
                continue;
            }
            float low=0,high=100;
            switch(i) {
                case CPS: low=1;high=30;break;
                case AIM_RANGE: case BLOCK_RANGE: case REACH_DISTANCE: high=6;break;
                case AIM_FOV: high=180;break;
                case BLOCK_HURT: case HIT_MIN_HURT: high=10;break;
                case REACH_TICKS: high=20;break;
                default: break;
            }
            if(v<low || v>high) throw new IllegalArgumentException("Setting out of range "+i);
            if(i==HIT_MIN_HURT && v!=Math.floor(v))
                throw new IllegalArgumentException("Hit-select hurt must be integer");
        }
        values=copy;
    }
    public HostSettings withToggle(int index, boolean enabled) {
        if(!isToggle(index)) throw new IllegalArgumentException("Not a toggle");
        float[] next=values.clone();next[index]=enabled?1:0;
        return new HostSettings(next);
    }
    public HostSettings withValue(int index,float value) {
        if(index<0 || index>=WIDTH || isToggle(index))
            throw new IllegalArgumentException("Not a numeric setting");
        float[] next=values.clone();next[index]=value;
        return new HostSettings(next);
    }
    public float[] toNativeArray() {return values.clone();}
    @Override public String toString() {return Arrays.toString(values);}
}
