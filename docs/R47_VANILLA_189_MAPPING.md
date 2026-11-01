# R47 — exact original Minecraft 1.8.9 obfuscated mappings

**Offline-only research. No Minecraft process was started or attached to.**

The installed vanilla 1.8.9 JAR (SHA-256 `14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298`) and local MCP stable_22 `joined.srg` were inspected read-only. The tick method was independently confirmed with `javap`.

Exact map: Minecraft class `ave`; tick `ave/s()V` → `func_71407_l`; singleton `ave/A()Lave;` → `func_71410_x`; world `ave/f`, player `ave/h`, ray `ave/s`. Entity `pk/s,t,u` (position), `pk/y,z` (yaw,pitch), `pk/aS()F` (eye height); ray `auh/d` (entityHit). These names are 1.8.9-specific and must not be generalized to other versions.

The adapter selects the exact obfuscated name profile **only** for class `ave`. Named/SRG development mappings remain separate. The instrumentation hook admits obfuscated `s()V` only for target class `ave`. No original game bytecode, proprietary assets or Minecraft JAR are redistributed.

Synthetic Java fixtures (`ave`, `pk`, `auh`) exercise mapped player, target, camera, eye height, disconnect/reentry and tick admission in a disposable Java process. This is a mapping test, not proof of successful injection into an actual vanilla 1.8.9 client.
