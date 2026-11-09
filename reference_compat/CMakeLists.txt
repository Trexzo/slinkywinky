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
  # R179: two independent module decisions through one metadata-backed router.
  add_executable(slinkywinky_no_jump_delay_bridge_test
    tests/test_no_jump_delay_bridge.cpp)
  target_link_libraries(slinkywinky_no_jump_delay_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r179_no_jump_delay_bridge COMMAND slinkywinky_no_jump_delay_bridge_test)
  add_test(NAME r179_two_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/four_modules_with_two_decisions.tsv")
  add_test(NAME r179_reject_wrong_jump_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_jump_event.tsv")
  add_test(NAME r179_reject_wrong_jump_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_jump_category.tsv")
  set_tests_properties(r179_reject_wrong_jump_event
    r179_reject_wrong_jump_category PROPERTIES WILL_FAIL TRUE)
  # R180: independent opposite-key policy and three-binding manifest.
  add_executable(slinkywinky_null_move_bridge_test
    tests/test_null_move_bridge.cpp)
  target_link_libraries(slinkywinky_null_move_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r180_null_move_bridge COMMAND slinkywinky_null_move_bridge_test)
  add_test(NAME r180_three_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/five_modules_with_three_decisions.tsv")
  add_test(NAME r180_reject_wrong_null_move_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_null_move_event.tsv")
  add_test(NAME r180_reject_wrong_null_move_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_null_move_category.tsv")
  set_tests_properties(r180_reject_wrong_null_move_event
    r180_reject_wrong_null_move_category PROPERTIES WILL_FAIL TRUE)
  # R181: test native event-16 priority 200 Instant Stop before 100 Null Move,
  # with a single shared offline movement frame. No JVM/game process needed.
  add_executable(slinkywinky_instant_stop_bridge_test
    tests/test_instant_stop_bridge.cpp)
  target_link_libraries(slinkywinky_instant_stop_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r181_instant_stop_and_null_move
    COMMAND slinkywinky_instant_stop_bridge_test)
  add_test(NAME r181_four_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/six_modules_with_four_decisions.tsv")
  add_test(NAME r181_reject_wrong_instant_stop_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_instant_stop_event.tsv")
  add_test(NAME r181_reject_wrong_instant_stop_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_instant_stop_category.tsv")
  add_test(NAME r181_reject_wrong_instant_stop_priority
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_instant_stop_priority.tsv")
  set_tests_properties(r181_reject_wrong_instant_stop_event
    r181_reject_wrong_instant_stop_category
    r181_reject_wrong_instant_stop_priority PROPERTIES WILL_FAIL TRUE)
  # R182: independent No Slow scaling, and complete five-policy catalog audit.
  add_executable(slinkywinky_no_slow_bridge_test
    tests/test_no_slow_bridge.cpp)
  target_link_libraries(slinkywinky_no_slow_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r182_no_slow_bridge COMMAND slinkywinky_no_slow_bridge_test)
  add_test(NAME r182_five_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/seven_modules_with_five_decisions.tsv")
  add_test(NAME r182_reject_wrong_no_slow_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_slow_event.tsv")
  add_test(NAME r182_reject_wrong_no_slow_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_slow_category.tsv")
  add_test(NAME r182_reject_wrong_no_slow_priority
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_slow_priority.tsv")
  set_tests_properties(r182_reject_wrong_no_slow_event
    r182_reject_wrong_no_slow_category
    r182_reject_wrong_no_slow_priority PROPERTIES WILL_FAIL TRUE)

  # R183: two-mode No Hit Delay attack callback and six-policy catalog.
  add_executable(slinkywinky_no_hit_delay_bridge_test
    tests/test_no_hit_delay_bridge.cpp)
  target_link_libraries(slinkywinky_no_hit_delay_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r183_no_hit_delay_bridge COMMAND slinkywinky_no_hit_delay_bridge_test)
  add_test(NAME r183_six_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/eight_modules_with_six_decisions.tsv")
  add_test(NAME r183_reject_wrong_no_hit_delay_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_hit_delay_event.tsv")
  add_test(NAME r183_reject_wrong_no_hit_delay_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_hit_delay_category.tsv")
  add_test(NAME r183_reject_wrong_no_hit_delay_priority
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_hit_delay_priority.tsv")
  set_tests_properties(r183_reject_wrong_no_hit_delay_event
    r183_reject_wrong_no_hit_delay_category
    r183_reject_wrong_no_hit_delay_priority PROPERTIES WILL_FAIL TRUE)

  # R184: stateful No Use Delay late-tick policy and seven-decision coverage.
  add_executable(slinkywinky_no_use_delay_bridge_test
    tests/test_no_use_delay_bridge.cpp)
  target_link_libraries(slinkywinky_no_use_delay_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r184_no_use_delay_bridge COMMAND slinkywinky_no_use_delay_bridge_test)
  add_test(NAME r184_seven_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/nine_modules_with_seven_decisions.tsv")
  add_test(NAME r184_reject_wrong_no_use_delay_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_use_delay_event.tsv")
  add_test(NAME r184_reject_wrong_no_use_delay_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_use_delay_category.tsv")
  add_test(NAME r184_reject_wrong_no_use_delay_priority
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_no_use_delay_priority.tsv")
  set_tests_properties(r184_reject_wrong_no_use_delay_event
    r184_reject_wrong_no_use_delay_category
    r184_reject_wrong_no_use_delay_priority PROPERTIES WILL_FAIL TRUE)

  # R185: paired Item Use Fix before/after events (14/15) and catalog.
  add_executable(slinkywinky_item_use_fix_bridge_test
    tests/test_item_use_fix_bridge.cpp)
  target_link_libraries(slinkywinky_item_use_fix_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r185_item_use_fix_bridge COMMAND slinkywinky_item_use_fix_bridge_test)
  add_test(NAME r185_eight_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ten_modules_with_eight_decisions.tsv")
  add_test(NAME r185_reject_item_use_wrong_event
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_item_use_fix_event.tsv")
  add_test(NAME r185_reject_item_use_wrong_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_item_use_fix_category.tsv")
  add_test(NAME r185_reject_item_use_wrong_priority
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_item_use_fix_priority.tsv")
  add_test(NAME r185_reject_missing_after_callback
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/missing_item_use_fix_after.tsv")
  set_tests_properties(r185_reject_item_use_wrong_event
    r185_reject_item_use_wrong_category
    r185_reject_item_use_wrong_priority
    r185_reject_missing_after_callback PROPERTIES WILL_FAIL TRUE)

  # R186: metadata-backed incoming/outgoing Ping Fix (events 2 and 3).
  add_executable(slinkywinky_ping_fix_bridge_test
    tests/test_ping_fix_bridge.cpp)
  target_link_libraries(slinkywinky_ping_fix_bridge_test PRIVATE
    slinkywinky_reference_event_model
    slinkywinky_reference_firstparty_core)
  add_test(NAME r186_ping_fix_bridge COMMAND slinkywinky_ping_fix_bridge_test)
  add_test(NAME r186_nine_decision_coverage
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/eleven_modules_with_nine_decisions.tsv")
  add_test(NAME r186_reject_ping_wrong_category
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_ping_fix_category.tsv")
  add_test(NAME r186_reject_ping_wrong_receive
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_ping_fix_receive_event.tsv")
  add_test(NAME r186_reject_ping_wrong_send
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_ping_fix_send_event.tsv")
  add_test(NAME r186_reject_ping_wrong_priority
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/wrong_ping_fix_priority.tsv")
  add_test(NAME r186_reject_ping_missing_receive
    COMMAND slinkywinky_implementation_coverage_audit
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/missing_ping_fix_receive.tsv")
  set_tests_properties(r186_reject_ping_wrong_category
    r186_reject_ping_wrong_receive
    r186_reject_ping_wrong_send
    r186_reject_ping_wrong_priority
    r186_reject_ping_missing_receive PROPERTIES WILL_FAIL TRUE)

endif()
