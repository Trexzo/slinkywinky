# R276 Java8 read-only owner bound roster regression (no Minecraft)
if(NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE OR NOT PROJECT_ROOT OR NOT BUILD_ROOT)
  message(FATAL_ERROR "R276 toolchain missing")
endif()
set(CLASSES "${BUILD_ROOT}/r276-roster-classes")
file(MAKE_DIRECTORY "${CLASSES}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${CLASSES}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/RelationshipRosterSampler.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/RelationshipRosterSamplerSmoke.java"
  RESULT_VARIABLE COMPILE_RESULT OUTPUT_VARIABLE COMPILE_OUT ERROR_VARIABLE COMPILE_ERR)
if(NOT COMPILE_RESULT EQUAL 0)
  message(FATAL_ERROR "R276 javac failed: ${COMPILE_OUT} ${COMPILE_ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${CLASSES}"
  dev.slinkywinky.RelationshipRosterSamplerSmoke
  RESULT_VARIABLE TEST_RESULT OUTPUT_VARIABLE TEST_OUT ERROR_VARIABLE TEST_ERR)
if(NOT TEST_RESULT EQUAL 0 OR NOT TEST_OUT MATCHES "R276_JAVA8_ROSTER_OWNER_RECEIPT_PASS")
  message(FATAL_ERROR "R276 roster failed: ${TEST_OUT} ${TEST_ERR}")
endif()
message(STATUS "${TEST_OUT}")
