# R156: source-identified GUI Boolean settings

Three recovered Boolean controls now edit existing native ClientSettings fields:
- Aim Assist / Conditions / Mouse pressed -> aim_requires_mouse
- Auto Block / Conditions / Damaged -> block_requires_damage
- Reach / Conditions / Sprinting -> reach.require_sprint

The first-party preview gets a source-validated explicit Boolean setter. The new profile binding uses the actual engine flag, not the candidate HTML sample value, and validates the full proposed profile before setting both values. Render frames show the native profile's Boolean even after loading a file, so the GUI cannot misleadingly display stale fixture defaults.

Every other recovered Boolean remains preview-only. Local editor admission still requires the exact visible module tab, expanded card and five-widget page. No gameplay actions or native hooks have been enabled. This is not a claim that the three original Slinky Boolean implementations are known.
