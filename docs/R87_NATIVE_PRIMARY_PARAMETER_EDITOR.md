# R87 — bounded native module parameter editor

**Scope:** first-party C++ source and standalone Windows GDI / OpenGL 1.1
preview programs. The production injector and Minecraft render hook are
unchanged. This is not in-game ClickGUI acceptance or a gameplay module claim.

## What is wired

R86 made seven visible module switches edit real `ClientSettings` fields.
R87 adds **one first-party numeric tuning parameter** for each of those
modules via `MenuProfileBridge::primary()` and
`MenuProfileBridge::adjust_primary(index, +/-1)`:

| Public catalog row | First-party setting | Allowed range | Step |
|---|---|---|---|
| Aim Assist | `combat.aim_range` | 0–6 | 0.25 |
| Auto Block | `combat.block_range` | 0–6 | 0.25 |
| Auto Clicker | `combat.clicks_per_second` | 1–30 | 0.50 |
| Backtrack | `backtrack_ms` | 0–1000 | 25 |
| Hit Select | `combat.hit_select_min_hurt` | 0–10 | 1 |
| Reach | `combat.reach.distance` | 0–6 | 0.10 |
| Velocity | `velocity.horizontal` | 0–1 | 0.05 |

The ranges are enforced through the existing strict native settings parser,
not through a client hook. Editing is denied unless the targeted catalog row
belongs to the selected tab, is on the current eight-row page, is expanded,
is implemented, and the direction is exactly +1 or -1. Values outside the
schema range or edits at hard boundaries fail without mutation. Every
accepted edit validates a copied complete `ClientSettings` document before
adoption.

## Standalone preview interaction

1. Select **Combat**, click the + glyph beside an implemented row to expand
   its parameter, and press **+** or **-** to adjust. The keyboard also accepts
   numpad add/subtract. Expansion, not the on/off switch, selects the numeric
   control.
2. GDI preview prints the current parameter name and value in its expanded
   row. GL preview shows the current parameter and value in its native window
   title (including bounded-overlay demo mode).
3. `P` saves and `L` loads the same strictly parsed standalone profile
   used in R86. Appearance preferences remain separate. Any stale tab/scroll
   focus is rejected by the controller, even when the preview still holds
   an older selected row ID.

The remaining 58 public catalog entries have **no** profile binding.
No JNI, game memory, packets, game input, or actual module activation occur
in the previews. Other numerical settings of the seven supported modules
are not yet editable through this UI.

## Regression evidence and release gate

`native_menu_profile_binding` now exercises the new API, hidden/unexpanded
rows, unsupported indices, invalid directions, seven assignments, limits,
tab changes, and persistence. The final GitHub CI verdict must be confirmed
on the **exact R87 branch head** before claiming verification. Actual
Minecraft 1.8.9 graphical/runtime acceptance remains separate and outstanding.

Keep PR #1 draft and unmerged.
