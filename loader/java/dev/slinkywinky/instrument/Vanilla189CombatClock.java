package dev.slinkywinky.instrument;

import java.lang.reflect.Field;

/**
 * R360: original Mojang 1.8.9 player combat clock, independent of ray target.
 * Source JNI candidate maps Entity=pk.W:I and EntityLivingBase=pr.bl:I.
 * Only immutable primitive values escape; owner thread is the caller.
 */
public final class Vanilla189CombatClock {
    public static final class Sample {
        public final int playerTicks,lastAttackerTime;
        private Sample(int ticks,int attack){
            playerTicks=ticks;lastAttackerTime=attack;
        }
    }
    private static Class<?> playerClass;
    private static Field ticksField,attackField;

    private Vanilla189CombatClock(){}

    private static Field exactOwner(Class<?> player,String ownerName,String field)
            throws ReflectiveOperationException{
        for(Class<?> type=player;type!=null;type=type.getSuperclass()){
            if(ownerName.equals(type.getName())){
                Field found=type.getDeclaredField(field);
                if(found.getType()!=Integer.TYPE)
                    throw new NoSuchFieldException("Source candidate clock must be int");
                found.setAccessible(true);
                return found;
            }
        }
        throw new NoSuchFieldException("Unknown source candidate clock owner "+ownerName);
    }
    public static synchronized Sample read(Object player)
            throws ReflectiveOperationException{
        if(player==null)return null;
        final Class<?> c=player.getClass();
        if(playerClass!=c||ticksField==null||attackField==null){
            final Field ticks=exactOwner(c,"pk","W");
            final Field attack=exactOwner(c,"pr","bl");
            // Publish both fields together, never a partial mapping cache.
            ticksField=ticks;attackField=attack;playerClass=c;
        }
        final int now=ticksField.getInt(player),hit=attackField.getInt(player);
        if(now<0||hit<0||hit>now)
            throw new IllegalStateException("R360 impossible original combat clock");
        return new Sample(now,hit);
    }
    public static synchronized void release(){
        playerClass=null;ticksField=null;attackField=null;
    }
}
