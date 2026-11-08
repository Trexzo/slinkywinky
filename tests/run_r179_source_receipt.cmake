if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R179 exact source target production agent missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r179_source_receipt_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r179_source_receipt_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R179OriginalSourceTargetReceiptSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R179 original Java8 native receipt fixture failed: ${out} ${err}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r179_source_receipt_classes;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R179OriginalSourceTargetReceiptSmoke
  RESULT_VARIABLE test OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT test EQUAL 0 OR
   NOT out MATCHES "R179_JAVA8_EXACT_SOURCE_SLIDER_NATIVE_RECEIPT_PASS" OR
   NOT out MATCHES "R179_MATCHED_R176_FOREGROUND_NUMERIC_NEGATIVES_PASS" OR
   NOT out MATCHES "R179_SOURCE_GEOMETRY_NO_GUI_CLICK_PERMISSION")
  message(FATAL_ERROR "R179 original native Java8 format fixture failed: ${out} ${err}")
endif()
message(STATUS "R179_ORIGINAL_SOURCE_SLIDER_READ_ONLY_RECEIPT_PASS")
