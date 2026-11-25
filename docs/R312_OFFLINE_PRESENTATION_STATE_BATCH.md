# R312 — four-module offline reconstruction batch

Base R311: c2a53efaa6f181d386e06150e300d06e7f8c39fc. Both archives were inspected read-only; their original commercial provenance is unverified. C++ source SHA-256s:
- friends.cpp 94c5a239ff232b830df1f4700400672bfff2724352ad36dd1b74de37622f939e
- debug.cpp 3daa12a84f18edb7e89c90eb36efce4df8bbfcfdc913aed6df8426dfff1802dc
- indicators.cpp 452404278338c2065437461a17ccf43754509a0509959360a32d79eaf34d6134
- mod_overlay.cpp e912196d9e53f01e873b94e933d030aaccdd84b171fd1f27caa069d0fa79a23e

Friends: independent UUID membership/toggle, flag-4 snapshot, bot exclusion for addition, last-seen update, own disable and shutdown. Persistence, bulk UI operations and native dispatch remain unbound.

Debug: typed diagnostic records from caller-supplied events; last-three-digit stamps, hurt threshold/colors, item state transitions and click-count window. No raw packet hook, JNI logging or original format strings. Serial fixture only; saturation/reset are defensive additions.

Indicators: collection subset only: IDs 10/12/14, grounded-arrow filter, approaching comparison, encounter-order cap 10, label formatting and strict 15/30 distance color boundaries; disable and missing-font lifecycle. Original arrows, scale/radius, color tables, rotation and GL/JNI rendering remain unimplemented. Nonfinite input rejection is defensive.

Mod Overlay: entry selection by category, hidden flag and keybind; 50-entry cap, lowercasing and suffix visibility, explicit roster rebuild, profile dirtiness, toggle and 5/s linear fade lifecycle. This is not original NanoVG draw/measurement/sorting/easing, exact source refresh timing or actual GUI integration. Rebuild reconciles the full supplied roster, safe bounded strings and all expired rows are deliberately removed; exact candidate edge behavior is not claimed.

One combined CTest exercises these independent state controllers. Local compiler testing of this commit was not performed. CI on this exact commit must establish compilation and fixture results.

R312 source accounting: 40 offline controllers + six experimental models + 20 missing original table implementations = 66. Chest Stealer is descriptor-only. No newly bound native callback or original runtime parity is claimed. PR #1 untouched; no merge or release.
