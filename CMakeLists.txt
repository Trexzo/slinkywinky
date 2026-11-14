cmake_minimum_required(VERSION 3.20)
project(slinkywinky VERSION 0.1.0 LANGUAGES CXX)

add_library(slinkywinky_core STATIC src/core.cpp src/runtime.cpp src/combat.cpp src/movement.cpp src/settings.cpp src/session.cpp src/observation.cpp src/hud.cpp src/hud_snapshot.cpp src/module_arraylist.cpp src/menu.cpp src/menu_profile.cpp src/menu_prefs.cpp src/menu_render.cpp src/menu_navigation.cpp src/menu_keyboard_router.cpp src/menu_hit_test.cpp src/menu_mouse_gesture.cpp src/menu_bitmap_font.cpp src/recovered_settings.cpp
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
  # R190: completely isolated paired keyboard/mouse handoff and recovery
  # model. No OS hooks, no Minecraft input dispatch, no live ownership.
  add_executable(slinkywinky_menu_input_handoff_ledger_tests
    tests/test_menu_input_handoff_ledger.cpp)
  target_include_directories(slinkywinky_menu_input_handoff_ledger_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(slinkywinky_menu_input_handoff_ledger_tests
    PRIVATE cxx_std_17)
  target_link_libraries(slinkywinky_menu_input_handoff_ledger_tests
    PRIVATE Threads::Threads)
  add_test(NAME r190_paired_input_handoff_recovery_fixture
    COMMAND slinkywinky_menu_input_handoff_ledger_tests)
  # R183: pure C++ exact OS focus/refusal sequence, portable and input-free.
  add_executable(slinkywinky_original_focus_trace_tests
    tests/test_original_window_focus_trace.cpp)
  target_include_directories(slinkywinky_original_focus_trace_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(slinkywinky_original_focus_trace_tests
    PRIVATE cxx_std_17)
  add_test(NAME r183_exact_os_focus_tri_state_negative_controls
    COMMAND slinkywinky_original_focus_trace_tests)
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
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME native_source_gui_inventory_coverage
      COMMAND "${Python3_EXECUTABLE}" -u "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gui_source_inventory.py")
    # R160: any premature live keyboard/mouse queue activation fails CI.
    add_test(NAME r160_no_unverified_live_input_hook
      COMMAND "${Python3_EXECUTABLE}" -u "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r119_live_input_guard.py")
  endif()
  add_executable(slinkywinky_piercing_preview_tests tests/test_piercing_preview.cpp)
  target_link_libraries(slinkywinky_piercing_preview_tests PRIVATE slinkywinky_core)
  add_test(NAME native_piercing_preview_persistence COMMAND slinkywinky_piercing_preview_tests)
  add_executable(slinkywinky_menu_dropdown_popup_tests tests/test_menu_dropdown_popup.cpp)
  target_link_libraries(slinkywinky_menu_dropdown_popup_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_dropdown_popup_preview_safety
           COMMAND slinkywinky_menu_dropdown_popup_tests)
  add_executable(slinkywinky_menu_unload_modal_tests tests/test_menu_unload_modal.cpp)
  target_link_libraries(slinkywinky_menu_unload_modal_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_unload_confirmation_preview_safety
           COMMAND slinkywinky_menu_unload_modal_tests)
  add_executable(slinkywinky_recovered_boolean_preview_tests
    tests/test_recovered_boolean_preview.cpp)
  target_link_libraries(slinkywinky_recovered_boolean_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_boolean_preview_safety
    COMMAND slinkywinky_recovered_boolean_preview_tests)
  add_executable(slinkywinky_recovered_choice_preview_tests
    tests/test_recovered_choice_preview.cpp)
  target_link_libraries(slinkywinky_recovered_choice_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_choice_preview_safety
    COMMAND slinkywinky_recovered_choice_preview_tests)
  add_executable(slinkywinky_recovered_slider_preview_tests
    tests/test_recovered_slider_preview.cpp)
  target_link_libraries(slinkywinky_recovered_slider_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_slider_preview_safety
    COMMAND slinkywinky_recovered_slider_preview_tests)
  add_executable(slinkywinky_recovered_range_preview_tests
    tests/test_recovered_range_preview.cpp)
  target_link_libraries(slinkywinky_recovered_range_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_range_preview_safety
    COMMAND slinkywinky_recovered_range_preview_tests)
  add_executable(slinkywinky_recovered_multi_preview_tests
    tests/test_recovered_multi_preview.cpp)
  target_link_libraries(slinkywinky_recovered_multi_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_multiselect_preview_safety
    COMMAND slinkywinky_recovered_multi_preview_tests)
  add_executable(slinkywinky_menu_adaptive_scroll_tests
    tests/test_menu_adaptive_scroll.cpp)
  target_link_libraries(slinkywinky_menu_adaptive_scroll_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_narrow_menu_adaptive_scroll
    COMMAND slinkywinky_menu_adaptive_scroll_tests)
  add_executable(slinkywinky_recovered_color_preview_tests
    tests/test_recovered_color_preview.cpp)
  target_link_libraries(slinkywinky_recovered_color_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_color_preview_safety
    COMMAND slinkywinky_recovered_color_preview_tests)
  # R198: portable source-informed 6-digit hex, HSL, and eight-recent
  # color-picker state. Completely independent of Minecraft, WGL, JNI.
  add_executable(slinkywinky_color_popup_model_tests
    tests/test_menu_color_popup_preview.cpp)
  target_include_directories(slinkywinky_color_popup_model_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(slinkywinky_color_popup_model_tests
    PRIVATE cxx_std_17)
  add_test(NAME r198_native_color_popup_hex_hsl_recent_fixture
    COMMAND slinkywinky_color_popup_model_tests)
  add_executable(slinkywinky_recovered_text_preview_tests
    tests/test_recovered_text_preview.cpp)
  target_link_libraries(slinkywinky_recovered_text_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_text_preview_safety
    COMMAND slinkywinky_recovered_text_preview_tests)
  add_executable(slinkywinky_recovered_action_button_tests
    tests/test_recovered_action_buttons.cpp)
  target_link_libraries(slinkywinky_recovered_action_button_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_friend_action_button_isolation
    COMMAND slinkywinky_recovered_action_button_tests)
  add_executable(slinkywinky_recovered_bind_preview_tests
    tests/test_recovered_bind_preview.cpp)
  target_link_libraries(slinkywinky_recovered_bind_preview_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_keybind_preview_safety
    COMMAND slinkywinky_recovered_bind_preview_tests)
  add_executable(slinkywinky_recovered_profiles_interaction_tests
    tests/test_recovered_profiles_interaction.cpp)
  target_link_libraries(slinkywinky_recovered_profiles_interaction_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_profiles_create_and_actions
    COMMAND slinkywinky_recovered_profiles_interaction_tests)
  add_executable(slinkywinky_recovered_profile_tooltip_tests
    tests/test_recovered_profile_tooltips.cpp)
  target_link_libraries(slinkywinky_recovered_profile_tooltip_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_profile_tooltip_hover_isolation
    COMMAND slinkywinky_recovered_profile_tooltip_tests)
  add_executable(slinkywinky_recovered_aim_speed_alias_tests
    tests/test_recovered_aim_speed_alias.cpp)
  target_link_libraries(slinkywinky_recovered_aim_speed_alias_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_aim_speed_shared_settings
    COMMAND slinkywinky_recovered_aim_speed_alias_tests)
  add_executable(slinkywinky_recovered_numeric_tooltip_tests
    tests/test_recovered_numeric_tooltips.cpp)
  target_link_libraries(slinkywinky_recovered_numeric_tooltip_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_numeric_tooltips_hover
    COMMAND slinkywinky_recovered_numeric_tooltip_tests)
  add_executable(slinkywinky_recovered_numeric_drag_tests
    tests/test_recovered_numeric_drag.cpp)
  target_link_libraries(slinkywinky_recovered_numeric_drag_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_numeric_drag_transaction
    COMMAND slinkywinky_recovered_numeric_drag_tests)
  add_executable(slinkywinky_expander_geometry_tests
    tests/test_menu_expander_geometry.cpp)
  target_link_libraries(slinkywinky_expander_geometry_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_source_expander_full_text_hit
    COMMAND slinkywinky_expander_geometry_tests)
  add_executable(slinkywinky_vertical_settings_tests
    tests/test_menu_settings_vertical.cpp)
  target_link_libraries(slinkywinky_vertical_settings_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_gui_vertical_settings_css
    COMMAND slinkywinky_vertical_settings_tests)
  add_executable(slinkywinky_setting_layout_tests
    tests/test_menu_setting_layout.cpp)
  target_link_libraries(slinkywinky_setting_layout_tests PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_gui_setting_layout_css COMMAND slinkywinky_setting_layout_tests)
  add_executable(slinkywinky_control_motion_tests
    tests/test_menu_control_motion.cpp)
  target_link_libraries(slinkywinky_control_motion_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_gui_control_visual_motion
    COMMAND slinkywinky_control_motion_tests)
  add_executable(slinkywinky_visual_motion_tests
    tests/test_menu_visual_motion.cpp)
  target_link_libraries(slinkywinky_visual_motion_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_gui_tab_open_motion
    COMMAND slinkywinky_visual_motion_tests)
  add_executable(slinkywinky_sprint_model_tests
    tests/test_sprint_module.cpp)
  target_link_libraries(slinkywinky_sprint_model_tests PRIVATE slinkywinky_core)
  add_test(NAME native_sprint_intent_model_and_gui_profile
    COMMAND slinkywinky_sprint_model_tests)
  add_executable(slinkywinky_menu_source_boolean_tests
    tests/test_menu_source_boolean_binding.cpp)
  target_link_libraries(slinkywinky_menu_source_boolean_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_gui_boolean_to_profile
    COMMAND slinkywinky_menu_source_boolean_tests)
  add_executable(slinkywinky_menu_source_binding_tests
    tests/test_menu_source_profile_binding.cpp)
  target_link_libraries(slinkywinky_menu_source_binding_tests
    PRIVATE slinkywinky_core)
  add_test(NAME native_recovered_gui_to_native_profile
    COMMAND slinkywinky_menu_source_binding_tests)
  add_executable(slinkywinky_menu_profile_tests tests/test_menu_profile.cpp)
  target_link_libraries(slinkywinky_menu_profile_tests PRIVATE slinkywinky_core)
  add_test(NAME native_menu_profile_binding COMMAND slinkywinky_menu_profile_tests)
  add_executable(slinkywinky_hud_tests tests/test_hud.cpp)
  target_link_libraries(slinkywinky_hud_tests PRIVATE slinkywinky_core)
  add_test(NAME native_hud_model COMMAND slinkywinky_hud_tests)
  # R207: genuine runtime-active status only, never GUI preview toggles.
  add_executable(slinkywinky_module_arraylist_tests
    tests/test_module_arraylist.cpp)
  target_link_libraries(slinkywinky_module_arraylist_tests
    PRIVATE slinkywinky_core)
  add_test(NAME r207_module_arraylist_runtime_state_only
    COMMAND slinkywinky_module_arraylist_tests)
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
    add_test(NAME r158_java8_retained_profile_receipt COMMAND ${CMAKE_COMMAND}
      -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
      -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
      -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r158_receipt.cmake)
    add_test(NAME r160_java8_mouse_owner_no_game_hook COMMAND ${CMAKE_COMMAND}
      -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
      -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
      -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r160_mouse_owner.cmake)
    add_test(NAME r161_java8_paired_input_rollback_no_game_hook COMMAND ${CMAKE_COMMAND}
      -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
      -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
      -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r161_paired_input.cmake)
    add_test(NAME r165_java8_atomic_modal_and_rollback_fixture COMMAND ${CMAKE_COMMAND}
      -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
      -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
      -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r165_modal_pair.cmake)
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
    add_executable(slinkywinky_retained_pointer_tests
      tests/test_windows_retained_pointer.cpp)
    target_link_libraries(slinkywinky_retained_pointer_tests PRIVATE
      slinkywinky_menu_gl11 opengl32 user32 gdi32)
    add_test(NAME native_retained_menu_source_mouse_transactions
      COMMAND slinkywinky_retained_pointer_tests)
      # R166: isolated Java8 JNI -> actual retained C++ WGL pointer
      # transaction. Separate DLL; never link it to production JNI/agent.
      if(SLINKYWINKY_BUILD_JNI)
        add_library(slinkywinky_r166_pointer_test_jni SHARED
          tests/r166_retained_pointer_jni.cpp)
        target_include_directories(slinkywinky_r166_pointer_test_jni
          PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${JNI_INCLUDE_DIRS})
        target_link_libraries(slinkywinky_r166_pointer_test_jni PRIVATE
          slinkywinky_menu_gl11 opengl32 gdi32 user32)
        set_target_properties(slinkywinky_r166_pointer_test_jni PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/r166_disposable_only")
        add_test(NAME r166_java8_jni_to_retained_wgl_pointer_profile
          COMMAND ${CMAKE_COMMAND}
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r166_pointer_jni.cmake)
        set_tests_properties(
          r166_java8_jni_to_retained_wgl_pointer_profile
          PROPERTIES TIMEOUT 40)
        add_test(NAME r167_java8_jni_recovered_slider_to_native_profile
          COMMAND ${CMAKE_COMMAND}
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r167_slider_jni.cmake)
        set_tests_properties(
          r167_java8_jni_recovered_slider_to_native_profile
          PROPERTIES TIMEOUT 40)
        add_test(NAME r168_java8_jni_recovered_boolean_settings_paging
          COMMAND ${CMAKE_COMMAND}
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r168_boolean_jni.cmake)
        set_tests_properties(
          r168_java8_jni_recovered_boolean_settings_paging
          PROPERTIES TIMEOUT 40)
        add_test(NAME r169_java8_jni_retained_dropdown_option_routing
          COMMAND ${CMAKE_COMMAND}
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r169_dropdown_jni.cmake)
        set_tests_properties(
          r169_java8_jni_retained_dropdown_option_routing
          PROPERTIES TIMEOUT 40)
          add_test(NAME r175_java8_original_retained_passive_hover
            COMMAND ${CMAKE_COMMAND}
              -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
              -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
              -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
              -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
              -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r175_passive_hover.cmake)
          set_tests_properties(
            r175_java8_original_retained_passive_hover
            PROPERTIES TIMEOUT 40)
          # R177: real GL11 pixel readback in the exact source tooltip
          # region. Disposable WGL/JNI fixture, never a production hook.
          add_test(NAME r177_java8_jni_source_tooltip_pixel_proof
            COMMAND ${CMAKE_COMMAND}
              -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
              -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
              -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
              -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
              -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r177_tooltip_pixels.cmake)
          set_tests_properties(
            r177_java8_jni_source_tooltip_pixel_proof
            PROPERTIES TIMEOUT 45)
          # R179: source-indexed actual native GL11 slider rectangle and
          # foreground hover-hit counters. Disposable JNI test DLL only.
          add_test(NAME r179_java8_jni_native_exact_source_geometry
            COMMAND ${CMAKE_COMMAND}
              -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
              -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
              -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
              -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
              -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r179_source_target_jni.cmake)
          set_tests_properties(
            r179_java8_jni_native_exact_source_geometry
            PROPERTIES TIMEOUT 45)
          # R182: synthetic foreground transitions on the actual retained
          # WGL renderer. The test DLL is separate from the production agent.
          add_test(NAME r182_java8_jni_native_focus_cycle
            COMMAND ${CMAKE_COMMAND}
              -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
              -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
              -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
              -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
              -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r182_focus_jni.cmake)
          set_tests_properties(r182_java8_jni_native_focus_cycle
            PROPERTIES TIMEOUT 45)
          # R183: refusal is a third status, not an inferred background.
          add_test(NAME r183_java8_jni_native_exact_os_focus
            COMMAND ${CMAKE_COMMAND}
              -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
              -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
              -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
              -DLIBRARY_PATH=$<TARGET_FILE:slinkywinky_r166_pointer_test_jni>
              -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r183_focus_jni.cmake)
          set_tests_properties(r183_java8_jni_native_exact_os_focus
            PROPERTIES TIMEOUT 45)
          # R177: the source-only Java 8 receipt requires the attach
          # agent JAR, not merely the standalone JNI-preview DLL.
          # The JNI-only workflow has no slinkywinky_loader target.
          if(SLINKYWINKY_BUILD_ATTACH)
            add_test(NAME r176_java8_original_retained_hover_receipt
              COMMAND ${CMAKE_COMMAND}
                -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
                -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
                -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
                -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
                -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r176_hover_receipt.cmake)
            set_tests_properties(
              r176_java8_original_retained_hover_receipt
              PROPERTIES TIMEOUT 40)
            add_test(NAME r178_java8_original_slider_hover_dwell_schedule
              COMMAND ${CMAKE_COMMAND}
                -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
                -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
                -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
                -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
                -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r178_hover_schedule.cmake)
            set_tests_properties(
              r178_java8_original_slider_hover_dwell_schedule
              PROPERTIES TIMEOUT 40)
            add_test(NAME r179_java8_original_native_source_target_receipt
              COMMAND ${CMAKE_COMMAND}
                -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
                -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
                -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
                -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
                -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r179_source_receipt.cmake)
            set_tests_properties(
              r179_java8_original_native_source_target_receipt
              PROPERTIES TIMEOUT 40)
            add_test(NAME r182_java8_original_native_focus_cycle_receipt
              COMMAND ${CMAKE_COMMAND}
                -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
                -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
                -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
                -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
                -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r182_focus_receipt.cmake)
            set_tests_properties(r182_java8_original_native_focus_cycle_receipt
              PROPERTIES TIMEOUT 40)
            add_test(NAME r183_java8_original_exact_window_focus_receipt
              COMMAND ${CMAKE_COMMAND}
                -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
                -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
                -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
                -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
                -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r183_focus_receipt.cmake)
            set_tests_properties(r183_java8_original_exact_window_focus_receipt
              PROPERTIES TIMEOUT 40)
          endif()
      endif()
    set_tests_properties(native_retained_menu_source_mouse_transactions
      PROPERTIES TIMEOUT 30)
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
      slinkywinky_menu_gl11 slinkywinky_core opengl32 user32 gdi32 comdlg32)
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
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/OriginalGuiScreenProbe.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/OriginalPointerArbitrationAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/MouseQueueOwner.java"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R162InputOrderLedger.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R162InputOrderLedgerSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R163InputControlGraph.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R163InputControlGraphSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineModalGate.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineInputCut.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineInputCutSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R171VanillaJarInputSurface.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R171VanillaJarInputSurfaceSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R194OriginalInputCallReachability.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R194OriginalInputCallReachabilitySmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R195PossibleInputDispatch.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R195PossibleInputDispatchSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R173OriginalGuiFieldAudit.java"
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
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/OriginalGuiScreenProbe.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/OriginalPointerArbitrationAudit.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/MouseQueueOwner.java"
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
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/OriginalInputOwnershipAudit.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R162InputOrderLedger.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R162InputOrderLedgerSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R163InputControlGraph.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R163InputControlGraphSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineModalGate.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineInputCut.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineInputCutSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R171VanillaJarInputSurface.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R171VanillaJarInputSurfaceSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R194OriginalInputCallReachability.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R194OriginalInputCallReachabilitySmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R195PossibleInputDispatch.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R195PossibleInputDispatchSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R173OriginalGuiFieldAudit.java"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R162InputOrderLedger.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R162InputOrderLedgerSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R163InputControlGraph.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R163InputControlGraphSmoke.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineModalGate.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineInputCut.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R164OfflineInputCutSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R171VanillaJarInputSurface.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R171VanillaJarInputSurfaceSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R194OriginalInputCallReachability.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R194OriginalInputCallReachabilitySmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R195PossibleInputDispatch.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R195PossibleInputDispatchSmoke.java"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/R173OriginalGuiFieldAudit.java"
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
      # R176: reject forged/duplicated source-JVM hover receipts.
      add_test(NAME r176_original_client_hover_marker_negatives
        COMMAND "${Python3_EXECUTABLE}" -B
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r176_original_hover_receipt.py")
      set_tests_properties(r176_original_client_hover_marker_negatives
        PROPERTIES TIMEOUT 25)
        # R178: original-game hover dwell must be exactly 60 source frames
        # and consistent with separately verified R176 foreground counts.
        add_test(NAME r178_original_client_slider_window_marker_negatives
          COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r178_original_slider_window.py")
        set_tests_properties(r178_original_client_slider_window_marker_negatives
          PROPERTIES TIMEOUT 25)
        add_test(NAME r179_original_client_native_target_marker_negatives
          COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r179_original_native_target.py")
        set_tests_properties(r179_original_client_native_target_marker_negatives
          PROPERTIES TIMEOUT 25)
        # R181: require repeated true original foreground samples for the
        # exact last-painted source slider, without admitting click delivery.
        add_test(NAME r181_original_client_foreground_source_hit_negatives
          COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r181_original_foreground_source_hits.py")
        set_tests_properties(r181_original_client_foreground_source_hit_negatives
          PROPERTIES TIMEOUT 25)
        add_test(NAME r182_original_client_focus_cycle_marker_negatives
          COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r182_original_focus_cycle.py")
        set_tests_properties(r182_original_client_focus_cycle_marker_negatives
          PROPERTIES TIMEOUT 25)
        add_test(NAME r183_original_exact_os_focus_marker_negatives
          COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r183_original_window_focus.py")
        set_tests_properties(r183_original_exact_os_focus_marker_negatives
          PROPERTIES TIMEOUT 25)
        # R206: foreground observations and even exact cursor hover
        # may not silently promote R99 render receipts into live input.
        add_test(NAME r206_original_read_only_input_scope_negatives
          COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r206_original_input_readiness.py")
        set_tests_properties(r206_original_read_only_input_scope_negatives
          PROPERTIES TIMEOUT 25)
      add_test(NAME r162_java8_original_input_instruction_order_fixture
        COMMAND "${CMAKE_COMMAND}"
          -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
          -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
          -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r162_input_ledger.cmake")
      set_tests_properties(r162_java8_original_input_instruction_order_fixture
        PROPERTIES TIMEOUT 30)
      add_test(NAME r163_java8_original_input_control_flow_fixture
        COMMAND "${CMAKE_COMMAND}"
          -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
          -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
          -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r163_input_graph.cmake")
      set_tests_properties(r163_java8_original_input_control_flow_fixture
        PROPERTIES TIMEOUT 30)
      add_test(NAME r164_java8_whole_input_method_cut_fixture
        COMMAND "${CMAKE_COMMAND}"
          -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
          -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
          -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r164_input_cut.cmake")
      set_tests_properties(r164_java8_whole_input_method_cut_fixture
        PROPERTIES TIMEOUT 30)
        add_test(NAME r171_java8_whole_jar_input_surface_fixture
          COMMAND "${CMAKE_COMMAND}"
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r171_input_surface.cmake")
        set_tests_properties(r171_java8_whole_jar_input_surface_fixture
          PROPERTIES TIMEOUT 30)
        # R194: test-only declared-target transitive input graph, no JVM attach.
        add_test(NAME r194_java8_original_input_call_reachability_fixture
          COMMAND "${CMAKE_COMMAND}"
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r194_input_reachability.cmake")
        set_tests_properties(r194_java8_original_input_call_reachability_fixture
          PROPERTIES TIMEOUT 30)
        # R195: offline source-only possible virtual/interface dispatch.
        add_test(NAME r195_java8_possible_original_input_dispatch_fixture
          COMMAND "${CMAKE_COMMAND}"
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r195_possible_input_dispatch.cmake")
        set_tests_properties(r195_java8_possible_original_input_dispatch_fixture
          PROPERTIES TIMEOUT 30)
        # R172: Java 8 synthetic receipt test, no original process and
        # no GUI/mouse or input owner hook. Production JNI stays read-only.
        add_test(NAME r172_java8_original_window_pointer_receipt
          COMMAND "${CMAKE_COMMAND}"
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r172_window_pointer.cmake")
        set_tests_properties(r172_java8_original_window_pointer_receipt
          PROPERTIES TIMEOUT 30)
        add_test(NAME r173_java8_original_gui_presence_observer
          COMMAND "${CMAKE_COMMAND}"
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r173_gui_presence.cmake")
        set_tests_properties(r173_java8_original_gui_presence_observer
          PROPERTIES TIMEOUT 30)
        add_test(NAME r174_java8_original_input_overlap_risk
          COMMAND "${CMAKE_COMMAND}"
            -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
            -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
            -DBUILD_ARTIFACT_DIR=$<TARGET_FILE_DIR:slinkywinky_loader>
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_r174_input_overlap.cmake")
        set_tests_properties(r174_java8_original_input_overlap_risk
          PROPERTIES TIMEOUT 30)
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
