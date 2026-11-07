# R118 — Page input routing without unsafe original-game interception

**Project:** Trexzo/slinkywinky, native injectable architecture (not Forge).
**Branch:** reconstruction/r1-core-fidelity, PR #1 draft, unmerged.

## Why live Minecraft input is still gated

The user-supplied `decrypted files.zip` underlies the 67-module, 378-widget GUI reference.
R117 made every recovered widget reachable through the owner-bound native
`NextSettingsPage`/`PreviousSettingsPage` commands. R118 tests keyboard and
Windows-window input routing without claiming the original game's controls
are exclusively intercepted.

The exact, pinned vanilla Minecraft 1.8.9 input-loop audit finds
`ave.s()V` owns a single `Keyboard.next()` call and additional
`avb.a(IZ)V` / `avb.a(I)V` keybinding activity **before and after**
that call. Replacing `Keyboard.next()` alone is **not** sufficient to
prove game action non-delivery when the GUI is open. The source-pinned
`OriginalKeyboardBridgeWeaver` and Java `KeyboardQueueOwner` therefore
remain **dormant and uninstalled**. R118 never changes actual game
bytecode, real LWJGL dispatch or player input.

## Changes

1. **Native modal keyboard router**
   `menu_keyboard_router.cpp` maps real LWJGL 2.9.4 `KEY_PRIOR=201`
   to previous and `KEY_NEXT=209` to next settings page. These constants
   were checked against the installed original library using Java 8
   `javap -constants org.lwjgl.input.Keyboard`. The router still
   requires independent, newer-epoch/exclusive-dispatch/physical release
   attestation. It consumes all simulated modal keys, suppresses repeated
   keydown, quarantines the close key and refuses lost owner/foreground.

2. **Dormant Java 8 page relay**
   `KeyboardPageRelay.java` implements `KeyboardQueueOwner.KeySink`
   for the same two key codes. Its callback only emits a `direction` of
   -1 or +1 and a session epoch to an explicitly provided **first-party
   preview receiver**. It cannot poll the OS or LWJGL, install a hook,
   invoke JNI, start a game module, or infer source admission.
   `KeyboardPageRelaySmoke.java` runs against a disposable
   `org.lwjgl.input.Keyboard` mock, never an actual Minecraft process.
   It tests pre-arm refusal, owner thread, repeat suppression, epoch
   isolation, close/drain/restoration and post-close vanilla pass-through.
   The helper is compiled into the Java 8 instrument JAR **but no runtime
   code installs it**. The test-only fake Keyboard is **not** in that JAR.

3. **Real OpenGL preview route**
   The isolated WGL test now uses `MenuKeyboardRouter.event` for
   Page Down/Page Up, rather than calling the pagination model directly.
   Source-attested synthetic events change the first-party native frame
   and produce the actual 900×630 rendered settings pages. The test
   also verifies repeat suppression, stale click cancellation,
   refusing forged offsets, close-key drain and complete GL state
   restoration. This is a source-owned **test fixture**, not a physical
   keyboard or in-game GUI acceptance result.

4. **Usable standalone Windows preview**
   The separate `slinkywinky_gl11_menu_preview.exe` now handles
   Windows `VK_PRIOR` (Page Up) and `VK_NEXT` (Page Down) while its
   exact window has focus and foreground ownership. It refuses
   autorepeat for page keys. The mouse wheel pages over the **expanded
   settings region**; wheel movement outside continues scrolling modules.
   At the first/last settings page, wheel events over settings do not
   unexpectedly scroll the module list. A missing/invalid owner WGL
   frame denies wheel input entirely. These Windows key codes are
   separate from LWJGL2's Java key code numbers.

5. **Shared wheel hit testing**
   A first-party `settings_page_wheel_target` function lives beside
   native `hit_test_menu`. It validates the entire render snapshot
   before returning any hovered page target, aligns with the exact
   recovered multi-column/scale/clip geometry, rejects invalid 5-control
   offsets, forged title/index, duplicate/out-of-order rows and
   unsupported/short/invisible regions. Tests exercise standalone,
   bounded and compact overlay coordinate transforms. This helper
   never captures OS mouse input.

6. **GUI start-up geometry**
   The standalone GL preview now opens at the recovered **900 px**
   default (or **1500 px** wide mode), not the previous pre-R115
   676/844 px size. Window title shows the current settings page for
   expanded modules with multiple pages.

## Evidence

All three images below are exact framebuffer readbacks from a disposable
900×630 Windows OpenGL 1.1 context. In R118 the page changes were
driven by **synthetic LWJGL2 key events** through the native modal
router and navigation model, not direct page mutations. They do
**not** show or certify a game or real OS keypress.

- [Aim Assist — key-routed first page](screenshots/r118-keyboard-first-page.png)
- [Aim Assist — key-routed second page](screenshots/r118-keyboard-second-page.png)
- [Aim Assist — key-routed final page](screenshots/r118-keyboard-last-page.png)

## Test status and remaining prerequisites

- Windows x64 Release build: **PASS**.
- Full CTest: **31/31 PASS** (including modal router, wheel geometry,
  Java 8 attach mock-relay, WGL rendered page transitions).
- Python: **55 tests run, one skipped, zero failed**.
- Original vanilla Minecraft 1.8.9: supervised separate
  JVM **PID 39844**, exact source-pinned R94
  **script-driven show/hide/stop PASS** and normally exited. This proves
  neither physical keyboard/wheel handling nor live page navigation.
- GitHub CI: independently verify after the commit is pushed.

**Still unproven:** physical standalone key/wheel acceptance by a human,
live original-game input exclusivity, exact post-hook keybind polling,
JVM-to-native action dispatch, actual game module controls and profile
filesystem operations. The next milestone must establish *full* original
keyboard ownership and exact restoration without allowing GUI-owned
keys into Minecraft's other keybinding paths. Until that source-backed
proof exists, the live page input hook remains **disabled**; PR #1
remains **draft/unmerged**.
