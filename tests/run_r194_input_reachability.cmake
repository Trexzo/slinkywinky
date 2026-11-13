if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar" OR
   NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R194 offline source audit JARs not built")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.attach.R194OriginalInputCallReachabilitySmoke
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR
   NOT output MATCHES "R194_TWO_HOP_ORIGINAL_INPUT_NEGATIVE_CONTROL_PASS" OR
   NOT output MATCHES "R194_VIRTUAL_DISPATCH_UNRESOLVED_NEGATIVE_CONTROL_PASS" OR
   NOT output MATCHES "R194_UNRESOLVED_EXTERNAL_CALLEE_NO_FALSE_EDGE_PASS" OR
   NOT output MATCHES "R194_INDEPENDENT_CLASS_ORDER_FINGERPRINT_PASS")
  message(FATAL_ERROR "R194 Java 8 static call graph fixture failed: ${output} ${errors}")
endif()
message(STATUS "R194_JAVA8_STATIC_REVERSE_CALL_GRAPH_FIXTURE_PASS")
