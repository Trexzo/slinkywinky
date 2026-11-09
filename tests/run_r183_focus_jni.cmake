if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R183 disposable native WGL JNI DLL missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r183_exact_window_focus_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r183_exact_window_focus_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R183ExactWindowFocusJniSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R183 Java8 WGL JNI compilation failed: ${stdout} ${stderr}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r183_exact_window_focus_classes"
  dev.slinkywinky.attach.R183ExactWindowFocusJniSmoke
  "${LIBRARY_PATH}"
  RESULT_VARIABLE status OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT status EQUAL 0 OR
   NOT stdout MATCHES "R183_JAVA8_JNI_REAL_WGL_TRI_STATE_FOCUS_PASS" OR
   NOT stdout MATCHES "R183_JNI_REFUSED_POINTER_NEVER_BACKGROUND_PASS" OR
   NOT stdout MATCHES "R183_JNI_EXACT_SOURCE_HITS_ONLY_AFTER_REGAIN_PASS" OR
   NOT stdout MATCHES "R183_SYNTHETIC_OS_STATUS_NOT_ORIGINAL_GAME_PROOF")
  message(FATAL_ERROR "R183 real native WGL focus tri-state failed: ${stdout} ${stderr}")
endif()
message(STATUS "R183_NATIVE_GL11_TRI_STATE_FOCUS_CHRONOLOGY_PASS")
