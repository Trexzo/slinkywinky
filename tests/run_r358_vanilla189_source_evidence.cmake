# R358 Java8 exact-name vanilla original source evidence, no game process.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R358 Java8 inputs")
endif()
set(STAGE "${BUILD_ROOT}/r358-vanilla189-source-evidence")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189SourceEvidence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R358OriginalSourceFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189SourceEvidenceSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R358 Java8 sample compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189SourceEvidenceSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R358_VANILLA189_ORIGINAL_SOURCE_FIELDS_READONLY_PASS")
  message(FATAL_ERROR "R358 source field sample: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
