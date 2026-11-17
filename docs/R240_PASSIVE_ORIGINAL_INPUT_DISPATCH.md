# R240 — Source-pinned, opt-in original input dispatcher entry diagnostic

2026-11-17 | Chat 1 | `reconstruction/r1-core-fidelity` | draft PR #1.

An explicit new Java 8 attached-agent mode
`minecraft-input-dispatch-readonly-r240` adds a bounded passive
entry observer to the actual vanilla Minecraft 1.8.9 `ave.s()V` input
dispatcher. It is not enabled by default or by R99's native GUI mode.

## Live admission and safe restoration

R90 first verifies the two unique already-loaded original Mojang classes,
official JAR/source fingerprints, shared loader and transformed runtime
bytecode execution shape. A new input-specific preflight performs the same
source check on `ave` before the weaver runs.

ASM adds precisely one `INVOKESTATIC onInputEntry()V` at method entry.
The original input body executes unchanged. There is no code to invoke
LWJGL Mouse/Keyboard, intercept `Mouse.next`, consume/replay events,
suppress the vanilla method, or act on gameplay, window focus or GUI
state. The synchronized observer admits at most 64 owner-thread-only
callbacks, and the bounded supervisor closes observation before verifying
restoration. The weaver must be **removed** before original `ave`
retransformation/inspection. A missing removal or failed restored-source
equivalence keeps the shared attach admission gate locked.

A generated-class smoke compiles the **same ASM weaver** into a disposable
Java 8 classloader and exercises unchanged method semantics, repeat-hook
refusal, wrong-thread counts and post-close callback suppression.

This is original input **dispatcher-entry instrumentation**, not a
proof of exclusive GUI ownership. R194–R195 showed original input
consumers outside `ave.s()`; physical double dispatch and full JVM
input restoration remain independently unproven. The Windows desktop
was unavailable for live original-client input acceptance. PR #2 is
untouched; no original game JAR or private original source is committed.

## Exact-commit CI verification

The R240 original-input diagnostic and direct-Mojang source audit passed **9/9 GitHub checks** on commit `58181cfe77d579237947d4515d797398fe3bcd30`:

- Windows and Ubuntu Debug/Release, Linux sanitizers, and JNI smoke: **6/6 success** — https://github.com/Trexzo/slinkywinky/actions/runs/44083442699
- Windows native JNI bridge: **success** — https://github.com/Trexzo/slinkywinky/actions/runs/44083442684
- Windows standalone Java 8 loader and Attach API: **success** — https://github.com/Trexzo/slinkywinky/actions/runs/44083442674
- Exact-original Minecraft 1.8.9 Mojang class audit: **success** — https://github.com/Trexzo/slinkywinky/actions/runs/44083442622

The direct original-source audit independently printed:
`R240_EXACT_MOJANG_189_INPUT_METHOD_ENTRY_ONE_CALL_ONLY_PASS` and
`R240_ALL_58_ORIGINAL_INPUT_CONSUMER_SITES_UNCHANGED_PASS`.
It also verified original game binaries were not committed or uploaded.

This exact-source audit proves only that the in-memory original
`ave.s()V` transformation adds one observer method call and preserves
the existing dispatcher bytecode's input consumers. The actual opt-in
R240 agent was **not physically exercised in an original Minecraft
client** because the connected Windows desktop was offline. Minecraft
physical input ownership, safe interception, gameplay handler suppression,
reversible event delivery and full Slinky ClickGUI functional parity
remain unverified. Keep PR #1 draft and unmerged; PR #2 untouched.
