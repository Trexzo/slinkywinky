# R349: exact source geometry, Java 8, no Minecraft/JNI.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R349 test settings")
endif()
set(STAGE "${BUILD_ROOT}/r349-backtrack-reach-evidence")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceBacktrackReachEvidence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceBacktrackReachEvidenceSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R349 compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.SourceBacktrackReachEvidenceSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR NOT OUT MATCHES "R349_SOURCE_BACKTRACK_REACH_GEOMETRY_PASS")
  message(FATAL_ERROR "R349 geometry failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
