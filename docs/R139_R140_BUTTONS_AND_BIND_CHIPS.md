# R139–R140 — recovered Friends action rows and module binding chips

**Branch:** `reconstruction/r1-core-fidelity`, draft PR #1 (not merged).  
**Source:** user's candidate `decrypted files.zip/re/clickgui.tpl.html`. This is a reconstructed HTML transcription of recovered properties, not authenticated native commercial Slinky source.

## R139 — the two Friends buttons

The typed fixture contains exactly two `RecoveredWidgetKind::Button` callsites, both in Friends:
- `Add nearby players as friends`
- `Remove all friends`

The candidate HTML `widget(it)` code for `it.k==='b'` appends a **single full-width 20px-high rounded .btn** row with the source label, centered 15px text, background `#404040` and radius 8. **No onclick callback is present in that candidate HTML branch.**

The first-party GL11 renderer now draws the precise source labels rather than `Preview action` and uses a full row instead of a right-half control. It does not duplicate a left-side label. Since the available HTML offers no source handler/confirmation logic, the buttons remain **intentionally unbound to any friend-list, gameplay, OS or network operation**. Do not interpret their rendered presence as implementation of friend management.

CTest `native_recovered_friend_action_button_isolation` verifies the two source callsites, Friends-only ownership, full-row geometry in the three preview layouts and no pointer dispatch/side effects. The HTML hover color `#505050` is an outstanding screenshot-fidelity refinement.

## R140 — keyboard, mouse and HOLD binding chips

The candidate HTML shows `bindChip` alongside each module name except the `NOBIND` set **Weapons, Friends, Teams**. The chip displays `NONE`, a bound key label or `PRESS A KEY...` during capture. A separate HOLD badge displays when the first-row source is hovered or HOLD is enabled.

The reconstructed candidate's example bindings, **not authenticated native defaults**, are:
- Aim Assist — `R`
- Auto Clicker — `MOUSE4`
- Velocity — `V`
- GUI — `RSHIFT`
- Blink — `C` with HOLD selected

R140 implements independent `MenuBindPreview {code,hold}` for all 67 module descriptors, validates a bounded Win32-style preview virtual-key vocabulary, and adds renderer/hit-test geometry shared across all panel modes. It never writes to `ClientSettings`, original Minecraft key tables or Java agent hooks.

- Key capture starts only from a visible, fully contained chip for a non-NOBIND module. While awaiting a key, it refuses other bind or HOLD commands, and the standalone preview intercepts its own Win32 keyboard events before tab/module shortcuts.
- Fresh `WM_KEYDOWN` codes, left/right modifier resolution and separate mouse buttons MOUSE2/3/4/5 can set a key. ESC, represented as code zero, clears the key **and HOLD**. Focus loss cancels capture without rewriting the last committed binding.
- HOLD toggles only for a valid, visible module with a bound key. This stricter policy is an isolated preview adaptation: the candidate JS allows toggling its HOLD state even when no key is assigned, although the unbound badge is visually disabled.
- Mouse input and binding changes stay in `MenuRenderFrame` snapshots; pending gestures are invalidated if the underlying binding/capture changes.
- A bound HOLD badge is **currently shown persistently** in the native GL11 demo, whereas the reference normally reveals it on row hover or when HOLD is already on. That exact hover behavior and source CSS animation are still missing.

## New regression

`native_recovered_keybind_preview_safety` covers all five example binds, source exclusion rules, vetted key names including mouse 2–5, wrong/out-of-view modules, bind capture, invalid key rejection, HOLD isolation and toggling, ESC clearing, focus-loss cancellation, snapshot invalidation, Unload modal ownership and all three preview panel modes.

## Acceptance and remaining gaps

No new gameplay modules or injected key hooks are claimed here. CTest, source screenshots and interactive Windows input checks must be evaluated on the *exact head* of this draft PR before passing the R139/R140 gate. Next visual work: hover-only HOLD badge, press styling, full source-scale/animation/typography comparison. Next broader GUI work: Profiles tab load/save/bind controls, and a rigorous mapping of GUI interactions into the independent runtime architecture only after each source-backed action is verified.
