# R301: Java8 real JNI -> R297/R298/R299 source controllers.
foreach(key PROJECT_ROOT BUILD_ROOT JAVA_COMPILER JAVA_EXECUTABLE LIBRARY_DIR)
  if(NOT DEFINED ${key})
    message(FATAL_ERROR "R301 requires ${key}")
  endif()
endforeach()
set(STAGE "${BUILD_ROOT}/r301-esp-java")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/EspWorldSnapshotSampler.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/EspNativeReadOnlyPreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/PlayerEspScreenProjection.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/EspNativeReadOnlyPreviewSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R301 javac: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.EspNativeReadOnlyPreviewSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R301_JAVA8_REAL_NATIVE_THREE_ESP_CONTROLLERS_PASS" OR
   NOT OUT MATCHES "R306_JAVA8_REAL_NATIVE_EVENT7_SCREEN_PROJECTION_PASS")
  message(FATAL_ERROR "R301 JNI three ESP test: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
