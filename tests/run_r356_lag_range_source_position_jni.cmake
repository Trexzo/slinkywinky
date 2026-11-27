# R356 actual Java8/JNI event18->event3 source target coordinate parity.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R356 JNI inputs")
endif()
set(STAGE "${BUILD_ROOT}/r356-lag-target-position")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceCandidateField189.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceCandidatePosition189.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceCandidatePosition189Smoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R356 Java8 compilation failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.SourceCandidatePosition189Smoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R356_NATIVE_LAG_TARGET_POSITION_SOURCE_PARITY_PASS")
  message(FATAL_ERROR "R356 actual native JNI target parity failure: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
