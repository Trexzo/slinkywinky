# R302 — exact Minecraft 1.8.9 Forge world/native ESP acceptance gate

R302 adds an **isolated, operator-requested** extension to the R207 Windows
launcher and R300/R301 Forge preview. It creates a singleplayer flat save
**only** under a new, direct OS-TEMP child named
`slinkywinky-r302-independent-forge-<unique>`, with dummy offline
username `R302Offline`. It never touches a normal Minecraft profile.

After actual world/player readiness the preview starts the R300 snapshot
sampler and R301 Java8 JNI session; 32 successful Forge client
PlayerTick.END callbacks must retain the same game-world and game-player
identities and generation. The native adapter must roundtrip owner tokens,
collection counts, and valid ESP row counts in **all three real C++ models**.
It then emits a one-time R302 acceptance marker, closes all native/Java
observers, and disallows automatic restart. No packets, keys, rotations,
world/entity changes, or GL drawing are performed by these observers.

The R207 launcher adds -R302EspWorldTest and requires the exact
R302ExpectedJarSha256 value:

    b4967fb970e2f369179b8268add1be5a80a8fcd7bba79d69659597213355ebe5

This digest was extracted from the successful Forge 1.8.9 artifact built
for commit bccdd860e24e7130a0e8ffef920731fb1e379787 and independently
verified against the JAR in that CI archive and forge-preview.sha256.
The launcher, Python verifier and Windows process wrapper all enforce
this exact SHA-256; arbitrary user-provided digest overrides are refused.
Other test modes retain their
existing fixed hashes and are mutually excluded from this mode.
R302 also checks the local game directory and dummy username.

## Authentic in-world acceptance (not CI simulation)

Run the isolated R207 launcher with the R302 mode, owner-selected exact
hash, fresh direct-TEMP InstanceRoot, original Minecraft libraries,
reviewed Forge JAR and matching built JNI DLL. For recorded proof, call
`tools/verify_r302_esp_windows.ps1` with the same InstanceRoot, Java8Home,
NativeJniDir, PythonExe, ExpectedReobfJarSha256, and the exact running
test JVM PID. The process wrapper checks its executable, full command
line, creation timestamp before/after, and forbids gameplay action modes.
The independent Python validator hashes the staged JAR and native DLL,
checks the original Minecraft 1.8.9 hash declared by the existing
preflight, requires a real temporary level.dat, Forge loader evidence,
32 causally ordered native world observations, and verified owner cleanup.

`python tests/r302_esp_world_receipt.py --selftest` exercises fabricated
positive/negative transcripts **only**. Neither CI nor the synthetic
selftest proves that this R302 build ran in an actual game world.

## Limits

- Actual Windows original 1.8.9 in-world R302 receipts are **pending** until
  the independent owner machine can execute them.
- A normally generated flat world can contain only the local player, so
  item/player/chest matching-row counts may be zero. A valid JNI
  roundtrip therefore proves execution but **not** visible ESP behavior.
- R300's actual Forge PlayerTick.END sampling has not been proven
  semantically equivalent to Chest's original event 0 or Item/Player's
  original event 18. GUI-selected color settings, projector, fonts, GL,
  ClickGUI and original decrypted client parity remain unfinished.
- This is a research/test harness, not an injectable playable release.
  Do not merge either draft PR based on CI alone.
