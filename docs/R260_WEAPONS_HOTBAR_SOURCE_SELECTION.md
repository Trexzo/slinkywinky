# R260 — Weapons policy and Auto Weapon slot-selection interop

Recovered source: user-provided Slinky-Verified-SourceBuild.zip,
WeaponsModule.java, and original generated mod_weapons.cpp settings.

Independent C++ controller reconstructs:
- 9 hotbar slot-enable flags and first-allowed-slot order
- vanilla sword IDs 267,268,272,276,283
- vanilla axe IDs 258,271,275,279,286
- sharpness/knockback/fire-aspect enchant group IDs 16/19/20
- explicit extra-item numeric identifiers and fist behavior
- event26 EXTRA-ITEM CLEAR *proposal* only (no settings write)

The first eligible hotbar slot is selected by source iteration order;
a selected empty slot flag can win the search, while isAllowed on an
actual empty held stack still depends on the fist flag. Malformed item
snapshots/unknown enchant-group bits are refused by safety policy.

A new regression feeds the source-derived slot into R259 AutoWeapon's
source-derived fractional selection delay, verifying a *proposal*
rather than modifying Minecraft.inventory.currentItem.

Not reconstructed: source Weapons.Cache tick-based memoization,
NBT enchantment reads, the item-policy name/identifier mapping,
native semantic event26 priority, real host inventory integration,
or hotbar action ownership acceptance. No injected game behavior.
