# R257 — No Hurt Cam field sentinel lifecycle (offline)

Reconstructed Java source: \`NoHurtCamModule.java\` and its
\`State\` nested class from user-supplied verified source build.
The decrypted native archive does not provide a matching generated
module file; the event scheduling/provenance is not authenticated.

The reconstructed module's \`applyPreRender\` temporarily models
\`EntityLivingBase.maxHurtTime = Integer.MAX_VALUE\`, preserving
the original observed value. Repeated sentinel observations do
not replace that snapshot; external non-sentinel changes can
replace it when the preview applies again. On disable/unload,
a saved value is restored only when the field still equals
Integer.MAX_VALUE.

The R257 first-party C++ controller emits *value-only proposals*,
with world/player identity checks and explicit stop/reset. On
a world or player swap without an old-owner restore, the model
refuses the swap and drops the stale snapshot, an intentional
safety divergence. It never touches EntityLivingBase or the
Minecraft renderer.

CMake/CTest tests original preservation, repeat frames, non-
sentinel concurrent modification, wrong-player/world, no game,
missing/malformed field, and sentinel-at-entry.

**Missing**: source-exact pre-render callback ordering; game-side
owner-controlled field adapter; original render/overlay behavior;
isolated Minecraft 1.8.9 acceptance. A passing offline test
does not establish a playable No Hurt Cam.
