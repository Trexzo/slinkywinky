if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R177 disposable JNI/WGL test library not found")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r177_tooltip_pixel_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r177_tooltip_pixel_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R175RetainedHoverJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R177RetainedTooltipPixelsSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE compile_out ERROR_VARIABLE compile_err)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R177 Java 8 source tooltip pixel fixture compile: ${compile_out} ${compile_err}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r177_tooltip_pixel_classes"
  dev.slinkywinky.attach.R177RetainedTooltipPixelsSmoke
  "${LIBRARY_PATH}"
  RESULT_VARIABLE tested OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT tested EQUAL 0 OR
   NOT stdout MATCHES "R177_JAVA8_JNI_WGL_ACTUAL_SOURCE_TOOLTIP_PIXEL_DELTA_PASS" OR
   NOT stdout MATCHES "R177_TOOLTIP_FOCUS_RECOVERY_PIXEL_RESTORATION_PASS" OR
   NOT stdout MATCHES "R177_TOOLTIP_NO_CLICKS_NO_PROFILE_MUTATION_PASS" OR
   NOT stdout MATCHES "R177_TEST_ONLY_WGL_NOT_ORIGINAL_MINECRAFT_FOREGROUND")
  message(FATAL_ERROR "R177 source numeric tooltip did not produce real GL11 pixel evidence: ${stdout} ${stderr}")
endif()
message(STATUS "R177_SOURCE_NUMERIC_TOOLTIP_GL11_PIXELS_PROVED_IN_DISPOSABLE_WGL")
