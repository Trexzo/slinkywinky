# R228 — Native Auto Tool: source-grounded hotbar selection and shutdown

Date: 2026-11-15. First-party native integration branch
`integration/openslinky-fd9c592-reference`, draft PR #2.
GUI PR #1 remained unchanged. **No Minecraft launches.**

## Pinned original source evidence

The exact pinned reference `Skid-Technology/OpenSlinky@fd9c592`
contains `src/logic/block/auto_tool.cpp` and
`src/data/data.s:auto_tool_module_descriptor`.

The original descriptor is **Block category 3**, with exactly two
native callbacks, both at native priority zero:

| Descriptor entry | Candidate event | Handler |
|---|---:|---|
| +0x1d8 | 25 | `auto_tool_on_inventory_tick` |
| +0x1e8 | 26 | `auto_tool_on_shutdown` |

The source scans all nine hotbar slots **starting from the currently
selected slot** and chooses the fastest mining tool over baseline
speed 1.0. With equal speeds, the first visited slot wins, so the
current slot remains selected on a tie. The source requires held
attack, valid target block, allowed original held-item category,
optional physical sneak and optional allowed-block membership.
It refuses switching while an item is in use. Its activation-time
value is converted to a whole-tick countdown on inactive ticks;
when active, the countdown must drain before any selection.

On selecting another slot, the original requests exclusive hotbar
ownership and remembers the original selected slot if switch-back
is enabled. When mining stops, it releases ownership and requests
restoring the saved slot. Crucially, if the player **manually changes
the slot**, it discards the saved slot and **does not override the
player's choice**. The shutdown callback releases the allowed-block
list allocation rather than silently reusing the list.

## First-party implementation

New independent value-only module:
`reference_compat/include/slinkywinky/auto_tool_controller.hpp`.

- `auto_tool_item_kind` classifies the exact pinned original item
  IDs for five sword types, fifteen tools and shears, fists and other.
- `AutoToolController::on_tick` implements per-item permission
  gates, attack/sneak/block gating, block allowlist observation,
  activation delay, speed-based cyclic best-slot scan, exclusive
  hotbar-claim decision, optional switch-back and manual slot
  override. It returns **proposed** selection/release, never
  changes Minecraft inventory.
- `on_shutdown` proposes releasing the allowed-block list and
  closes its offline state. The actual original pointer memory is
  not accessed by this implementation.
- `reset_auto_tool_world_state` explicitly resets local state
  on world-owner retirement; `stop()` clears all callbacks.

The original's hotbar lock and item-stack mining-speed values are
**typed caller-supplied observations**, not invented live owner
claims. Fail-closed bounds reject impossible selected slots,
non-finite/negative speeds and invalid activation times. The upper
speed / activation limits are independent safety guards, not
proven original-binary equivalence.

New first-party `ModuleExecutionBridge::bind_auto_tool`
requires the precise **category 3 / event 25 and 26 / priority 0**
descriptor pair. No other events can invoke an Auto Tool payload.
Native source callback ordering and typed session admission remain
separate from actual JVM/JVMTI game integration.

## Offline verification

New `test_auto_tool_bridge.cpp` exercises exact source item-ID
classes, cyclic mining-speed ties, activation countdown truncation,
blocked ownership claim, saved-slot restoration, manual override,
item/target/allowlist permissions, corrupted settings, immutable
shutdown and exact two-event metadata binding.

A new synthetic 24-descriptor coverage fixture includes 21
independently implemented module policies, and **six negative
metadata fixtures** deliberately corrupt category, tick event,
tick priority, shutdown event, missing shutdown, or add a third
callback. All six must be rejected.

The 66-module candidate therefore has an expected offline source
coverage of **21 policies, 33 bound callbacks**, leaving
**138/171 callback subscriptions unimplemented**. These are **not
21 confirmed in-game working modules**.

The implementation deliberately does not install an injector,
simulate a game, intercept keys or execute a player hotbar action.
The original runtime's hotbar lock ownership, JNI access, actual
world block queries, inventory ItemStack speed calls and executable
in-game acceptance are still unverified.

Both pull requests remain draft and unmerged. Cross-platform
GitHub CI is a separate verification gate from local C++ tests.

## Verified Windows acceptance

MSVC x64 Release full `reference_compat` CMake build and **128/128
CTest PASS**. MSVC x64 Debug full build and **128/128 CTest PASS**.
Eight new R228 tests passed (one executable policy test, one positive
synthetic coverage audit and six negative source descriptor audits).

The original pinned reference was read but not altered. No Minecraft
or injected client process was launched for R228. Exact real-game
hotbar ownership and commercial 1:1 parity remain unverified.
