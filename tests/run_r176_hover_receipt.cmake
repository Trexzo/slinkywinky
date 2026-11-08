if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R176 Java 8 instrument JAR absent")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r176_hover_receipt_testclasses")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r176_hover_receipt_testclasses"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R176RetainedHoverReceiptSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE comp_out ERROR_VARIABLE comp_err)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R176 Java8 source-limited receipt compile failed: ${comp_out} ${comp_err}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r176_hover_receipt_testclasses;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R176RetainedHoverReceiptSmoke
  RESULT_VARIABLE tested OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT tested EQUAL 0 OR
   NOT stdout MATCHES "R176_JAVA8_EXACT_WGL_HOVER_COUNTER_FORMAT_PASS" OR
   NOT stdout MATCHES "R176_COUNTER_NEGATIVES_AND_NO_CLICK_AUTHORIZATION_PASS")
  message(FATAL_ERROR "R176 Java8 hover receipt regression failed: ${stdout} ${stderr}")
endif()
message(STATUS "R176_JAVA8_ORIGINAL_WGL_HOVER_RECEIPT_FORMAT_PASS")
