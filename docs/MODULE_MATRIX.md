# Module coverage matrix

Reference: user-supplied native C++ archive (not an authenticated commercial release). The table inventories **source filenames**, not verified active registration or completed behavior. Modules are never counted as complete based on names alone.

| Category | Candidate module | SlinkyWinky implementation | Next fidelity check |
| --- | --- | --- | --- |
| block | `auto_tool` | Not implemented | Identify state, host events and reference traces |
| block | `block_in` | Not implemented | Identify state, host events and reference traces |
| block | `bridge_assist` | Not implemented | Identify state, host events and reference traces |
| block | `clutch` | Not implemented | Identify state, host events and reference traces |
| block | `fast_mine` | Not implemented | Identify state, host events and reference traces |
| block | `fast_place` | Not implemented | Identify state, host events and reference traces |
| block | `ghost_hand` | Not implemented | Identify state, host events and reference traces |
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
| move | `fast_accel` | Not implemented | Identify state, host events and reference traces |
| move | `instant_stop` | Not implemented | Identify state, host events and reference traces |
| move | `inv_walk` | Not implemented | Identify state, host events and reference traces |
| move | `keep_sprint` | Not implemented | Identify state, host events and reference traces |
| move | `no_jump_delay` | Not implemented | Identify state, host events and reference traces |
| move | `no_slow` | Not implemented | Identify state, host events and reference traces |
| move | `null_move` | Not implemented | Identify state, host events and reference traces |
| move | `safe_walk` | Not implemented | Identify state, host events and reference traces |
| move | `sprint` | Not implemented | Identify state, host events and reference traces |
| move | `strafe` | Not implemented | Identify state, host events and reference traces |
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
| utility | `item_use_fix` | Not implemented | Identify state, host events and reference traces |
| utility | `no_hit_delay` | Not implemented | Identify state, host events and reference traces |
| utility | `no_item_release` | Not implemented | Identify state, host events and reference traces |
| utility | `no_use_delay` | Not implemented | Identify state, host events and reference traces |
| utility | `ping_fix` | Not implemented | Identify state, host events and reference traces |
| utility | `refill` | Not implemented | Identify state, host events and reference traces |
| utility | `teams` | Not implemented | Identify state, host events and reference traces |
| utility | `timer` | Not implemented | Identify state, host events and reference traces |
| visual | `anti_debuff` | Not implemented | Identify state, host events and reference traces |
| visual | `block_esp` | Not implemented | Identify state, host events and reference traces |
| visual | `chams` | Not implemented | Identify state, host events and reference traces |
| visual | `chest_esp` | Not implemented | Identify state, host events and reference traces |
| visual | `free_look` | Not implemented | Identify state, host events and reference traces |
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

Inventory contains 67 native module source files, not an assertion of 1:1 module registration.
