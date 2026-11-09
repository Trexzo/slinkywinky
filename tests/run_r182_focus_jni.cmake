if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R182 disposable focus WGL JNI DLL missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r182_focus_native_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r182_focus_native_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R175RetainedHoverJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R182FocusCycleJniSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R182 Java 8 JNI source compile failed: ${stdout} ${stderr}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r182_focus_native_classes"
  dev.slinkywinky.attach.R182FocusCycleJniSmoke
  "${LIBRARY_PATH}"
  RESULT_VARIABLE status OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT status EQUAL 0 OR
   NOT stdout MATCHES "R182_JAVA8_JNI_NATIVE_WGL_FOCUS_LOSS_REACQUISITION_PASS" OR
   NOT stdout MATCHES "R182_REGAIN_ALONE_NEVER_CERTIFIES_SOURCE_HIT_PASS" OR
   NOT stdout MATCHES "R182_WRONG_OWNER_VIEWPORT_PROFILE_DENIAL_PASS" OR
   NOT stdout MATCHES "R182_SYNTHETIC_FOREGROUND_NOT_HARDWARE_OR_GAME_INPUT")
  message(FATAL_ERROR "R182 native focus-cycle JNI/WGL fixture failed: ${stdout} ${stderr}")
endif()
message(STATUS "R182_DISPOSABLE_NATIVE_FOCUS_CHRONOLOGY_PASS")
