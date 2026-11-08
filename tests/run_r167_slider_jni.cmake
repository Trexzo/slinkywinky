if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R167 disposable R166 JNI DLL must be built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r167_slider_java_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r167_slider_java_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  RESULT_VARIABLE compile_result)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "R167 Java8 source slider compilation failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r167_slider_java_classes"
  dev.slinkywinky.attach.R167RecoveredSliderJniSmoke "${LIBRARY_PATH}"
  RESULT_VARIABLE run_result OUTPUT_VARIABLE result_output ERROR_VARIABLE result_error)
if(NOT run_result EQUAL 0 OR
   NOT result_output MATCHES "R167_JAVA8_JNI_RECOVERED_AUTO_BLOCK_SLIDER_PROFILE_PASS" OR
   NOT result_output MATCHES "R167_ENGINE_RANGE_DENIAL_AND_QUANTIZED_VALUES_PASS")
  message(FATAL_ERROR "R167 Java8 source slider JNI integration failed:\n${result_output}\n${result_error}")
endif()
message(STATUS "${result_output}")
