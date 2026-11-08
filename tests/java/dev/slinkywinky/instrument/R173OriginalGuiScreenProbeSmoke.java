package dev.slinkywinky.instrument;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicInteger;

/** Synthetic Java 8 fixture. No game, no native hooks, no production input. */
public final class R173OriginalGuiScreenProbeSmoke {
    private static void require(boolean yes,String why) {
        if(!yes)throw new AssertionError("R173 "+why);
    }
    public static void main(String[] args)throws Exception {
        require(args.length==0,"External handles forbidden");
        final ClassLoader loader=R173OriginalGuiScreenProbeSmoke.class.getClassLoader();
        final Class<?> game=Class.forName("ave",false,loader);
        final Class<?> screenType=Class.forName("axu",false,loader);
        final OriginalGuiScreenProbe probe=OriginalGuiScreenProbe.bind(game,loader);
        require(!probe.ownerThread(),"Renderer owner was guessed at bind time");
        require(probe.observe()==OriginalGuiScreenProbe.NO_SCREEN,
                "Synthetic world/no-screen classified as GUI");
        require(probe.ownerThread(),"Read-only render owner did not pin");
        final Method singleton=game.getMethod("A");
        final Field screen=game.getField("m");
        final Object instance=singleton.invoke(null);
        final Object gui=screenType.newInstance();
        screen.set(instance,gui);
        require(probe.observe()==OriginalGuiScreenProbe.SCREEN_PRESENT,
                "Synthetic GUI-open presence not observed");
        final AtomicInteger foreign=new AtomicInteger(42);
        Thread other=new Thread(new Runnable(){
            @Override public void run() {
                foreign.set(probe.observe());
            }
        });
        other.start();other.join();
        require(foreign.get()==OriginalGuiScreenProbe.REFUSED,
                "Foreign thread inspected original screen");
        require(probe.observe()==OriginalGuiScreenProbe.SCREEN_PRESENT,
                "Wrong-thread refusal poisoned legitimate render observer");
        screen.set(instance,null);
        require(probe.observe()==OriginalGuiScreenProbe.NO_SCREEN,
                "Synthetic GUI close not recognized");
        require(screen.get(instance)==null,
                "Read-only inspection changed the game screen");

        boolean bad=false;
        try{OriginalGuiScreenProbe.bind(null,loader);}
        catch(IllegalArgumentException expected){bad=true;}
        require(bad,"Null original class admitted");
        bad=false;
        try{OriginalGuiScreenProbe.bind(screenType,loader);}
        catch(IllegalArgumentException expected){bad=true;}
        require(bad,"Wrong original class admitted");
        bad=false;
        try{OriginalGuiScreenProbe.bind(game,null);}
        catch(IllegalArgumentException expected){bad=true;}
        require(bad,"Missing original classloader admitted");
        bad=false;
        ClassLoader alternate=new ClassLoader(loader){};
        try{OriginalGuiScreenProbe.bind(game,alternate);}
        catch(IllegalArgumentException expected){bad=true;}
        require(bad,"Mismatched original classloader admitted");

        System.out.println("R173_JAVA8_SOURCE_SHAPED_GUI_OPEN_CLOSE_READ_ONLY_PASS");
        System.out.println("R173_GUI_RENDER_OWNER_AND_CLASSLOADER_REFUSAL_PASS");
        System.out.println("R173_NOT_LIVE_MINECRAFT_INPUT_OWNERSHIP");
    }
}
