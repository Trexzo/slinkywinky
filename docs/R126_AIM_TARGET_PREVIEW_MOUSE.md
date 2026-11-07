# R126 — Aim Assist mode controls use actual preview mouse clicks

**Scope:** First-party Windows GL1.1 standalone/overlay *previews only*. Injectable architecture remains unchanged. Draft PR #1 remains unmerged.

## Change

R125 introduced physical Windows mouse admission for the isolated Piercing settings. R126 expands the same existing bounded mouse-down / same-hit mouse-up gate to two first-party, source-identified **Aim Assist** dropdown controls:

- **Aim / Mode** cycles the four local demonstration modes (Regular, Linear, Lock-on, Silent).
- **Target / Mode** cycles the two local target demonstration modes (Single, Switch), wherever that control appears in the five-widget, branch-dependent visible catalog page.

The two menu hits use distinct kinds and retain the active control index, ensuring a down on Aim cannot commit an up on Target. Only the drawn dropdown rectangle is clickable; labels, hidden branch widgets, unimplemented sliders, switches, and unrelated modules remain inert. A tab/page/mode change still invalidates a held click, and admission requires the existing foreground/window/context checks. Existing keyboard F7/F8 cycling remains.

The renderer now shows the *current* Aim/Target demonstration mode inside those two dropdowns, rather than leaving their reference options rendered as static text. Target's two displayed names come from the existing local two-state preview, not the candidate archive's apparently inconsistent four-item Target/Mode label list.

**No inference of original commercial UI or gameplay parity:** This is a preview control implementation, not proof of exact original Slinky dropdown menus, animations, module execution, original keybindings, input ownership, or file persistence.

## Tests and limitations

Added the three-mode model/hit-test/gesture acceptance to `native_piercing_preview_persistence`, covering direct Aim mode click, dynamic Target mode location after a branch change, label rejection, stale-frame denial and preserved independent preview values.

These tests must be run through Windows Release/CTest and GitHub Actions on the exact R126 head before claiming PASS. The connected desktop was offline during implementation, so no new original-Minecraft or WGL live acceptance is claimed.

## Next

Model real dropdown popup placement, selection and closing behavior against the recovered reference; keep original Minecraft keyboard/mouse interception disabled until complete ownership/restoration is proven.