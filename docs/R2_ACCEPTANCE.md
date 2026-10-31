# R2 integration and acceptance boundary

## Reproducible coverage

| Component | Current status | What is *not* proven |
| --- | --- | --- |
| xoshiro128+ generator | Deterministic vectors + range tests | Authentic official initialization seed |
| Reach | Candidate nine-tick decision model | Identical call frequency/game outcomes |
| Auto Clicker | Deterministic press scheduling, GUI/release guards | Original click timing distribution/input method |
| Aim Assist | Geometry, angle wrapping, FOV, speed bounds | Original target selection, smoothing, randomization |
| AutoBlock | Damage window, range and input ownership | Exact protected branch and packet timing |
| Velocity | Deterministic knockback scaling and chance | Actual game packet interception |
| Hit Select | Attack gating model | Full original prediction/interaction semantics |
| Backtrack | Packet-notice holding FIFO + expiry | Actual packet copies, serialization or replay |
| Runtime | Bounded commands, owner-thread admission for all host callbacks; explicit pulse commands, emergency release and generation fencing | JNI/JVMTI or Minecraft frame callback integration |
| Config | Strict parsing, defaults, backup and replacement | In-game settings UI / profile migration |
| Shared library/JNI | Portable C ABI; JVM integration smoke with Java 8-compatible wrapper | Real Minecraft event integration, injector, complete GUI |

## Next gates (must not be silently skipped)

1. Integrate a legal/authorized Minecraft 1.8.9 host with exact-client thread identity checks. Keep it independent of the nonredistributable reference DLL.
2. Test multiple worlds, player identity reset, quick reconnect, and shutdown with host-owned synthetic keys.
3. Map real packet types to explicit packet queue ordering. No arbitrary timeouts or packet replay after world generation change.
4. Expand modules with deterministic replay cases from independently collected expected behavior. Mark differences rather than hiding them.
5. Add GUI/HUD with keyboard-focus, settings persistence and fallback font handling.
6. Run a full Windows graphical playthrough in an isolated account/localhost test environment; compare event traces, FPS, and safe unload/reentry.
7. Publish a versioned release only after build, provenance, smoke, runtime, and user acceptance all pass.

No claim of official binary/source identity is made.
