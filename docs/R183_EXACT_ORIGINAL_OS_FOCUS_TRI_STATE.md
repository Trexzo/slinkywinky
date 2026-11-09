# R183 — exact Win32 focus-state evidence, separate from pointer admission

**Scope:** draft PR #1 (`reconstruction/r1-core-fidelity`). PR #2 remains
a separate reference-comparison track.

## Why R182 was insufficient

The R182 counters classify foreground through
`audit_original_render_pointer(...).has_value()`. That helper is
foreground-only and returns `nullopt` in **more than one case**:
the original window is truly background, the exact window identity
cannot be admitted, the WGL context/viewport is wrong, or a Win32
cursor/coordinate read fails. Therefore loss of a pointer sample alone
**cannot** prove Windows lost focus.

R183 retains R182 for compatibility but introduces a second, more
specific source-pinned trace. The native callback calls
`audit_game_render_window` after each old passive pointer observation.
This independent read provides an explicit three-way result:

- `ValidForeground`: only the correct current WGL DC/window/thread,
  with pinned original Minecraft 1.8.9 title and exact viewport,
  is also the active Windows foreground window.
- `ValidBackground`: that same valid original window is not foreground.
- `Refused`: WGL/viewport/window identity or another audit predicate
  is not trustworthy. Never infer background from this.

A focus/pointer inconsistency is **Refused**. Refusal breaks the focus
sequence and cannot bridge a loss/reacquisition; no synthetic focus
change is sent to the operating system or Minecraft.

## Inert native state machine

`OriginalWindowFocusTrace` is a bounded C++17 header-only state
machine. The production `WglRetainedMenuPreview` samples it only on
its exact WGL owner and only once per already-admitted passive hover
observation. Each sample conserves one of six aggregate counters:

`foreground, background, refused, losses, regains, post_hits`

`losses` requires an exact valid foreground-to-background transition;
`regains` requires a subsequent valid background-to-foreground
transition in the same uninterrupted valid epoch; `post_hits`
requires a true source-exact native Auto Block/Main/Range slider hover
**after** an OS-validated regain. No pointer coordinates or input events
cross the JNI evidence boundary. WGL context, DC and viewport mismatch
or missing/repeated paired samples fails closed.

The production Java 8 probe validates the six counters against R176,
R179 and R182 source receipts and prints, after the original source
restoration and successful admission:

`R183_ORIGINAL_EXACT_WINDOW_OS_FOCUS_PASS frames=64 foreground=N background=N refused=N losses=N regains=N post_hits=N native_click_delivery=disabled`

The `PASS` suffix describes the well-formed original-process receipt.
It is **not** an unconditional assertion that any focus cycle occurred:
`foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`
is a valid passive-background receipt.

## Operator-only strict gate

The existing original-client acceptance script accepts an additional
`--require-exact-os-focus-cycle` flag, which requires all parent gates:

`--run --render-retained-acceptance --require-foreground-source-hits --require-focus-cycle-source-hit --require-exact-os-focus-cycle`

Supply the existing `--workspace` / `--jdk8` and exact optional
`--loader` arguments. The script refuses noninteractive admission
and mixed test modes. Strict acceptance requires **zero refused OS
audits**, at least one exact foreground-to-background loss, one
regain and one source-exact post-regain hover. A failure is expected
if the isolated original Minecraft window never actually changes
foreground or the short 64-frame window finishes too quickly.

A successful result would establish read-only Windows focus-state
transition evidence on the exact source-pinned original WGL owner.
It **does not** establish who physically moved the pointer, human
intent, a real original-game mouse down/up, safe LWJGL event
suppression, or exclusive click ownership.

## Tests and current status

- Portable pure C++17 test for background/foreground/refused ordering,
  pointer denial, sequence reset, repeated exact source hits and
  impossible transition rejection.
- Disposable Java 8 JNI test runs the real first-party C++ GL11 menu
  and explicitly **synthetic** tri-state statuses, with foreign-thread,
  viewport, native profile immutability and teardown checks.
- Production-agent Java 8 receipt shape tests plus Python original-JVM
  marker negative controls (including refused=background forgery).
- All new fixtures are registered in CTest. No original commercial
  binary or Minecraft game assets have been committed.

An offline C++17 state machine compile/smoke has passed on Linux;
the connected Windows PC was offline when changes were made. A full
Windows/JDK8 CTest run, GitHub CI results and actual original-Minecraft
foreground loss/recovery have **not** yet been confirmed. Until then,
**keep PR #1 draft and live input delivery disabled**.
