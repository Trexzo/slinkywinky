# R64 — original vanilla 1.8.9 render-callsite preflight

**Status (2026-11-01):** Offline structural verifier and negative-control tests committed; a Mojang client JAR was **not** available to execute the original-binary check in the authorized runtime. **No render hook, GL state change, input injection or in-game GUI activation.** PR #1 remains draft.

## Evidence and provenance
The public Minecraft 1.8.9 development sources document `net.minecraft.client.renderer.EntityRenderer.updateCameraAndRender(float,long)`, descriptor `(FJ)V`. The `Minecraft` game loop calls this render method. These readable names are mappings, **not** proof of the exact obfuscated class or method names shipped by Mojang.

Sources:
- https://github.com/Marcelektro/MavenMCP-1.8.9/blob/master/src/main/java/net/minecraft/client/renderer/EntityRenderer.java
- https://github.com/KhoiBruh/OptimizedMCP-1.8.9/blob/main/src/main/java/net/minecraft/client/Minecraft.java
- https://radmanplays.github.io/minecraft-1.8.9-javadoc/net/minecraft/client/renderer/EntityRenderer.html
- Original 1.8.9 launcher metadata: Mojang client SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`, size `8461484` bytes, class `ave` in the earlier validated 1.8.9 vanilla client.

## Implementation
`tests/java/dev/slinkywinky/attach/RenderCallsiteAudit.java` uses the already pinned ASM 5.2 toolchain to inspect **only locally supplied original Mojang 1.8.9 JAR bytes**. It checks the exact file size and expected SHA-1 **before** reading any class. It then:
1. Inspects `ave.class`, rejecting wrong class identity.
2. Finds instance field classes and its virtual invocations with descriptor `(FJ)V`.
3. Accepts candidates only if the invoked owner corresponds to a declared instance field and that class declares a public, nonstatic, concrete method of that exact name/descriptor.
4. Rejects zero or multiple matches, missing/oversized classes, bad size, bad hash and unsupported class identities.
5. Prints the candidate's obfuscated owner, caller and method name, tagged `R64_RENDER_CALLSITE_CANDIDATE`—**not** `RENDER_HOOK_PASS`.

The inspector does not write to the game, start a JVM, load native libraries, inject code or install render hooks. Its outputs are a preliminary mapping to be independently validated in a live JVM before relying on them.

The separate `slinkywinky_render_audit.jar` is included in the Windows injector preview artifact. It uses `slinkywinky_instrument.jar` for its pinned ASM dependency but is **not** included in the injected agent itself.

## Offline usage (user-owned original installed game JAR)
From the built Windows `Release` folder, with a full Java 8 JDK installed:

```powershell
& '<JDK8_HOME>\bin\java.exe' -cp 'slinkywinky_render_audit.jar;slinkywinky_instrument.jar' dev.slinkywinky.attach.RenderCallsiteAudit "$env:APPDATA\.minecraft\versions\1.8.9\1.8.9.jar"
```

If the installed file does not have Mojang's exact SHA-1/size, the command fails. **Do not upload or commit the game JAR.** Do not use the output to patch or hook a different client or version. The previous original vanilla in-world acceptance R60/R61 remains unperformed while the authorized Desktop Commander machine is offline.

## Regression and next integration gates
`RenderCallsiteAuditSmoke` generates small synthetic classfiles in memory with ASM and tests unique candidate admission, absent/incorrect field, static field, absent renderer class, nonpublic renderer method, ambiguous two-renderer calls, wrong root class and invalid hash parsing. It runs within the existing Java 8 Attach CTest **without launching Minecraft**.

R65 still needs:
- Run the audit against the actual, independently hashed original 1.8.9 client JAR.
- Cross-verify that the inferred obfuscated class is the renderer and the callsite lies in a real render-loop path, not merely any method matching `(FJ)V`.
- Safely admit a **read-only** callback to a disposable, non-game Java fixture. Require exact callback count, original-thread ownership, no GL changes and verified bytecode restoration.
- Only after separate original-game and GL state acceptance, draw the native GUI in Minecraft. No gameplay module toggles until independently certified.

Avoid claiming original Slinky injector parity, working in-game menu, full client, or 1.7.10 / Lunar / Badlion support from this offline preflight.
