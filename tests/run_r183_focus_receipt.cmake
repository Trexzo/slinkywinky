if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R183 exact-window agent Java8 JAR missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r183_exact_window_receipt_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r183_exact_window_receipt_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R183ExactWindowFocusReceiptSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R183 Java8 source receipt compilation failed: ${stdout} ${stderr}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r183_exact_window_receipt_classes;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R183ExactWindowFocusReceiptSmoke
  RESULT_VARIABLE status OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT status EQUAL 0 OR
   NOT stdout MATCHES "R183_JAVA8_EXACT_OS_WINDOW_RECEIPT_NEGATIVES_PASS" OR
   NOT stdout MATCHES "R183_WIN32_REFUSAL_NOT_BACKGROUND_PASS" OR
   NOT stdout MATCHES "R183_CLICK_INPUT_CAPTURE_REMAINS_DISABLED")
  message(FATAL_ERROR "R183 Java8 original exact OS focus receipt rejected: ${stdout} ${stderr}")
endif()
message(STATUS "R183_ORIGINAL_OS_FOCUS_MARKER_JAVA8_PASS")
