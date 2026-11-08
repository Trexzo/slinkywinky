if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R179 disposable source-slider WGL JNI DLL not built")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r179_native_source_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r179_native_source_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R175RetainedHoverJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R179ExactSourceTargetJniSmoke.java"
  RESULT_VARIABLE compilation OUTPUT_VARIABLE out ERROR_VARIABLE errors)
if(NOT compilation EQUAL 0)
  message(FATAL_ERROR "R179 Java8 JNI source target compile failed: ${out} ${errors}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r179_native_source_classes"
  dev.slinkywinky.attach.R179ExactSourceTargetJniSmoke
  "${LIBRARY_PATH}"
  RESULT_VARIABLE test OUTPUT_VARIABLE out ERROR_VARIABLE errors)
if(NOT test EQUAL 0 OR
   NOT out MATCHES "R179_JAVA8_JNI_EXACT_CXX_SOURCE_TARGET_GEOMETRY_PASS" OR
   NOT out MATCHES "R179_NATIVE_WGL_SOURCE_ELIGIBLE_AND_EXACT_HIT_COUNTERS_PASS" OR
   NOT out MATCHES "R179_NO_ORIGINAL_GAME_CLICK_DISPATCH")
  message(FATAL_ERROR "R179 source geometry JNI/WGL test failed: ${out} ${errors}")
endif()
message(STATUS "R179_ORIGINAL_SOURCE_NUMERIC_NATIVE_GL11_HIT_TARGET_PASS")
