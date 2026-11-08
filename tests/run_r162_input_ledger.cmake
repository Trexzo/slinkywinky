if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar" OR
   NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R162 Java 8 audit artifacts missing: build loader first")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.attach.R162InputOrderLedgerSmoke
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR
   NOT output MATCHES "R162_SAME_COUNT_REORDERED_SOURCE_CALLSITE_REFUSED_PASS")
  message(FATAL_ERROR "R162 source order fixture failed: ${output}\n${errors}")
endif()
message(STATUS "R162_SOURCE_ONLY_JAVA8_SEQUENCE_FIXTURE_PASS")
