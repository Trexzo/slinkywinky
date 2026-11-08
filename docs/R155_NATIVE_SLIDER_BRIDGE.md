# R155: first source-indexed native settings bindings

Five recovered GUI slider edit events connect to fields already present in the native ClientSettings profile: Auto Clicker / In-game / Target CPS, Backtrack / Main / Maximum delay, Reach / Main / Distance, Velocity / Main / Chance, and Auto Block / Main / Range.

The frame renders native profile values rather than stale fixture defaults; load/save uses the existing versioned strict settings parser. A source-matched GUI mouse-up validates the visible expanded card, source widget ID, filtered page and step quantization, then validates the proposed native profile before changing either GUI or native settings. Source values above native limits (e.g. Auto Block Range over 6) are rejected rather than coerced.

The other recovered widget callsites remain GUI-only previews. No new gameplay module is implemented, and the seven existing native modeled flags remain disconnected from live Minecraft input. Injector/JNI code is unchanged.
