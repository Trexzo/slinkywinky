# R42: safe eye coordinates and deterministic real-field observation regression

The Java 8 read-only sampler obtains the player eye height through `getEyeHeight()` (or `func_70047_e()`) and adds it to the mapped player Y-coordinate before sending the frame to the C++ engine. Only known exact methods are accepted. Nonfinite or unreasonable heights reject sampling; all gameplay modules remain disabled.

The disposable Java 8 fixture now tests: (1) world, player and ray target simultaneously present; (2) player eye coordinates, camera yaw/pitch and selected entity XYZ; (3) ray target disappearance without changing session generation; (4) disconnect and fresh-world reentry incrementing generation; (5) nonfinite player position rejection. The fixture runs as a required part of the native Attach API CTest job.

A successful Java fixture isn't proof of an actual populated Minecraft world. The isolated Minecraft 1.8.9 acceptance test to date is the main menu, which validates mapping admission and the no-world path only. Keep this distinction visible. No inputs, packets, rotations or combat actions are applied.
