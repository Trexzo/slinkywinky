# R122 — Native Aim Assist branches and Piercing controls

**Basis:** user-supplied candidate `decrypted files.zip`, SHA-256 `326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`, inspected read-only. `decrypted files/src/mod_aim.cpp` contains GUI callback `cb_2bae0`; `decrypted files/src/gen/mod_piercing.cpp` contains `cb_44ca0`. The independently extracted R120 native manifest identifies the display labels, config pointers and enum option strings. This candidate is **not independently authenticated as an official signed Slinky binary**.

## Exact static native callbacks

**Aim Assist:** 22 registered config options, **24 widget callsites** due to alternative mode-dependent Horizontal speed and Vertical speed calls. The old GUI's synthetic “Multi-point” boolean was removed; the recovered **Multi-point** section contains Horizontal and Vertical sliders. The Aim, Target and Conditions sections now follow native callback order. The literal branch metadata is retained as `RecoveredWidgetVisibility`: mode 2 (Lock-on) shows Vertical; mode 3 (Silent) shows Keep move direction, Ignore manual aim and its speed controls; modes 0/1 (Regular/Linear) use the alternate speed controls. Mode 3 hides Sprinting. Target mode 0 shows Increased FOV while locked; target mode 1 shows Sort by. These paths are **mutually exclusive**; the 24 rows in this static forensic preview are not all simultaneously visible in the original runtime.

**Piercing:** **8 separate callsites** replace four guessed display controls. Its Always hit through heading contains Sort targets by (Hurt time / Health), Non-player entities, Friendlies, Bots, Enemies and Blocks. Conditions contains Holding weapon and Inside entity hitbox. The previous multiselect was an approximation, not the source's original UI.

The source inventory grows from 378 to **385 callsite records** across 67 modules (206 boolean, 115 slider, 32 dropdown, 4 range, 13 color, 7 multiselect, 6 text, 2 button). This is not a count of concurrently shown original GUI controls.

New Multi-point sliders use **normalized illustrative 0–1 values**, clearly **not** recovered vendor bounds. Numeric examples, conditions' actual runtime inputs, edit actions, profile storage, mouse/keyboard interception, animation and active game modules are not certified or enabled. The GUI remains a source inspector and preview; this PR remains draft/unmerged.

The C++ native regression checks every recovered record, all Aim/Piercing ordered labels and types, each mode's static branch predicate, bounded page iteration and invalid mode refusal. The WGL test expects Piercing's real dropdown and retains multiselect rendering coverage using Ghost Hand. Production Minecraft input integration remains deliberately disabled under the R119 guard.
