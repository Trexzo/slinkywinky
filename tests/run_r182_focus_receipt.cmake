if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R182 original Java 8 focus receipt agent missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r182_focus_receipt_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r182_focus_receipt_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R182OriginalFocusCycleReceiptSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R182 Java 8 receipt compile failed: ${stdout} ${stderr}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r182_focus_receipt_classes;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R182OriginalFocusCycleReceiptSmoke
  RESULT_VARIABLE status OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT status EQUAL 0 OR
   NOT stdout MATCHES "R182_JAVA8_ORIGINAL_FOCUS_CYCLE_RECEIPT_NEGATIVES_PASS" OR
   NOT stdout MATCHES "R182_NATIVE_COUNTER_SHAPE_NO_HARDWARE_CLAIM_PASS" OR
   NOT stdout MATCHES "R182_SOURCE_FOCUS_OBSERVATION_NEVER_ENABLES_CLICKS")
  message(FATAL_ERROR "R182 Java 8 original focus receipt failed: ${stdout} ${stderr}")
endif()
message(STATUS "R182_ORIGINAL_JVM_FOCUS_CYCLE_RECEIPT_PASS")
