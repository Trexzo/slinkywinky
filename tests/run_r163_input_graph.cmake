if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar" OR
   NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R163 audit artifacts absent; build Java 8 loader first")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.attach.R163InputControlGraphSmoke
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR
   NOT output MATCHES "R163_SYNTHETIC_BRANCH_OPCODE_NEGATIVE_CONTROL_PASS")
  message(FATAL_ERROR "R163 Java 8 CFG fixture failed: ${output} ${errors}")
endif()
message(STATUS "R163_OFFLINE_CFG_NEGATIVE_CONTROL_PASS")
