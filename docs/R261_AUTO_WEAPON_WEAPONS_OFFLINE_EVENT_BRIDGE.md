# R261 — Auto Weapon & Weapons offline event25/event26 bridge

Source independently cross-checked from the user-supplied verified
reconstruction bundle:

- AutoWeaponModule.java SHA256
  eb00bfce78852f40ada5cae53929b94499e930d61bba9f58976029ecc123cc17
  (event25, category COMBAT, float activation_time).
- WeaponsModule.java SHA256
  d3b8e218386d9a3d1c4a8aa4aeee569f599d451e5f5ebfcb23a804f0685ec115
  (event26 clears extra_items, Weapons slot/held-item classification).

R261 installs one validated offline callback per reconstructed
semantic event. Auto Weapon's callback derives FIRST ELIGIBLE SLOT and
current-slot weapon classification from the same 9-slot inventory
and settings values through the existing R260 WeaponsController.
Untrusted caller-provided first_allowed_slot and current_is_weapon
hints are overridden; a malformed Weapons snapshot refuses a
selection. The actual Java original caches the held weapon
classification by player.ticksExisted: this cache is **not yet**
reconstructed and therefore this is direct current-snapshot parity,
not full client parity.

Auto Tool and Auto Weapon may share an event25 frame, but conflicting
current slots, using-item state, or simultaneous block AND entity
target claims are rejected. Auto Tool and Weapons may share event26;
their proposed "clear allowed blocks" and "clear extra items" remain
separate value-only outputs. Neither touches an inventory/settings
field. Native callback priority is not authenticated; registration
fixture ordering is used in the regression.

The bridge resets AutoWeapon's source delay on disable/world reset,
and restores its pre-dispatch delay state if a later offline
callback throws, including in shared-source-RNG mode.

No live Minecraft target/hotbar input, game action, packet or
client-thread event hook is added. The remaining acceptance gate is
genuine player-tick cache, host dispatch ordering, and local 1.8.9
runtime instrumentation.
