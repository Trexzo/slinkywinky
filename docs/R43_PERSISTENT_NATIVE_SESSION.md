# R43 — persistent C++ decision context owned by the real Minecraft game thread

Previous diagnostic versions created and destroyed a native `SWContext` on **every tick**. That would reset timing, RNG and combat-state history before a real runtime was possible.

R43 adds an explicit native `open → observe → close` lifecycle. The first admitted read-only Minecraft tick opens `SWContext` on the exact Minecraft `Client thread`, subsequent ticks reuse it, and the terminal tick calls `nativeCloseObservation` on the **same thread** before the Java bytecode hook is restored. Any native exception, rejected frame, or inability to close triggers a diagnostic abort and prevents a false `GAME_THREAD_HOOK_CLEANUP_PASS` or unrestricted reattachment. No gameplay module can be enabled through this test interface.

The disposable-Java and real Minecraft acceptance checks require `NATIVE_SESSION_OPEN_PASS` and `NATIVE_SESSION_CLOSE_PASS`, in addition to their existing state observation, 20 ticks, exact class SHA-256 restore and repeated-injection checks.

This remains an **experimental, read-only native context** (explicit `--hook-probe` plus `SLINKYWINKY_EXPERIMENTAL_HOOK=1`). Live player+target sampling, native render/GUI, actions/packets and original vendor behavioral parity remain separate unverified work.
