if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R166 disposable JNI DLL not built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r166_pointer_java_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r166_pointer_java_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  RESULT_VARIABLE compile_result)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "R166 Java8 input-transaction test compilation failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r166_pointer_java_classes"
  dev.slinkywinky.attach.R166RetainedPointerJniSmoke "${LIBRARY_PATH}"
  RESULT_VARIABLE run_result OUTPUT_VARIABLE result_output ERROR_VARIABLE result_error)
if(NOT run_result EQUAL 0 OR
   NOT result_output MATCHES "R166_JAVA8_JNI_NATIVE_RETAINED_POINTER_PROFILE_PASS" OR
   NOT result_output MATCHES "R166_DISPOSABLE_WGL_NO_MINECRAFT_HOOK_PASS")
  message(FATAL_ERROR "R166 Java8 -> native WGL input test failed:\n${result_output}\n${result_error}")
endif()
message(STATUS "${result_output}")
