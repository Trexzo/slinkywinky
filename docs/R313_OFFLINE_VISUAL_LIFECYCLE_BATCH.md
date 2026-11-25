# R313 — four offline visual models

Base R312 2401378d55b74015906b3ba6a9adf373038e5707: 13/13 GitHub checks passed. The following candidate source files were inspected in the user Library C++ archive, SHA-256 c06b4c6033e2d19de86bad8f19127b0a3a68cfc496130233ebc9ff29f4540915:

- gui.cpp 78a0571c98b146e0941ce98e6aba89abe5106dc2c2cbe38af7bafa270a8c8276
- name_tags.cpp 621876d207438103b38179382fe8e1ff9cee68671cc9c8a33e5322e9849bad4c
- pointers.cpp (source scalar formulas; no world collection)
- trajectories.cpp 2cd5eebd63e090421e230ec439e0ae8bd5c36ba21c70111894188d987f96cd21

GUI: input-decision fixture, queued open/close request, own-descriptor transitions, cursor and settings-save *intents*, close/reset/shutdown gates, redraw flags and explicitly supplied appearance. No event is consumed in Minecraft; no OS calls or persistence. Duplicate transitions and shutdown admission are defensive. No claim of original scale table, NanoVG layout or live integration.

Name Tags: synthetic roster and structured components for self/third-person, relationship bit-2, death, range and font gating, 100-entry cap, plain versus formatted name selection, health/absorption scaling and equipment presence. No original formatted text/color tier/enchantment sorting or projection. Encounter order is retained rather than assuming the candidate comparator. Input and output use supplied data only.

Pointers: scalar gradient/radius/scale and 3D tilt arithmetic, including equal-endpoint epsilon. This is a presentation subset only, no player collection, arrow direction, relationship logic or actual rendering.

Trajectories: candidate item selection, bow charge, two power samples, gravity and 100-step free-flight drag/point recurrence. No world access, collision prediction, launch direction or render path. A caller supplies synthetic start and velocity. Defensive bounds are not a claim of exact source edge behavior.

One combined CTest exercises the subset of each controller. CI pending; local compilation not performed. No new live callback registrations or original-commercial parity are established.

Source inventory: 44 partial offline controllers, 6 experimental, 16 missing original table implementations. Descriptor-only Chest Stealer remains out of scope. The full 66-module target is not achieved. PR #1 remains untouched; both drafts remain unmerged.
