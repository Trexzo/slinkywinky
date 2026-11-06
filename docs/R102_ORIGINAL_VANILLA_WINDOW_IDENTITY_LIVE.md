# R102 — independently observed genuine vanilla 1.8.9 game window (read-only)

**Status:** an isolated original Mojang vanilla Minecraft 1.8.9 process passed the new Win32 **external window-identity** check. The result is not equivalent to certifying the original renderer's WGL HWND/DC, physical input control, in-game click-through protection, or a playable Slinky reconstruction. No keyboard/mouse events were sent.

## Purpose and relationship to R101

R101 provided a C++/JNI `audit_game_render_window` routine in the source tree and tested its owner-context/window checks on disposable WGL fixtures, but intentionally did **not** expose the read-only Java diagnostic through a production game launcher. R102 adds an **independent** way to inspect an original vanilla game window while the source-pinned launcher runs exactly one isolated JVM. It verifies one critical prerequisite for later focus admission, **not** the full in-game input safety chain.

The R102 inspector is the pure-Python `tests/windows_r102_window_identity_evidence.py`:

- On Windows only, enumerates top-level HWNDs read-only via `EnumWindows` and asks the OS for their PID, window-thread ID, title, root ownership, visibility, minimization, enabled state, client width/height and current foreground HWND.
- Accepts only one visible root-window titled **Minecraft 1.8.9** in the **exact isolated target PID**, paired with the original process's **kernel creation FILETIME** checked again after sampling. Acceptable client bounds: width 320–8192 and height 240–8192 pixels.
- Reports **background and foreground as different states**; a correct but background window never becomes an 'input granted' window. It neither attempts to raise the window nor asks another process to send input.
- Repeats the observation **eight times** using a bounded sample schedule. Any absent, ambiguous, resized, wrongly owned, forged, minimized or changed process/session result is refused; the inspector never guesses a replacement window.
- It has **no native process injection, Java Attach, WGL hook, subclassed WndProc, keyboard capture, mouse capture, GetAsyncKeyState or SendInput**. R99/R100's renderer and operator controls remain separate.

The original-only manual launcher `tests/windows_vanilla189_isolated_manual.py --run --window-identity-acceptance` verifies the locally installed original JAR's SHA-256 and 1.8.9 manifest/assets, insists there is **no unrelated Minecraft-like JVM** before launch, starts only one disposable Java 8 vanilla 1.8.9 process under a separate sandbox (never modifies the user's ordinary Minecraft installation), requires the original render classes to be loaded, checks process-birth identity and writes a strict UTF-8 JSON report. It then terminates only its owned JVM.

## Live Windows evidence (6 November 2026)

The first trial against a fresh original vanilla PID **10448** produced a native-window observation success (0 foreground, 8 background, 0 refused), but an escaped literal newline at the end made the persisted JSON invalid. This was discovered by independently inspecting the report and **fixed**; that first output is **not** used as the final machine-readable proof.

The second fresh original Mojang vanilla 1.8.9 PID **37344** was launched with the corrected report code. Its output reported:

```text
R102_ORIGINAL_WINDOW_IDENTITY_PASS pid=37344 foreground=0 background=8
R102_WINDOW_IDENTITY_NOT_WGL_OR_INPUT_CAPTURE_CERTIFICATION
```

Independent verification reopened the resulting `r102-window-evidence-37344.json` with `json.load` and reported:

```text
R102_VALID_JSON_PASS
samples=8 foreground=0 background=8 refused=0
original_sha=14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298
HDC_certified=False input_certified=False
R102_NO_MINECRAFT_JVM_REMAINING
```

The observed single original-game window was **854×480 client pixels**, exactly one visible root matching that original PID, and **background for all eight observations**. **No foreground/physical Insert/click was tested or proven.** The saved JSON deliberately records `render_hdc_certified=false`, `input_capture_certified=false`, `foreground_forced=false`, `os_input_sent=false`, and `target_process_reused=false`.

Evidence remains in the Windows disposable build workspace at:

```text
C:\Users\Felix\AppData\Local\Temp\slinkywinky-r89-inject-build\vanilla189-isolated\r102-window-evidence-37344.json
```

## Regression tests

`tests/test_windows_r102_window_evidence.py` is a standalone cross-platform, no-network, no-game unit test of the pure classifier. Its negative controls refuse absent/duplicate same-title windows, wrong process birth, wrong PID, minimized or invisible HWNDs, out-of-range client dimensions, foreign title, invalid thread/root/enable state, and ambiguous foreground status. The first local run failed only because its test referred to a misspelled evidence attribute; after correction it passed **4/4**. The four tests are also registered in the `core.yml` GitHub Windows/Linux Debug/Release workflow.

## What remains before actual mouse/keyboard ClickGUI

The live R102 result **does not prove the original renderer's HDC corresponds to that external HWND**; the independent C++/JNI R101 audit should be admitted in a separately verified source-pinned original-JVM mode before combining the two. Background-only sampling cannot certify foreground key admission. The **R95 physical/Synthetic Insert focus acceptance remains unverified**. A later scoped, opt-in test must positively prove a foreground original render HWND, explicit key release/rearm, modal input consumption that prevents clicks/keys reaching normal Minecraft UI or gameplay, safe focus-loss/resize transitions, session stop/unload and an independent visible-window screenshot or equivalent visual certification.

No gameplay, combat, movement or multiplayer modules were enabled. The authentic Slinky feature set is not reconstructed by this read-only milestone. Keep PR #1 **draft/unmerged**.
