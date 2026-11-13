if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar" OR
   NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R195 source-only audit JARs not built")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.attach.R195PossibleInputDispatchSmoke
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR
   NOT output MATCHES "R195_CLASS_OVERRIDE_POSSIBLE_INPUT_EDGE_PASS" OR
   NOT output MATCHES "R195_INTERFACE_POSSIBLE_INPUT_EDGE_PASS" OR
   NOT output MATCHES "R195_SOURCE_ORDER_AND_MUTATION_NEGATIVE_CONTROLS_PASS" OR
   NOT output MATCHES "R195_EXTERNAL_CLASS_HIERARCHY_GAP_IS_UNRESOLVED")
  message(FATAL_ERROR "R195 Java 8 possible input dispatch failed: ${output} ${errors}")
endif()
message(STATUS "R195_JAVA8_POSSIBLE_VIRTUAL_INTERFACE_DISPATCH_PASS")
