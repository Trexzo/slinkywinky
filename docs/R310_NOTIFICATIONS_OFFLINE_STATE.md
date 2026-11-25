# R310 — recovered Notifications offline state machine

## Reference evidence
Read directly from user Library archives:
- decrypted files.zip SHA256 326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4, 185 entries.
- c++_possibly_OpenSlinky-main.zip SHA256 c06b4c6033e2d19de86bad8f19127b0a3a68cfc496130233ebc9ff29f4540915, 316 entries.
- decrypted re/modules_real.json SHA256 35aa89f1b56e5cffa78c30758e4e70c03da4045e177b0b1dd10e396b8bcafcb0 contains 67 unique module records (17 combat, 10 move, 15 visual, 7 block, 18 utility), including Chest Stealer. This is an archive candidate inventory, not proof of the authentic original 66-module registration.
- decrypted src/gen/mod_notifications.cpp SHA256 2991946cd693f907a1c84b4ce591706d71556d31da1b1d6b6c4af146202cd6c0. Verified original candidate rdata.bin base 0x13e000 and strings at RVAs 0x152031..0x15208a: hide_ingame, hide_hold_notifs, hide_bind_only, show_combat, show_move, show_block, show_visual, show_utility. Registration and GUI ordering agree with the C++ candidate header.
- C++ candidate src/logic/visual/notifications.cpp SHA256 cf9f3902035e18709c80805e81c6ffb680ec5e60cf54fc18d7959a08ef73d2d4; notifications.h SHA256 0951dff868125ac52d880eaf56e5ad79330ef106eff27eb44507e8e3664a4813.

## Recovered subset
Independent offline state model for ten-message FIFO overwrite, profile/error/friend messages, category and descriptor filtering, per-tick toggle batching, first special-descriptor key hint, GUI-open clear, strict >5s expiration before aging, and redraw/animation flags. Tests cover positive and negative paths; explicit settings are required. The special descriptor identity is supplied explicitly because the candidate comment and comparison target are not equivalent proof that this is the GUI descriptor. Owner-supplied session reset, bounded strings, invalid status/delta rejection and overflow checks are defensive decisions, not claimed original behavior.

## Not recovered
Original native callback addresses/order, live manager binding, exact status and flag mapping in the authenticated binary, NanoVG font/layout/easing and drawing, GUI keyboard naming, real application behavior and pixel parity remain unverified. No game hook or executable action is added. The original archives are not committed.

The existing MODULE_MATRIX source-file count is updated by one controller; no completed-module or live callback coverage is claimed.
