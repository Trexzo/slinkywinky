# R341 — Native Backtrack/Lag Range original event6 geometry and opt-in Forge GL diagnostic

This milestone advances the four-module source-shaped reconstruction into **two real native source render callbacks** rather than only CPU predictions and Netty packet observation. The existing `ModuleExecutionBridge.bind_backtrack_source_render` and `bind_lag_range_source_render` handlers are now actually registered in the Java8/JNI source session. Both execute in one event-6 frame over exactly the same immutable source-camera snapshot. Native never retains Java/world refs or invokes OpenGL.

## JNI / typed owner

`SourceRuntimePreview.observeBoxes()` is a new synchronous Java8 JNI callback (strict 12-double array: previous/current target XYZ, camera XYZ, partial tick, previous/current yaw). JNI verifies owner thread and world/player generation, source numeric bounds and invokes both native event6 geometry controllers. A 30-double value-only receipt returns draw flag, camera-relative center, half-scale, rotation flag/angle, filled flag, line width and diagnostic RGB/alpha for each controller. A disabled module yields a valid `draw=false` row; it does not crash the other enabled source model. Immutable `SourceBox` and `SourceBoxes` Java types enforce bounds.

`SourceWorldReadOnlyHost.BoxPort` and `observeBoxes` share the SAME Java object-identity world owner and generation as R337-R340. Source request is admitted only after a valid world tick; absent targets skip; foreign thread, midrender world replacement and bad arrays retire the native host.

## Real Forge 1.8.9 rendering

`ForgeSourceWorldPort.captureBoxes()` reads the camera, crosshair entity, interpolated target center and yaw, but **does NOT recover the held server-side remote historical position**; actual original Backtrack ghost geometry is still NOT recovered. An explicit opt-in `-Dslinkywinky.r341SourceBoxDiagnostic=true` (requiring the separate existing read-only source preview with manual F9) enables a new `RenderWorldLastEvent` listener. It calls the actual C++ e6 geometry via JNI and submits outline geometry via `ForgeSourceBoxDiagnosticRenderer`, which guards all bounds and restores OpenGL matrix and attribute state even if a draw raises. The source e18 settings opt in to render boxes only under this mode. Backtrack still fails closed in the actual Forge tick without original reach admission; Lag Range may render an **observed current-target diagnostic** when its closer/third-person conditions are met. It is NOT visual parity.

## Verification

New Java8 JNI CTest `r341_java8_source_event6_native_boxes` tests the **real native callbacks** with controlled source tick/packet inputs, two simultaneous boxes, exact 0.6x2.0 scale and camera interpolation/yaw, module disable independence, wrong world and thread denial, host render skip and world replacement retirement. ForgeGradle JDK8 compiles/packets the new render code and checks all source observer classes, read-only/JNI/GL lifecycle guards in the distributable JAR. Existing JNI/Netty tests remain.

**Limitations:** Source input and GL display run only in explicitly admitted singleplayer debug profile, not original injector; original real source box positions/palette/state, actual live in-process acceptance, packet hold/delay, timer/key writes and full 66-module original 1:1 functionality remain outstanding. PR #2 draft, PR #1 unchanged, no decrypted original binaries uploaded.