# R158: original-WGL retained GUI to JNI profile receipt

The existing original-render retained WGL menu owns a typed native MenuProfileBridge, but accepted operator events were evidenced only by GUI tab/row/flag counters. There was no original-render-thread readback of the actual native configuration. R158 adds that read-only bridge.

The retained menu profile_snapshot method accepts only an active retained model, its construction thread, exact pinned HGLRC/HDC, exact viewport and current WGL owner. It serializes the actual first-party ClientSettings, validates a strict parser roundtrip, and bounds the result to 4096 bytes. It refuses after shutdown, off-owner or with stale dimensions/context.

The first-party Java 8 renderer evidence now uses nativeReadRetainedProfile after actual R99 scripted changes and after R100 externally acknowledged operator commands. Its receipt must match the native auto_block_enabled flag independently reported by the existing retained GUI counter. Refusals poison the diagnostic session rather than enabling any gameplay action.

This establishes READ-ONLY native-to-Java state transfer on the already-admitted original WGL renderer. It does not connect that profile to client-thread combat/movement, install original game input hooks, persist local profile files, or implement the remaining modules. The injected DLL/loader are not modified to synthesize input. The candidate decrypted reference is not an authenticated original Slinky binary.
