file(MAKE_DIRECTORY "${BUILD_ROOT}/r165_modal_pair_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r165_modal_pair_classes"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/MouseQueueOwner.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java"
  "${PROJECT_ROOT}/tests/java/org/lwjgl/input/Mouse.java"
  "${PROJECT_ROOT}/tests/java/org/lwjgl/input/Keyboard.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R161InputCoverageLedger.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R164OfflineModalGate.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R165ModalInputSession.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R165ModalInputSessionSmoke.java"
  RESULT_VARIABLE compiled)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R165 Java 8 paired modal compile failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r165_modal_pair_classes"
  dev.slinkywinky.attach.R165ModalInputSessionSmoke
  RESULT_VARIABLE executed OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT executed EQUAL 0 OR
   NOT output MATCHES "R165_CALLBACK_EXCEPTION_INPUT_LOCKOUT_PASS" OR
   NOT output MATCHES "R165_NORMAL_MODAL_NORMAL_TWO_QUEUES_AND_GATE_PASS" OR
   NOT output MATCHES "R165_PARTIAL_KEYBOARD_ARM_COMPENSATED_PASS")
  message(FATAL_ERROR "R165 isolated input session failed: ${output} ${errors}")
endif()
message(STATUS "R165_JAVA8_BOTH_OWNERS_AND_GATE_ATOMIC_LIFECYCLE_PASS")
