if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar" OR
   NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R171 audit source JAR not built")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_render_audit.jar;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.attach.R171VanillaJarInputSurfaceSmoke
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR
   NOT output MATCHES "R171_EXTERNAL_RENDER_CONSUMER_NEGATIVE_CONTROL_PASS" OR
   NOT output MATCHES "R171_CROSS_CLASS_SCAN_ORDER_AND_UNCHANGED_AVE_PASS")
  message(FATAL_ERROR "R171 global vanilla input source fixture failed: ${output} ${errors}")
endif()
message(STATUS "R171_JAVA8_OUTSIDE_METHOD_INPUT_SURFACE_FIXTURE_PASS")
