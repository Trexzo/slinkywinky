package dev.slinkywinky.attach;
import dev.slinkywinky.instrument.*;
import java.lang.reflect.*;
/** Synthetic 1.8.9 name-shape test. No Minecraft game classes. */
public final class Vanilla189MappingSmoke {
    private static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
    public static void main(String[] args)throws Exception {
        Class<?> mc=Class.forName("ave");
        Object game=mc.getDeclaredMethod("A").invoke(null);
        ObservedState.admit(mc);
        ObservedState.Frame first=ObservedState.sample();
        check(first.flags==7,"Mapped world/player/target flags");
        check(Math.abs(first.xyz[1]-65.62)<.001,"Mapped eye height");
        check(first.xyz[0]==1.25 && first.xyz[2]==2.0,"Mapped player coords");
        check(first.xyz[3]==4.5 && first.xyz[4]==65 && first.xyz[5]==-2.0,"Mapped target coords");
        check(first.angles[0]==42 && first.angles[1]==-9,"Mapped rotations");
        Field cache=ObservedState.class.getDeclaredField("mappingBuilds");
        cache.setAccessible(true);
        long initial=cache.getLong(null);
        check(initial==3,"Vanilla profile must bind three runtime classes");
        for(int i=0;i<200;i++) {
            ObservedState.Frame stable=ObservedState.sample();
            check(stable.flags==7 && stable.generation==first.generation,
                  "Vanilla repeated frames must preserve session");
        }
        check(cache.getLong(null)==initial,"Vanilla mapping must be reused, not reflected per tick");
        Method tick=InstrumentationProbe.class.getDeclaredMethod("isTickMethod",String.class,boolean.class,boolean.class);
        tick.setAccessible(true);
        check((Boolean)tick.invoke(null,"s",false,true),"vanilla tick");
        check(!(Boolean)tick.invoke(null,"runTick",false,true),"reject mixed profile");
        check(!(Boolean)tick.invoke(null,"s",false,false),"reject obf in named profile");
        check((Boolean)tick.invoke(null,"runTick",false,false),"named tick");
        check((Boolean)tick.invoke(null,"func_71407_l",false,false),"srg tick");
        check((Boolean)tick.invoke(null,"tick",true,true),"test tick");
        Object ray=mc.getDeclaredField("s").get(game);
        ray.getClass().getDeclaredField("d").set(ray,null);
        ObservedState.Frame lost=ObservedState.sample();
        check(lost.flags==3 && lost.generation==first.generation,"target removal");
        Field world=mc.getDeclaredField("f"),player=mc.getDeclaredField("h");
        world.set(game,null);player.set(game,null);
        ObservedState.Frame menu=ObservedState.sample();
        check(menu.flags==0 && menu.generation>first.generation,"disconnect");
        world.set(game,new Object());player.set(game,Class.forName("pk").newInstance());
        ObservedState.Frame rejoin=ObservedState.sample();
        check(rejoin.flags==3 && rejoin.generation>menu.generation,"reentry");
        ObservedState.release();
        check(cache.getLong(null)==0,"Release must drop vanilla reflection bindings");
        ObservedState.admit(mc);
        check(ObservedState.sample().flags==3,"Vanilla readmission must rebind cleanly");
        ObservedState.release();
        System.out.println("VANILLA_189_OBFUSCATED_MAPPING_PASS");
    }
}
