# R298 — Item ESP source-ordered event-18 sampling and count merging

Original reference: `Slinky-Verified-SourceBuild.zip`:
`source/java/src/main/java/dev/slinky/module/impl/ItemEspModule.java`
SHA256: `ed428c48d514d3ca43aa43baaeb9cc6a8f8e81f87450118f123878318b1b652e`.
Drawing/data constants recovered from `RenderConstants.json` SHA256
`3e6a7dba6433f6416ef73ae3193e04a09dd2e9275b0b4890f6432f743f71301e`:
mergeCounts squared distance < 0.0625 (d(1393504)), count label `%u` at 1393611.

Source behaviors recovered in an offline C++ model:
- Event 18 clears rows and refreshes the resolved allowed-item policy even
  if the world is unavailable; event 26 clears both rows and policy;
- event 6 is an original render callback, **not hooked up** by this model;
- policy retains the first valid color for each resolved item ID;
- encounters non-dead EntityItem with a positive stack and valid item,
  within inclusive float-squared range of the player;
- supports selected item colors, or the fallback if `highlight_all`;
- copies original previous/current world positions, caps collection at 100;
- original Row constructor masks stack size to 8 bits (`& 0xFF`);
- only when `show_count`, merges same-ID rows within **strict**
  squared distance < 0.0625, in original nested-loop order, zeroing the
  quantity of contributing rows, clamping anchor count at 99, forming
  its `%u` label, then optionally obtaining external text width;
- world unload drops rows but does not implicitly clear resolved policy.

The order-sensitive count grouping is **not** a connected-components
clustering algorithm; two nearby secondary rows need not combine with a
first row if their individual distance to the first exceeds the radius.
Tests preserve this original behavior rather than silently optimizing it.

File: `reference_compat/include/slinkywinky/item_esp_controller.hpp`.
Regression: `reference_compat/tests/test_item_esp_controller.cpp`.

No Minecraft renderer, item entity enumeration, drawing, native hooks,
font textures, Java input routing or injectable runtime is connected.
This is a source-faithful **offline collection and count-merging model**,
not a completed live Item ESP feature. PR #1 remains untouched.
