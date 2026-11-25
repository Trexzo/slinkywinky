# R314 — five inventory classification and ranking models

Base c4777be6b85025c83cd772b0498d3bff2bf7de5d. Directly inspected the candidate source archive in the user's Library; commercial authenticity remains unverified. Candidate C++ file SHA-256:
- auto_pot.cpp 5947aac83a5e0ae9ac73c503b49cce14d51d0b887f93c0225836dc2492e6a2ca
- auto_soup.cpp d8ab8ff7634d6bf8d6d7714b0f6b74d778c7f02b51ea7507dfea8e072101ea05
- auto_rod.cpp ffe2cb8d470b0317148a4089b4fc2b57aabc65fe9e0121451bfb0973eced4bc8
- refill.cpp 370a87995c285b1c8888c6dffd3c192b73821167154d46aabb4e0dae1268d972
- inv_manager.cpp 36d16964bcd7bc4d1c1db0726c7b1e957b9f1c97d7a82533eb049bf3c9a83c8c

The new shared inventory sample type holds only supplied item IDs/metadata/presence. No JNI, input, inventory click, network or game integration.
Auto Pot: splash/health metadata and first-match sentinel; activation count arithmetic from source.
Auto Soup: inclusive health gates, circular order, current-slot exclusion, custom list and stew classification. Explicit synthetic continuation booleans replace unresolved random-walk state.
Auto Rod: first throwable/first-or-last rod classification depending on settings.
Refill: 27 slot classes, caller-supplied class offsets, empty hotbar check. Offsets are not asserted as recovered defaults.
Inv Manager: exact paired-kind mapping and direct-score priority. Inputs are supplied scores, not recovered item/enchantment scoring.

These are partial offline models, NOT full state machines. All action, delay, lifecycle, settings and runtime behavior outside the listed functions remain missing. Defensive invalid-input guards are intentional. No local compiler execution was performed in this turn. Combined CTest and full CI pending. Matrix records 49 offline controllers, 6 experimental models, 11 missing original modules; Chest Stealer remains descriptor-only. No new native bindings or runtime acceptance. Both PRs remain draft and separate.
