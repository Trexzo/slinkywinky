if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R172 production Java 8 agent classes not built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r172_receipt_testclasses")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r172_receipt_testclasses"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R172OriginalPointerReceiptSmoke.java"
  RESULT_VARIABLE built OUTPUT_VARIABLE build_output ERROR_VARIABLE build_error)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "R172 read-only Java8 receipt compile failed: ${build_output} ${build_error}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r172_receipt_testclasses;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R172OriginalPointerReceiptSmoke
  RESULT_VARIABLE ran OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT ran EQUAL 0 OR
   NOT output MATCHES "R172_JAVA8_PHYSICAL_POINTER_RECEIPT_NEGATIVE_CONTROLS_PASS" OR
   NOT output MATCHES "R172_NO_GAME_ATTACH_NO_MOUSE_INTERCEPTION")
  message(FATAL_ERROR "R172 read-only pointer receipt regression failed: ${output} ${errors}")
endif()
message(STATUS "R172_JAVA8_READ_ONLY_WINDOW_POINTER_RECEIPT_PASS")
