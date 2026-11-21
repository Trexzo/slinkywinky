# R259 — Auto Weapon event25 offline selection-delay model

The user-supplied reconstructed Java AutoWeaponModule listens on
event25. Its State.waiting compares activation_time (float) to the
integer selectionDelayTicks, increments if still waiting, and
otherwise lets an action owner claim the right to change hotbar
selection. On target, mouse, weapon, item-use or slot mismatch,
the timer resets.

The original native mod_auto_weapon.cpp contains numeric activation
time and boolean mouse-setting registrations, but does not
authenticate the event25 priority or prove runtime behavior.
The selected slot comes from a separate WeaponsModule.firstAllowedSlot;
R259 accepts a caller-verified slot, not a fabricated selector.

The first-party C++ controller models the delay using the source's
float-before-increment order. It proposes a hotbar selection only
when target eligibility, owner identity, and action-owner permission
agree; it never changes Minecraft inventory. State resets at
world/player changes and source-defined invalid conditions.

CMake/CTest covers 1.5-tick fractional thresholds, 0 delay,
no eligible slot, already-selected slot, no mouse, wrong entity,
friendly/nonhostile target, item use, owner denial/reacquisition,
invalid settings, and world/player swaps.

The R258 inventory audit was also made growth-safe: it preserves
the 67 native candidates, 68 Java filenames and six separate
combat models but derives controller/unimplemented counts from
the actual matrix, so adding a module cannot fail merely because
the previous 36-missing checkpoint decreased.

Still missing: real Weapons selection, Minecraft event25 binding,
hotbar transactional restore, client-thread input ownership,
and in-game behavior verification.
