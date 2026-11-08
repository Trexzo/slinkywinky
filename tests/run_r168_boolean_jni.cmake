if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R168 isolated retained UI JNI fixture was not built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r168_boolean_java_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r168_boolean_java_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R168RecoveredBooleanJniSmoke.java"
  RESULT_VARIABLE compile_result)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "R168 Java8 Boolean source-page compilation failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r168_boolean_java_classes"
  dev.slinkywinky.attach.R168RecoveredBooleanJniSmoke "${LIBRARY_PATH}"
  RESULT_VARIABLE test_result OUTPUT_VARIABLE result_output ERROR_VARIABLE result_error)
if(NOT test_result EQUAL 0 OR
   NOT result_output MATCHES "R168_JAVA8_JNI_PAGE3_RECOVERED_BOOLEAN_NATIVE_PROFILE_PASS" OR
   NOT result_output MATCHES "R168_TWO_WAY_SOURCE_PAGE_BOUNDS_AND_STALE_CLICK_DENIAL_PASS")
  message(FATAL_ERROR "R168 isolated JNI Boolean source-page acceptance failed:\n${result_output}\n${result_error}")
endif()
message(STATUS "${result_output}")
