package dev.slinkywinky.attach;

import org.lwjgl.input.Keyboard;

/** Disposable Java 8 fixture; no original game classes or packets. */
public final class R112KeyboardTarget {
    public static int delivered;
    public static int released;
    public static int keySum;

    public static void run() {
        while (Keyboard.next()) {
            int code=Keyboard.getEventKey();
            boolean down=Keyboard.getEventKeyState();
            delivered++;
            keySum+=code;
            if(!down)released++;
        }
    }
}
