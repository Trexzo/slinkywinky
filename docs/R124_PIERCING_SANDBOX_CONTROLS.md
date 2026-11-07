# R124 — Interactive Piercing sandbox settings and isolated persistence

Project: `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, draft PR #1.

## Scope and provenance

R122 recovered eight distinct native GUI callsites for Piercing from the
user-supplied candidate decrypted archive: a two-option sort dropdown
(**Hurt time** / **Health**), five separate Always hit through switches
(Non-player entities, Friendlies, Bots, Enemies, Blocks), and two Conditions
switches (Holding weapon, Inside entity hitbox).

R124 adds working state transitions for **only those eight sandbox preview
controls**. The module's combat implementation, target acquisition, packet
handling, Minecraft key bindings and injection code are untouched. There
is no claim that the candidate decrypted archive is an authenticated
vendor source distribution.

## Behavior in the separate Windows OpenGL preview

Focus the Piercing module card on the Combat tab and expand it. Use:

- **F9:** move focus to the next Piercing setting.
- **F11:** move focus to the previous setting.
- **F10:** cycle the sort dropdown or toggle the selected boolean switch.
- **Page Up / Page Down** or the wheel over expanded settings:
  existing five-control page navigation.
- **F12:** save only the Piercing sandbox state.
- **Shift+F12:** load it, only if the entire file validates.

These keys are handled exclusively by the **separate first-party Windows
preview application**, not the injected Minecraft client. A setting cannot
be changed when Piercing is collapsed, not focused, in another tab or
outside the currently visible catalog page. Moving selection past control
four automatically presents the second settings page. There is no
wraparound or key autorepeat action.

The GL renderer reads the immutable render frame's own copy of the
preview state. Piercing's selected row gets an accent marker; booleans
render their actual preview state rather than original static fixture
examples, and the dropdown shows the currently selected source label.
A changed state or selected control invalidates an in-flight menu mouse
gesture. It cannot reach `ClientSettings` or change gameplay actions.

## Strict sandbox profile format

A **separate file**, not the existing native module profile, is stored
under `%LOCALAPPDATA%\SlinkyWinky\piercing-preview-only.conf`:

```text
# Local sandbox Piercing GUI only; NOT game profile
version=1
sort=0
flags=0000001
```

The seven ordered flag positions correspond to the native Piercing
switches listed above; the last source-example flag starts enabled.
This is **preview-only data**, not a recovered original Slinky setting
serialization format. Saving uses a temporary file and replacement,
and loading validates every field before applying any state. Missing
fields, invalid option index, truncated/oversized documents, duplicate
or unknown keys, non-binary flag data and malformed line structure
are refused. The existing menu appearance and gameplay-profile files
remain separate.

## Regression

New `native_piercing_preview_persistence` CTest verifies all eight
controls, selection/page boundaries, wrong-tab/collapsed/off-screen
and foreign-thread refusal through `MenuNavigation`, profile
round-tripping, malicious/corrupted document rejection, transactionality
and stale render/mouse ownership. The isolated WGL regression draws
the original and edited sandbox states in an actual disposable OpenGL
context, verifies state restoration, and refuses forged preview state.

## Explicit limits

- Only the separate Piercing **sandbox preview** is editable here;
  other modules' recovered widgets remain reference-only, and numeric
  range values are still illustrative.
- A separately admitted original Minecraft 1.8.9 input ownership
  solution has not passed acceptance; R119's live game keyboard hook
  remains disabled.
- This is not the real Slinky's verified profile format or a working
  Piercing module. Pixel-accurate GUI fidelity, full control hit-testing
  and physical in-game acceptance require more work.
- GitHub CI and physical Windows preview verification are independent
  of this code change and must be reported separately. PR #1 remains
  draft and unmerged.
