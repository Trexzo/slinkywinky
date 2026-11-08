if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R175 disposable first-party WGL JNI not built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r175_hover_java_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r175_hover_java_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R175RetainedHoverJniSmoke.java"
  RESULT_VARIABLE compile_result OUTPUT_VARIABLE build_stdout ERROR_VARIABLE build_stderr)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "R175 Java8 retained passive hover compile failed: ${build_stdout} ${build_stderr}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r175_hover_java_classes"
  dev.slinkywinky.attach.R175RetainedHoverJniSmoke "${LIBRARY_PATH}"
  RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT result EQUAL 0 OR
   NOT stdout MATCHES "R175_JAVA8_JNI_RETAINED_WGL_PASSIVE_NUMERIC_HOVER_PASS" OR
   NOT stdout MATCHES "R175_FOREGROUND_THREAD_VIEWPORT_HELD_GESTURE_DENIAL_PASS" OR
   NOT stdout MATCHES "R175_PASSIVE_HOVER_NO_PROFILE_MUTATION_OR_CLICK_DELIVERY")
  message(FATAL_ERROR "R175 disposable native WGL hover failure: ${stdout} ${stderr}")
endif()
message(STATUS "R175_JAVA8_JNI_NATIVE_PASSIVE_TOOLTIP_HOVER_PASS")
