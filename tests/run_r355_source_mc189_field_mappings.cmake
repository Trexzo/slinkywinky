# R355: candidate JNI field names by MCP/SRG/obf 1.8.9, pure Java8.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R355 Java8 test inputs")
endif()
set(STAGE "${BUILD_ROOT}/r355-source-mc189-private-fields")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceCandidateField189.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceCandidateField189Smoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R355 Java8 mapping compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.SourceCandidateField189Smoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R355_SOURCE_MINECRAFT_189_PRIVATE_FIELD_PARITY_PASS")
  message(FATAL_ERROR "R355 private field mapping regression failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
