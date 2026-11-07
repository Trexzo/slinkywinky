cmake_minimum_required(VERSION 3.20)
project(slinkywinky VERSION 0.1.0 LANGUAGES CXX)

add_library(slinkywinky_core STATIC src/core.cpp src/runtime.cpp src/combat.cpp src/settings.cpp src/session.cpp src/observation.cpp src/hud.cpp src/hud_snapshot.cpp src/menu.cpp src/menu_profile.cpp src/menu_prefs.cpp src/menu_render.cpp src/menu_navigation.cpp src/menu_keyboard_router.cpp src/menu_hit_test.cpp src/menu_mouse_gesture.cpp src/menu_bitmap_font.cpp src/recovered_settings.cpp
  src/piercing_preview_prefs.cpp)
find_package(Threads REQUIRED)
target_link_libraries(slinkywinky_core PUBLIC Threads::Threads)
target_include_directories(slinkywinky_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(slinkywinky_core PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(slinkywinky_core PRIVATE /W4)
else()
  target_compile_options(slinkywinky_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(slinkywinky_bridge SHARED src/bridge.cpp)
target_compile_definitions(slinkywinky_bridge PRIVATE SLINKYWINKY_BRIDGE_BUILD)
target_link_libraries(slinkywinky_bridge PRIVATE slinkywinky_core)
target_include_directories(slinkywinky_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(slinkywinky_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_executable(slinkywinky_offline_demo examples/offline_demo.cpp)
target_link_libraries(slinkywinky_offline_demo PRIVATE slinkywinky_core)
include(CTest)
if(BUILD_TESTING)
  add_executable(slinkywinky_bridge_tests tests/test_bridge.cpp)
  target_link_libraries(slinkywinky_bridge_tests PRIVATE slinkywinky_bridge)
  add_test(NAME bridge_abi COMMAND slinkywinky_bridge_tests)
  add_executable(slinkywinky_hud_snapshot_tests tests/test_hud_snapshot.cpp)
  target_link_libraries(slinkywinky_hud_snapshot_tests PRIVATE slinkywinky_core)
  add_test(NAME hud_snapshot_mailbox COMMAND slinkywinky_hud_snapshot_tests)
  add_executable(slinkywinky_bitmap_font_tests tests/test_menu_bitmap_font.cpp)
  target_link_libraries(slinkywinky_bitmap_font_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_bitmap_font COMMAND slinkywinky_bitmap_font_tests)
  add_executable(slinkywinky_menu_render_tests tests/test_menu_render.cpp)
  target_link_libraries(slinkywinky_menu_render_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_render_lifecycle COMMAND slinkywinky_menu_render_tests)
  add_executable(slinkywinky_menu_navigation_tests tests/test_menu_navigation.cpp)
  target_link_libraries(slinkywinky_menu_navigation_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_navigation_ownership COMMAND slinkywinky_menu_navigation_tests)
  # R108: standalone synthetic event dispatcher; NOT a game input hook.
  add_executable(slinkywinky_menu_keyboard_router_tests tests/test_menu_keyboard_router.cpp)
  target_link_libraries(slinkywinky_menu_keyboard_router_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_keyboard_router_no_game_passthrough_fixture
    COMMAND slinkywinky_menu_keyboard_router_tests)
  add_executable(slinkywinky_menu_hit_tests tests/test_menu_hit_test.cpp)
  target_link_libraries(slinkywinky_menu_hit_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_hit_testing COMMAND slinkywinky_menu_hit_tests)
  add_executable(slinkywinky_menu_mouse_gesture_tests tests/test_menu_mouse_gesture.cpp)
  target_link_libraries(slinkywinky_menu_mouse_gesture_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_mouse_gesture_safety COMMAND slinkywinky_menu_mouse_gesture_tests)
  add_executable(slinkywinky_menu_tests tests/test_menu.cpp)
  target_link_libraries(slinkywinky_menu_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_catalog COMMAND slinkywinky_menu_tests)
  add_executable(slinkywinky_recovered_settings_tests tests/test_recovered_settings.cpp)
  target_link_libraries(slinkywinky_recovered_settings_tests PRIVATE slinkywinky_core)
  add_test(NAME native_decrypted_settings_readonly_catalog COMMAND slinkywinky_recovered_settings_tests)
  add_executable(slinkywinky_piercing_preview_tests tests/test_piercing_preview.cpp)
  target_link_libraries(slinkywinky_piercing_preview_tests PRIVATE slinkywinky_core)
  add_test(NAME native_piercing_preview_persistence COMMAND slinkywinky_piercing_preview_tests)
  add_executable(slinkywinky_menu_profile_tests tests/test_menu_profile.cpp)
  target_link_libraries(slinkywinky_menu_profile_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_profile_binding COMMAND slinkywinky_menu_profile_tests)
  add_executable(slinkywinky_hud_tests tests/test_hud.cpp)
  target_link_libraries(slinkywinky_hud_tests PRIVATE slinkywinky_core)
  add_test(NAME native_hud_model COMMAND slinkywinky_hud_tests)
  add_executable(slinkywinky_observation_tests tests/test_observation.cpp)
  target_link_libraries(slinkywinky_observation_tests PRIVATE slinkywinky_core)
  add_test(NAME observation_ledger COMMAND slinkywinky_observation_tests)
  add_executable(slinkywinky_core_tests tests/test_core.cpp)
  target_link_libraries(slinkywinky_core_tests PRIVATE slinkywinky_core)
  add_test(NAME core_state_models COMMAND slinkywinky_core_tests)
  foreach(unit runtime combat settings session stress)
    add_executable(slinkywinky_${unit}_tests tests/test_${unit}.cpp)
    target_link_libraries(slinkywinky_${unit}_tests PRIVATE slinkywinky_core)
    add_test(NAME ${unit}_models COMMAND slinkywinky_${unit}_tests)
  endforeach()
endif()

option(SLINKYWINKY_BUILD_JNI "Build optional JVM integration smoke bridge" OFF)
if(SLINKYWINKY_BUILD_JNI)
  find_package(JNI REQUIRED)
  add_library(slinkywinky_jni SHARED src/jni_bridge.cpp src/bridge.cpp)
  target_compile_definitions(slinkywinky_jni PRIVATE SLINKYWINKY_BRIDGE_BUILD)
  target_link_libraries(slinkywinky_jni PRIVATE slinkywinky_core)
  target_include_directories(slinkywinky_jni PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${JNI_INCLUDE_DIRS})
  if(BUILD_TESTING)
    find_package(Java COMPONENTS Runtime Development REQUIRED)
    add_test(NAME jni_java_roundtrip COMMAND ${CMAKE_COMMAND}
      -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
      -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
      -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
      -DLIBRARY_DIR=$<TARGET_FILE_DIR:slinkywinky_jni>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_java_smoke.cmake)
  endif()
endif()

# Read-only native Windows/GDI preview. Never injects or reads Minecraft.
if(WIN32)
  # R67: isolated native WGL fixed-function draw adapter. No hook/injector.
  add_library(slinkywinky_menu_gl11 STATIC src/windows_menu_gl11.cpp
    src/windows_menu_system_font.cpp
    src/windows_menu_overlay_host.cpp src/windows_retained_menu_preview.cpp)
  target_include_directories(slinkywinky_menu_gl11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(slinkywinky_menu_gl11 PUBLIC cxx_std_17)
  target_link_libraries(slinkywinky_menu_gl11 PUBLIC slinkywinky_core opengl32 gdi32)
  if(BUILD_TESTING)
    # R104: no Minecraft JVM/OS input. Isolated foreground/epoch/refusal
    # contract cannot authorize gameplay event capture.
    add_executable(slinkywinky_menu_focus_fence_tests
      tests/test_windows_menu_focus_fence.cpp)
    target_include_directories(slinkywinky_menu_focus_fence_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(slinkywinky_menu_focus_fence_tests PRIVATE cxx_std_17)
    target_link_libraries(slinkywinky_menu_focus_fence_tests PRIVATE user32 opengl32)
    add_test(NAME native_menu_focus_loss_rearm_no_input
      COMMAND slinkywinky_menu_focus_fence_tests)
    add_executable(slinkywinky_gl11_state_tests tests/test_windows_menu_gl11.cpp)
    target_link_libraries(slinkywinky_gl11_state_tests PRIVATE
      slinkywinky_menu_gl11 opengl32 user32 gdi32)
    if(SLINKYWINKY_BUILD_JNI)
      target_link_libraries(slinkywinky_gl11_state_tests PRIVATE slinkywinky_jni)
      target_include_directories(slinkywinky_gl11_state_tests PRIVATE ${JNI_INCLUDE_DIRS})
      target_compile_definitions(slinkywinky_gl11_state_tests PRIVATE SW_R91_TEST_JNI_OVERLAY=1)
    endif()
    add_test(NAME isolated_wgl_menu_state_restore COMMAND slinkywinky_gl11_state_tests)
    set_tests_properties(isolated_wgl_menu_state_restore PROPERTIES TIMEOUT 30)
  endif()
  add_library(slinkywinky_hud_ipc STATIC src/windows_hud_ipc.cpp)
  target_include_directories(slinkywinky_hud_ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(slinkywinky_hud_ipc PUBLIC cxx_std_17)
  target_link_libraries(slinkywinky_hud_ipc PUBLIC slinkywinky_core)
  if(SLINKYWINKY_BUILD_JNI)
    target_link_libraries(slinkywinky_jni PRIVATE slinkywinky_hud_ipc slinkywinky_menu_gl11 opengl32)
  endif()
  if(BUILD_TESTING)
    add_executable(slinkywinky_hud_ipc_tests tests/test_windows_hud_ipc.cpp)
    target_link_libraries(slinkywinky_hud_ipc_tests PRIVATE slinkywinky_hud_ipc)
    add_test(NAME windows_cross_process_hud_ipc COMMAND slinkywinky_hud_ipc_tests)
  endif()
  option(SLINKYWINKY_BUILD_MENU_PREVIEW "Build standalone first-party menu preview" ON)
  if(SLINKYWINKY_BUILD_MENU_PREVIEW)
    add_executable(slinkywinky_menu_preview WIN32 examples/windows_menu_preview.cpp)
    target_link_libraries(slinkywinky_menu_preview PRIVATE slinkywinky_core user32 gdi32)
    target_include_directories(slinkywinky_menu_preview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(slinkywinky_menu_preview PRIVATE cxx_std_17)
    # Standalone visual verification of the actual GL1.1 renderer, not Minecraft.
    add_executable(slinkywinky_gl11_menu_preview WIN32
      examples/windows_gl11_menu_preview.cpp)
    target_link_libraries(slinkywinky_gl11_menu_preview PRIVATE
      slinkywinky_menu_gl11 slinkywinky_core opengl32 user32 gdi32)
    target_include_directories(slinkywinky_gl11_menu_preview PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(slinkywinky_gl11_menu_preview PRIVATE cxx_std_17)
  endif()
  option(SLINKYWINKY_BUILD_HUD_PREVIEW "Build standalone native HUD preview" ON)
  if(SLINKYWINKY_BUILD_HUD_PREVIEW)
    add_executable(slinkywinky_hud_preview WIN32 examples/windows_hud_preview.cpp)
    target_compile_features(slinkywinky_hud_preview PRIVATE cxx_std_17)
    target_link_libraries(slinkywinky_hud_preview PRIVATE slinkywinky_hud_ipc user32 gdi32)
    target_include_directories(slinkywinky_hud_preview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  endif()
endif()

# Injectable Windows architecture: official Java 8 Attach API -> JVMTI native agent.
# Forge is optional test code only and is NOT a runtime dependency.
option(SLINKYWINKY_BUILD_ATTACH "Build the standalone JVM attach loader and agent" OFF)
if(SLINKYWINKY_BUILD_ATTACH)
  if(NOT WIN32)
    message(FATAL_ERROR "The standalone loader currently targets Windows x64")
  endif()
  find_package(JNI REQUIRED)
  find_package(Java REQUIRED COMPONENTS Runtime Development)
  get_filename_component(SW_JDK_BIN "${Java_JAVA_EXECUTABLE}" DIRECTORY)
  get_filename_component(SW_JDK_HOME "${SW_JDK_BIN}" DIRECTORY)
  set(SW_TOOLS_JAR "${SW_JDK_HOME}/lib/tools.jar")
  if(NOT EXISTS "${SW_TOOLS_JAR}")
    message(FATAL_ERROR "The standalone loader requires a full Java 8 JDK containing lib/tools.jar")
  endif()
  # Pin the BSD-licensed ASM 5.2 dependency to a known SHA-256.
  set(SW_ASM_JAR "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/asm-5.2.jar")
  set(SW_ASM_HASH "3e5ea0d7da2c5155ef4f470d9092d42de34e3f53db6589c7c07d6721adf4ba3e")
  if(NOT EXISTS "${SW_ASM_JAR}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty")
    file(DOWNLOAD "https://repo.maven.apache.org/maven2/org/ow2/asm/asm/5.2/asm-5.2.jar"
      "${SW_ASM_JAR}" EXPECTED_HASH "SHA256=${SW_ASM_HASH}"
      TLS_VERIFY ON STATUS SW_ASM_DL_STATUS)
    list(GET SW_ASM_DL_STATUS 0 SW_ASM_DL_CODE)
    if(NOT SW_ASM_DL_CODE EQUAL 0)
      message(FATAL_ERROR "Verified ASM 5.2 dependency unavailable: ${SW_ASM_DL_STATUS}")
    endif()
  endif()
  file(SHA256 "${SW_ASM_JAR}" SW_ASM_CURRENT_HASH)
  if(NOT SW_ASM_CURRENT_HASH STREQUAL SW_ASM_HASH)
    message(FATAL_ERROR "ASM 5.2 SHA-256 integrity check failed")
  endif()
  add_library(slinkywinky_agent SHARED src/agent_entry.cpp src/bridge.cpp)
  target_compile_features(slinkywinky_agent PRIVATE cxx_std_17)
  target_compile_definitions(slinkywinky_agent PRIVATE SLINKYWINKY_BRIDGE_BUILD)
  target_include_directories(slinkywinky_agent PRIVATE ${JNI_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(slinkywinky_agent PRIVATE slinkywinky_core)
  set_target_properties(slinkywinky_agent PROPERTIES OUTPUT_NAME "slinkywinky_agent")

  # Source changes to Java helpers force the POST_BUILD staging commands to rerun.
  set_property(SOURCE src/windows_loader.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/InstrumentationProbe.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/ObservedState.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RendererLivePreflight.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RenderFrameGate.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RenderCallbackFence.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RendererFrameDiagnostic.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RendererWglEvidence.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/KeyboardPageRelay.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/OriginalKeyboardBridgeWeaver.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/attach/AttachMain.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/instrument-agent.mf"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/net/minecraft/launchwrapper/LaunchClassLoader.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/ClassLoaderSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/ObservationSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/NativeObservationSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/NativeHudIpcSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/Vanilla189MappingSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallsiteAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardInputCallsiteAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallsiteAuditSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardInputCallsiteAuditSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalInputOwnershipAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalInputOwnershipSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardQueueOwnerSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardPageRelaySmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R112KeyboardTarget.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R112KeyboardBridgeSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/org/lwjgl/input/Keyboard.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererWeaveAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererWeaveSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RendererLivePreflightSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RendererLivePreflightExactAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererFrameDiagnosticAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderFrameGateSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallbackFenceSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderPreweaveAdmissionSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RenderCallbackProbe.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallbackSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R65Fixture.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderAttachAgent.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderAttachLauncher.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/render-attach-agent.mf"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/render_attach_live_smoke.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/r74_render_attach_smoke.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/r74-render-attach-agent.mf"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/bfk.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R74RenderAttachAgent.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R74AttachLauncher.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R74RenderTarget.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R76WglJvmSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R77WglRenderReceiver.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R77WglRenderTarget.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R77WglAttachLauncher.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/r77_render_attach_wgl_smoke.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/windows_r76_wgl_jni.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_wgl_frame_probe.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/slinkywinky/windows_wgl_frame_probe.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/HookAdmissionSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/ave.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/pk.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/auh.java")
  # R76: disposable Java 8 JNI -> current-WGL validation only.
  # Deliberately built in testing mode; excluded from any runtime release.
  if(BUILD_TESTING)
    add_library(slinkywinky_r76_wgl_fixture SHARED
      src/windows_wgl_frame_probe.cpp tests/windows_r76_wgl_jni.cpp)
    target_include_directories(slinkywinky_r76_wgl_fixture PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include ${JNI_INCLUDE_DIRS})
    target_compile_features(slinkywinky_r76_wgl_fixture PRIVATE cxx_std_17)
    target_link_libraries(slinkywinky_r76_wgl_fixture PRIVATE
      slinkywinky_menu_gl11 opengl32 gdi32 user32)
  endif()
  add_executable(slinkywinky_loader src/windows_loader.cpp)
  target_include_directories(slinkywinky_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(SLINKYWINKY_BUILD_JNI)
    add_dependencies(slinkywinky_loader slinkywinky_jni)
  endif()
  target_compile_features(slinkywinky_loader PRIVATE cxx_std_17)
  add_dependencies(slinkywinky_loader slinkywinky_agent)
  if(BUILD_TESTING)
    add_dependencies(slinkywinky_loader slinkywinky_r76_wgl_fixture)
  endif()
  add_custom_command(TARGET slinkywinky_loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8 -cp "${SW_TOOLS_JAR}"
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/attach/AttachMain.java"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/AttachTarget.java"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8 -cp "${SW_ASM_JAR}"
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/InstrumentationProbe.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/ObservedState.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RendererLivePreflight.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RenderFrameGate.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RenderCallbackFence.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RendererFrameDiagnostic.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RendererWglEvidence.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/KeyboardPageRelay.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/OriginalKeyboardBridgeWeaver.java"
    COMMAND ${CMAKE_COMMAND} -E chdir "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes"
      "${Java_JAR_EXECUTABLE}" xf "${SW_ASM_JAR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf
      "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes/META-INF"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/ASM-5.2-LICENSE.txt"
      "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes/ASM-5.2-LICENSE.txt"
    COMMAND "${Java_JAR_EXECUTABLE}" cfm
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/instrument-agent.mf"
      -C "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes" .
    # Offline tool only: exact Mojang SHA-1 before structural render lookup.
    # Keep audit code in its own JAR and out of the injected Java agent.
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:slinkywinky_loader>/render-audit-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8
      -cp "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/render-audit-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallsiteAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardInputCallsiteAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererWeaveAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalKeyboardBridgeWeaveAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RendererLivePreflightExactAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererFrameDiagnosticAudit.java"
    COMMAND "${Java_JAR_EXECUTABLE}" cf
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_render_audit.jar"
      -C "$<TARGET_FILE_DIR:slinkywinky_loader>/render-audit-classes" .
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8
      -cp "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/net/minecraft/launchwrapper/LaunchClassLoader.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/ClassLoaderSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/ObservationSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/NativeObservationSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/NativeHudIpcSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/Vanilla189MappingSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallsiteAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardInputCallsiteAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallsiteAuditSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardInputCallsiteAuditSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalInputOwnershipAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalInputOwnershipSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardQueueOwnerSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/KeyboardPageRelaySmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R112KeyboardTarget.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R112KeyboardBridgeSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/org/lwjgl/input/Keyboard.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererWeaveAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererWeaveSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RendererLivePreflightSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RendererLivePreflightExactAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalRendererFrameDiagnosticAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderFrameGateSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallbackFenceSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderPreweaveAdmissionSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/RenderCallbackProbe.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderCallbackSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R65Fixture.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R74RenderAttachAgent.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R74RenderTarget.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R76WglJvmSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R77WglRenderReceiver.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R77WglRenderTarget.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R78WglEvidenceSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderAttachAgent.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/HookAdmissionSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/ave.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/bfk.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/pk.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/auh.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/AttachTarget.java"
    # R66: test-only Java8 Attach launcher compiled using the full tools.jar.
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8
      -cp "${SW_TOOLS_JAR}"
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/RenderAttachLauncher.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R74AttachLauncher.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R77WglAttachLauncher.java"
    # The test-agent bundle is deliberately NOT shipped in package artifacts.
    COMMAND "${Java_JAR_EXECUTABLE}" cfm
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_r66_test_agent.jar"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/render-attach-agent.mf"
      -C "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes" .
    COMMAND "${Java_JAR_EXECUTABLE}" cfm
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_r74_test_agent.jar"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/r74-render-attach-agent.mf"
      -C "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes" .
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:slinkywinky_agent>"
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_agent.dll"
    VERBATIM)
  if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
      add_test(NAME native_agent_attach
        COMMAND "${Python3_EXECUTABLE}" -u "${CMAKE_CURRENT_SOURCE_DIR}/tests/attach_smoke.py"
          "$<TARGET_FILE:slinkywinky_loader>" "${SW_JDK_HOME}")
      set_tests_properties(native_agent_attach PROPERTIES TIMEOUT 120)
      add_test(NAME native_render_fixture_attach
        COMMAND "${Python3_EXECUTABLE}" -u
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/render_attach_live_smoke.py"
          "$<TARGET_FILE:slinkywinky_loader>" "${SW_JDK_HOME}")
      set_tests_properties(native_render_fixture_attach PROPERTIES TIMEOUT 70)
      # R74: generated original-name ave/bfk, disposable JVM only.
      add_test(NAME native_r74_obfuscated_render_attach
        COMMAND "${Python3_EXECUTABLE}" -u
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/r74_render_attach_smoke.py"
          "$<TARGET_FILE:slinkywinky_loader>" "${SW_JDK_HOME}")
      set_tests_properties(native_r74_obfuscated_render_attach PROPERTIES TIMEOUT 80)
      # No PID, Minecraft, or hook. Java 8 runs on its own current WGL context.
      add_test(NAME native_r76_java8_wgl_context_admission
        COMMAND "${CMAKE_COMMAND}" -E env
          "SLINKYWINKY_R76_WGL_FIXTURE=1"
          "${Java_JAVA_EXECUTABLE}"
          "-Djava.library.path=$<TARGET_FILE_DIR:slinkywinky_r76_wgl_fixture>"
          -cp "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
          dev.slinkywinky.attach.R76WglJvmSmoke)
      set_tests_properties(native_r76_java8_wgl_context_admission PROPERTIES TIMEOUT 35)
      # R77: Java8 external Attach -> generated bfk.a callback -> JNI WGL ->
      # first-party menu render on the original Java owner -> exact restoration.
      add_test(NAME native_r77_attached_render_jni_wgl_menu
        COMMAND "${Python3_EXECUTABLE}" -u
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/r77_render_attach_wgl_smoke.py"
          "$<TARGET_FILE:slinkywinky_loader>" "${SW_JDK_HOME}")
      set_tests_properties(native_r77_attached_render_jni_wgl_menu PROPERTIES TIMEOUT 90)
      add_test(NAME native_r78_passive_wgl_evidence
        COMMAND "${CMAKE_COMMAND}" -E env
          "SLINKYWINKY_R76_WGL_FIXTURE=1"
          "SLINKYWINKY_R78_WGL_FIXTURE=1"
          "${Java_JAVA_EXECUTABLE}"
          "-Djava.library.path=$<TARGET_FILE_DIR:slinkywinky_r76_wgl_fixture>"
          -cp "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes;$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
          dev.slinkywinky.attach.R78WglEvidenceSmoke
          "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_jni.dll")
      set_tests_properties(native_r78_passive_wgl_evidence PROPERTIES TIMEOUT 35)
      # R79 pure Java 8 fence test: no JNI, GL, Minecraft or injected process.
      add_test(NAME native_r79_admission_before_wgl_and_drain
        COMMAND "${Java_JAVA_EXECUTABLE}"
          -cp "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes;$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
          dev.slinkywinky.attach.RenderCallbackFenceSmoke)
      set_tests_properties(native_r79_admission_before_wgl_and_drain PROPERTIES TIMEOUT 20)
      # R80: synthetic Java8 Instrumentation proxy, no game/native DLL needed.
      add_test(NAME native_r80_preweave_clean_release_and_uncertain_lockout
        COMMAND "${Java_JAVA_EXECUTABLE}"
          -cp "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes;$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
          dev.slinkywinky.attach.RenderPreweaveAdmissionSmoke)
      set_tests_properties(native_r80_preweave_clean_release_and_uncertain_lockout
        PROPERTIES TIMEOUT 20)
    endif()
  endif()
endif()
