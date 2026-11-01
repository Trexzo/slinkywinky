# R48 — original vanilla Minecraft 1.8.9 JVM accepted standalone native injection

## Observed on 1 November 2026

The original installed vanilla `1.8.9.jar` was used **read-only**, with SHA-256 `14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298`. The isolated test JVM ran **without Forge, LaunchWrapper, a commercial account session or the user's regular Minecraft profile**. Its Java libraries and Windows native DLLs were read from the locally installed official Minecraft libraries, checking hashes provided by the vanilla version manifest. Test game state and logs were confined to a dedicated temporary workspace.

The `slinkywinky_loader.exe` process enumerated and attached to the **exact JVM PID it had launched**, using Java 8 Attach API / Instrumentation; it did not target unrelated Java processes. The injector matched obfuscated class `ave`, tick method `ave.s()V`, admitted the exact mapped observation fields, and ran the bounded **zero-action** C++ decision callback on Minecraft's `Client thread`.

A first test completed one attach; the second attach's completion was mistakenly checked too early by the test harness, which then stopped the isolated process. This **test-runner issue was fixed** to require the nth occurrence of each marker. The corrected second run produced:

```
VANILLA_INJECT_PASS 1
VANILLA_INJECT_PASS 2
VANILLA_189_STANDALONE_NATIVE_ACCEPTANCE_PASS db50884624de99a0efb44fe7c2132335593b392504a8544e7aea47abd3be8b1d
```

Both injections completed 20 client-thread callbacks, native session close and exact class bytecode restoration; both restored SHA-256 fingerprints matched. No `Game crashed!` or `NoClassDefFoundError` occurred in the captured runtime output. The isolated game JVM was alive after both tests and was terminated only by the acceptance runner afterwards. Existing SpawnPK Java processes were left unchanged. Nothing from the original game installation was redistributed or committed.

## What this proves—and what it does not

**Proven:** actual original vanilla 1.8.9 obfuscated JVM compatibility for read-only native injection, correctly mapped tick method, Java-to-C++ execution, bounded callback, repeated injection, and bytecode restoration.

**Not yet proven:** populated single-player or multiplayer world/player/target fields inside real Minecraft (both test cycles were at the main menu); production actions or combat modules, game rendering/ClickGUI, packet protocol, reliable full DLL unload, anti-cheat compatibility, or original Slinky authenticity/1:1 behavior.

This result does **not** turn the diagnostic into a playable replacement; the injector remains opt-in and action-disabled. A reproducible **manual-only** vanilla acceptance harness will be retained in the repository once cleaned for portable invocation; ordinary CI must not start Minecraft.
