cmake_minimum_required(VERSION 3.20)
project(slinkywinky_reference_compat VERSION 0.1 LANGUAGES CXX)

# Entirely independent test target. Existing SlinkyWinky runtime CMake
# remains untouched while the parallel R-series branch is active.
add_library(slinkywinky_reference_event_model INTERFACE)
target_compile_features(slinkywinky_reference_event_model INTERFACE cxx_std_17)
target_include_directories(slinkywinky_reference_event_model INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(slinkywinky_reference_event_model INTERFACE /W4 /WX)
else()
  target_compile_options(slinkywinky_reference_event_model INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()
include(CTest)
if(BUILD_TESTING)
  add_executable(slinkywinky_reference_event_replay_test tests/test_event_replay.cpp)
  target_link_libraries(slinkywinky_reference_event_replay_test PRIVATE slinkywinky_reference_event_model)
  add_test(NAME r174_native_event_replay COMMAND slinkywinky_reference_event_replay_test)
  add_executable(slinkywinky_event_fixture_runner tools/run_candidate_event_fixture.cpp)
  target_link_libraries(slinkywinky_event_fixture_runner PRIVATE slinkywinky_reference_event_model)
  add_test(NAME r175_synthetic_event_fixture COMMAND slinkywinky_event_fixture_runner
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/three_modules.tsv")
  # R176: exact same offline lifecycle oracle can consume a private candidate
  # fixture locally; CI only uses invented synthetic data.
  add_executable(slinkywinky_event_lifecycle_test tools/audit_candidate_lifecycle.cpp)
  target_link_libraries(slinkywinky_event_lifecycle_test PRIVATE slinkywinky_reference_event_model)
  add_test(NAME r176_lifecycle_synthetic COMMAND slinkywinky_event_lifecycle_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/three_modules.tsv")
  add_test(NAME r176_reject_duplicate_subscription COMMAND slinkywinky_event_lifecycle_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/duplicate_subscription.tsv")
  set_tests_properties(r176_reject_duplicate_subscription PROPERTIES WILL_FAIL TRUE)
  # R177: compile the existing first-party ReachController itself, not a copy
  # of upstream C++ or a mock. No game/JNI/Java runtime is linked.
  add_library(slinkywinky_reference_firstparty_core STATIC ../src/core.cpp)
  target_include_directories(slinkywinky_reference_firstparty_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  target_compile_features(slinkywinky_reference_firstparty_core PUBLIC cxx_std_17)
  add_executable(slinkywinky_module_execution_bridge_test
    tests/test_module_execution_bridge.cpp)
  target_link_libraries(slinkywinky_module_execution_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r177_reach_callback_bridge
    COMMAND slinkywinky_module_execution_bridge_test)
  # R178: full 66-module metadata -> bridge coverage with a synthetic fixture.
  # Private candidate metadata is supplied only on the user's local machine.
  add_executable(slinkywinky_implementation_coverage_audit
    tools/audit_implementation_coverage.cpp)
  target_link_libraries(slinkywinky_implementation_coverage_audit PRIVATE
    slinkywinky_reference_event_model slinkywinky_reference_firstparty_core)
  add_test(NAME r178_synthetic_implementation_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/three_modules_with_reach.tsv")
  add_test(NAME r178_reject_duplicate_catalog_entry
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/duplicate_subscription.tsv")
  add_test(NAME r178_reject_wrong_reach_slot
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/reach_wrong_slot.tsv")
  set_tests_properties(r178_reject_duplicate_catalog_entry
    r178_reject_wrong_reach_slot PROPERTIES WILL_FAIL TRUE)
endif()
