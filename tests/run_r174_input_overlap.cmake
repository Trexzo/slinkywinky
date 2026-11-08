if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R174 original render risk auditor not built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r174_input_overlap_testclasses")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r174_input_overlap_testclasses"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R174OriginalInputOverlapSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE compile_out ERROR_VARIABLE compile_error)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R174 Java 8 read-only risk fixture compile failed: ${compile_out} ${compile_error}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r174_input_overlap_testclasses;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R174OriginalInputOverlapSmoke
  RESULT_VARIABLE tested OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT tested EQUAL 0 OR
   NOT stdout MATCHES "R174_JAVA8_GUI_INPUT_OVERLAP_PRESS_RELEASE_FOCUS_PASS" OR
   NOT stdout MATCHES "R174_VIEWPORT_SCREEN_CHORD_WRONG_THREAD_LOCKOUT_PASS" OR
   NOT stdout MATCHES "R174_EIGHT_FRAME_READ_ONLY_RISK_AUDIT_PASS" OR
   NOT stdout MATCHES "R174_NATIVE_CLICK_DELIVERY_REMAINS_DISABLED")
  message(FATAL_ERROR "R174 read-only audit failure: ${stdout} ${stderr}")
endif()
message(STATUS "R174_JAVA8_ORIGINAL_INPUT_OVERLAP_RISK_AUDIT_PASS")
