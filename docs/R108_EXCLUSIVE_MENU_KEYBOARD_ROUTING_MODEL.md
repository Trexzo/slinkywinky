# R108 — exclusive GUI keyboard delivery model, isolated from Minecraft

**Status:** implementation and deterministic fixture on draft reconstruction PR #1. This is a first-party C++ menu router, **not a live Minecraft event consumer**. The original Java 8 Attach renderer, R107 input audit, and R106 visibility Insert policy are unchanged.

## Architecture and precise limits

`MenuKeyboardRouter` adds one input-delivery model tied to the existing owner-thread `MenuNavigation`. It does not poll Windows, hook LWJGL, transform `ave.s()V`, send events to Minecraft, or activate combat/visual modules. The independent original-source audit in R107 establishes where a future event owner must be attached; it does not grant that ownership itself.

A new epoch may begin only if a caller separately attests BOTH exclusive original-game event dispatch and all keyboard/mouse controls released. Absent such an admission, a visible overlay produces `Quarantined`, not an invented no-pass-through success.

For an admitted modal session, every supported LWJGL2 key down and up (including unmapped gameplay keys) is assigned `MenuExclusive` in this **simulation**. Only new key-down edges map to existing local preview actions: Tab/Right/Left switch tabs, Up/Down focus rows, Enter expands, Space toggles local preview state, Numpad +/- adjusts a local setting. Insert/Escape request closure without forwarding a Minecraft key. Repeated held keys never repeat navigation. The model makes no network or game packets.

On menu closure, all previously GUI-owned key releases remain quarantined until drained; only subsequent events may return `GameUnchanged`. Unknown key codes, wrong thread, lost foreground, or a changed event source revoke routing rather than delivering uncertain events to gameplay. Revocation cannot be cleared by simply hiding a window: recovery additionally requires independently attested GUI closure, all controls released, and original input-path restoration. A fresh admission uses a strictly increasing epoch. Cross-thread calls cannot mutate owner state.

## Verification

- New `include/slinkywinky/menu_keyboard_router.hpp` and `src/menu_keyboard_router.cpp` are in portable C++17 `slinkywinky_core`, not installed as an OS input hook.
- New `tests/test_menu_keyboard_router.cpp` runs a deterministic mock-game dispatcher, checking the game receives no menu-owned keyboard events, including unmapped keys, repeats, close-key releases, focus loss, unknown keys, wrong thread, changed provenance, epoch rebinding and guarded abort restoration.
- `native_menu_keyboard_router_no_game_passthrough_fixture` is registered in CTest and passed locally. Its game non-delivery proof concerns only the **fixture**, never physical Minecraft keys.
- Complete local Windows x64 Release CTest: **30/30 PASS**, including Java 8 Attach, JNI and WGL. Python original-receipt suite: **55 run, 1 platform skip, 0 failures**. GitHub CI must be checked for the exact pushed head.
- Fresh supervised original vanilla Minecraft 1.8.9 compatibility run **PASS** on isolated PID **10908**, receipt `R94_ORIGINAL_189_OPERATOR_VISIBILITY_RECEIPTS_PASS`. R94's existing PID-scoped external menu toggle/stop and cooperative restoration remain intact; the disposable JVM was closed. This does **not** exercise R108 physical input or a modal keyboard consumer.

## Next gate before enabling actual in-game input

An explicitly opted-in, source-pinned original Minecraft 1.8.9 input-owner transaction must independently prove that both key-down and key-up of an actual UI-owned key are absent from the normal `avb` bindings and vanilla `axu` GUI path, while non-UI keys stay unchanged. It needs source-specific method admission, focus/viewport stability, loss-and-rearm, class restoration and external stop. No such live transformer, modal no-click-through proof, or physical keyboard acceptance exists in R108.

Keep draft PR #1 unmerged; gameplay/combat modules disabled.
