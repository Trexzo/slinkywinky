# R263 — Auto Weapon source-order tick cache integration

The verified reconstructed AutoWeaponModule.java first checks input
readiness, item-use state, mouse policy, real hostile other-player
cursor target and ONLY THEN queries WeaponsModule.isWeapon(player).
The latter calls WeaponsModule.Cache.update with player.ticksExisted,
which can return a stale same-tick classification. The source only
calls WeaponsModule.firstAllowedSlot after cache says false.

R263 connects R262's owner-bound cache to the R261 event25 bridge
in exactly that condition order. Every frame supplies an explicit
player_ticks_existed value. Untrusted caller hints for
current_is_weapon or first_allowed_slot are never consumed.

A target failing early conditions never advances the cache. A cached
allowed weapon skips firstAllowedSlot entirely. A no-weapon result
queries the separately verified live-value 9-slot policy and then
R259's activation-delay state. The two models reset on module
disable/world reset or unexpected ownership mismatch.

The frame-atomic rollback now snapshots BOTH AutoWeaponController
and WeaponsTickCache in ordinary and shared-RNG dispatch paths.
Regression covers initial tick0, tick1 eligible sword, a same-tick
switch to fist, tick2 recalculation, early-gate skip,
same-tick changed fist policy, next-tick policy update, player
owner swap and an exception from a later event25 subscriber.

No game thread hook, inventory write, user-input synthesis or JNI
hotbar selection was added. This is source-derived offline behavior,
not verified live-client parity.
