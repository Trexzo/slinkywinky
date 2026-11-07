package dev.slinkywinky.attach;

import org.lwjgl.input.Keyboard;

/** Disposable Java 8 fixture; no original game classes or packets. */
public final class R112KeyboardTarget {
    public static int delivered;
    public static int released;
    public static int keySum;
    // R119 fixture-only equivalents of vanilla pre-/post-queue logic.
    // These are intentionally NOT protected by the Keyboard.next() bridge.
    public static int beforeQueue;
    public static int afterQueue;

    public static void run() {
        beforeQueue++;
        while (Keyboard.next()) {
            int code=Keyboard.getEventKey();
            boolean down=Keyboard.getEventKeyState();
            delivered++;
            keySum+=code;
            if(!down)released++;
        }
        afterQueue++;
    }
}
