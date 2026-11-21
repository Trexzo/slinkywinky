# Module coverage matrix

Reference: user-supplied native C++ archive (not an authenticated commercial release), independently recovered Java source, and first-party PR #2 code. This is a **source-file inventory**, not verified active registration, live integration, or completed gameplay parity. Status was reconciled at R253; offline models remain unfinished modules.

| Category | Candidate module | SlinkyWinky implementation | Next fidelity check |
| --- | --- | --- | --- |
| block | `auto_tool` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| block | `block_in` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| block | `bridge_assist` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| block | `clutch` | Offline C++ controller + opt-in Forge read-only observations | Validate host event timing, settings and world-owner lifecycle |
| block | `fast_mine` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| block | `fast_place` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| combat | `ghost_hand` | Offline C++ policy + event-8 bridge (no game interactions) | Authenticate original callback priority; test real Minecraft entity/ray context |
| combat | `aim_assist` | Experimental decision model (offline) | Compare branch-by-branch and host timing |
| combat | `auto_block` | Experimental decision model (offline) | Compare branch-by-branch and host timing |
| combat | `auto_clicker` | Experimental decision model (offline) | Compare branch-by-branch and host timing |
| combat | `auto_weapon` | Not implemented | Identify state, host events and reference traces |
| combat | `backtrack` | Not implemented | Identify state, host events and reference traces |
| combat | `criticals` | Not implemented | Identify state, host events and reference traces |
| combat | `hit_select` | Experimental decision model (offline) | Compare branch-by-branch and host timing |
| combat | `hitboxes` | Not implemented | Identify state, host events and reference traces |
| combat | `knockback_delay` | Not implemented | Identify state, host events and reference traces |
| combat | `knockback_displace` | Not implemented | Identify state, host events and reference traces |
| combat | `lag_range` | Not implemented | Identify state, host events and reference traces |
| combat | `piercing` | Not implemented | Identify state, host events and reference traces |
| combat | `reach` | Experimental decision model (offline) | Compare branch-by-branch and host timing |
| combat | `sprint_reset` | Not implemented | Identify state, host events and reference traces |
| combat | `timer_range` | Not implemented | Identify state, host events and reference traces |
| combat | `velocity` | Experimental decision model (offline) | Compare branch-by-branch and host timing |
| combat | `weapons` | Not implemented | Identify state, host events and reference traces |
| move | `fast_accel` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `instant_stop` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `inv_walk` | Not implemented | Identify state, host events and reference traces |
| move | `keep_sprint` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `no_jump_delay` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `no_slow` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `null_move` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `safe_walk` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| move | `sprint` | Offline C++ controller + limited Forge diagnostics | Validate host event timing, settings and world-owner lifecycle |
| move | `strafe` | Offline C++ controller + limited Forge diagnostics | Validate host event timing, settings and world-owner lifecycle |
| utility | `anti_bot` | Not implemented | Identify state, host events and reference traces |
| utility | `auto_pot` | Not implemented | Identify state, host events and reference traces |
| utility | `auto_rod` | Not implemented | Identify state, host events and reference traces |
| utility | `auto_soup` | Not implemented | Identify state, host events and reference traces |
| utility | `blink` | Not implemented | Identify state, host events and reference traces |
| utility | `chest_stealer` | Not implemented | Identify state, host events and reference traces |
| utility | `debug` | Not implemented | Identify state, host events and reference traces |
| utility | `fake_lag` | Not implemented | Identify state, host events and reference traces |
| utility | `friends` | Not implemented | Identify state, host events and reference traces |
| utility | `inv_manager` | Not implemented | Identify state, host events and reference traces |
| utility | `item_use_fix` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| utility | `no_hit_delay` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| utility | `no_item_release` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| utility | `no_use_delay` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| utility | `ping_fix` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| utility | `refill` | Not implemented | Identify state, host events and reference traces |
| utility | `teams` | Not implemented | Identify state, host events and reference traces |
| utility | `timer` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| visual | `anti_debuff` | Offline C++ controller (no verified live integration) | Validate host event timing, settings and world-owner lifecycle |
| visual | `block_esp` | Not implemented | Identify state, host events and reference traces |
| visual | `chams` | Not implemented | Identify state, host events and reference traces |
| visual | `chest_esp` | Not implemented | Identify state, host events and reference traces |
| visual | `free_look` | Offline C++ controller + event-13 bridge (no game camera writes) | Verify event priority, settings and in-game camera restoration |
| visual | `gui` | Not implemented | Identify state, host events and reference traces |
| visual | `indicators` | Not implemented | Identify state, host events and reference traces |
| visual | `item_esp` | Not implemented | Identify state, host events and reference traces |
| visual | `mod_overlay` | Not implemented | Identify state, host events and reference traces |
| visual | `name_tags` | Not implemented | Identify state, host events and reference traces |
| visual | `no_hurt_cam` | Not implemented | Identify state, host events and reference traces |
| visual | `notifications` | Not implemented | Identify state, host events and reference traces |
| visual | `player_esp` | Not implemented | Identify state, host events and reference traces |
| visual | `pointers` | Not implemented | Identify state, host events and reference traces |
| visual | `trajectories` | Not implemented | Identify state, host events and reference traces |

The offline `PacketHoldQueue` is a generic identity-only queue, **not** a completed Backtrack/FakeLag implementation. Rotation, input ownership and profile handling are infrastructure rather than module parity.

Inventory contains 67 candidate primary native module source files (auxiliary/callback files excluded). R255 verifies 24 matching offline C++ controller headers in PR #2, and the existing six experimental offline combat decision models. The remaining rows require source reconstruction; no row should be interpreted as a fully playable module. The separate reconstructed Java bundle includes 68 class names, which is not a direct 1:1 match to this native inventory.
