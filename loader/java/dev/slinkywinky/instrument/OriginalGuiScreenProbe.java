package dev.slinkywinky.instrument;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * R173 READ-ONLY original Minecraft 1.8.9 screen-presence observer.
 *
 * This class is bound only AFTER RendererLivePreflight has admitted the
 * already-loaded exact-original ave/bfk classes. Original ave.A() is a
 * bytecode-verified static singleton field read, and public ave.m is
 * current GuiScreen (axu). No GUI method is invoked and no input is consumed.
 *
 * This evidence NEVER authorizes exclusive GUI input, event suppression,
 * gameplay actions, hooks, or changing the original screen.
 */
public final class OriginalGuiScreenProbe {
    public static final int REFUSED=-1;
    public static final int NO_SCREEN=0;
    public static final int SCREEN_PRESENT=1;
    private final Class<?> gameClass;
    private final Class<?> screenClass;
    private final Method singleton;
    private final Field currentScreen;
    private Thread renderOwner;

    private OriginalGuiScreenProbe(Class<?> game,Class<?> screen,
                                   Method getter,Field field) {
        gameClass=game;
        screenClass=screen;
        singleton=getter;
        currentScreen=field;
    }

    /**
     * Admitted game and loader must be obtained from the unique original
     * loaded classes, not by loading an arbitrary Minecraft lookalike.
     * Binding by itself proves ONLY the reflective shape. The caller must
     * have separately established the pinned original class bytecode.
     */
    public static OriginalGuiScreenProbe bind(Class<?> game,
                                               ClassLoader admittedLoader)
            throws ReflectiveOperationException {
        if(game==null || admittedLoader==null ||
           !"ave".equals(game.getName()) ||
           game.getClassLoader()!=admittedLoader)
            throw new IllegalArgumentException("R173 wrong original game owner");
        final Method getter=game.getMethod("A");
        final Field screen=game.getField("m");
        final Class<?> type=screen.getType();
        if(!Modifier.isPublic(getter.getModifiers()) ||
           !Modifier.isStatic(getter.getModifiers()) ||
           getter.getParameterTypes().length!=0 ||
           getter.getReturnType()!=game ||
           !Modifier.isPublic(screen.getModifiers()) ||
           Modifier.isStatic(screen.getModifiers()) ||
           screen.getDeclaringClass()!=game ||
           !"axu".equals(type.getName()) ||
           type.getClassLoader()!=admittedLoader)
            throw new IllegalArgumentException("R173 original screen shape mismatch");
        return new OriginalGuiScreenProbe(game,type,getter,screen);
    }

    /**
     * The first call pins the already-admitted render callback thread.
     * A refused observation cannot be interpreted as an absent GUI.
     */
    public synchronized int observe() {
        final Thread now=Thread.currentThread();
        if(renderOwner==null)renderOwner=now;
        if(renderOwner!=now)return REFUSED;
        try {
            final Object game=singleton.invoke(null);
            if(game==null || game.getClass()!=gameClass)return REFUSED;
            final Object screen=currentScreen.get(game);
            if(screen==null)return NO_SCREEN;
            return screenClass.isInstance(screen)?SCREEN_PRESENT:REFUSED;
        } catch(Throwable reflectionFailure) {
            return REFUSED;
        }
    }

    /** No reflection or mutable original-game state crosses this boundary. */
    public synchronized boolean ownerThread(){
        return renderOwner!=null && renderOwner==Thread.currentThread();
    }
}
