if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar" OR
   NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R164 offline Java8 audit JARs absent; build loader first")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.attach.R164OfflineInputCutSmoke
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR
   NOT output MATCHES "R164_TEST_ONLY_GATE_VANILLA_MODAL_VANILLA_EXECUTION_PASS")
  message(FATAL_ERROR "R164 offline input method gate failed: ${output} ${errors}")
endif()
message(STATUS "R164_JAVA8_OFFLINE_METHOD_CUT_TEST_PASS")
