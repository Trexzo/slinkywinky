# R358 real Java8/JNI e18 roundtrip from original-name strict field sample.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R358 native JNI settings")
endif()
set(STAGE "${BUILD_ROOT}/r358-vanilla-source-native-lag-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189SourceEvidence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R358OriginalSourceFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189SourceNativeLagSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R358 Java8 native source compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189SourceNativeLagSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R358_REAL_ORIGINAL_SOURCE_VALUES_TO_NATIVE_LAG_E18_PASS")
  message(FATAL_ERROR "R358 actual native source e18 failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
