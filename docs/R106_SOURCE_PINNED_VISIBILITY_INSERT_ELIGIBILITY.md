# R106 — source-pinned Insert eligibility on the original WGL render owner

**State:** draft PR #1; local original-client event-only regression PASS, GitHub Actions pending. This change is not a modal keyboard handler and does not enable modules.

## Production change

R95's manual visibility JNI poll previously treated a matching foreground root + PID as sufficient before reading the global Insert key state. That predated the stronger R101/R103 exact-WGL/HWND audit. R106 removes the weaker check.

The current renderer context/HDC and GL viewport are passed through the same `audit_game_render_window` identity gate used by R103. It verifies unique original 1.8.9 title/root HWND, same render thread and PID, visible/enabled unminimized window, physical client size, and current OpenGL viewport. Failed or background checks cannot activate the Insert path. Desktop-global key state is read only on the independently admitted foreground window.

The new first-party `ScopedVisibilityHotkey` is created lazily on the owning WGL render thread. It requires all observed mouse buttons and Insert to be released before its first admission; the existing `MenuFocusFence` then requires three consecutive source-audited foreground observations. A held Insert at entry does not toggle, auto-repeat cannot toggle, loss of focus or ambiguous HWND latches refusal for that session, and a new session requires a new epoch plus released controls. Wrong-thread calls cannot mutate the classifier.

PID-scoped external toggle/stop events still have independent authority and priority. Stop is polled before visual eligibility; a separate external toggle remains available when the game is in the background. No keyboard hook, WndProc subclass, SendInput, new native gameplay API, Minecraft keybinding change, mouse capture, or module activation is introduced.

## Verification

Local Windows x64 MSVC Release full JNI, JDK8 Attach and WGL CTest: **29/29 PASS**. Deterministic R106 negative controls cover held initial key, required release, button condition, three valid foreground samples, first rising edge, held-repeat refusal, wrong thread, focus-loss latch, new-session rearm, and invalid/ambiguous window refusal. Production JNI and external event/stop fixtures remain included in the full suite. Python source and original-receipt suite: **55 tests run, one skipped, zero failures** (mocked receipt assertions are not live-game acceptance).

Fresh supervised original vanilla Minecraft 1.8.9 seven-stage, event-only R94 compatibility run on isolated PID **39856**: `R94_ORIGINAL_189_OPERATOR_VISIBILITY_RECEIPTS_PASS`; the separate original JVM was closed automatically. This proves existing external PID-scoped toggle/stop and restoration remain intact with R106 source. It deliberately sends no Insert key and is **not** foreground hotkey, physical input, or click-through acceptance. Only the pre-existing SpawnPK Java processes remained afterward.

## Limitations

This class classifies a visibility toggle; it cannot prove that the same physical Insert input will be suppressed in Minecraft's ordinary LWJGL gameplay processing. No human-operated key press has been certified and the actual original foreground case remains to be established separately. Do not call this a physically interactive ClickGUI, no-pass-through proof, gameplay acceptance or source parity. Keep PR draft and do not activate combat/movement modules.
