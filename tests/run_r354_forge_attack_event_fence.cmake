# R354: client-owner and world-scoped Forge event handoff, pure Java8.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R354 Java8 test inputs")
endif()
set(STAGE "${BUILD_ROOT}/r354-source-attack-event-fence")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceAttackEventFence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceAttackEventFenceSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R354 Java8 compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.SourceAttackEventFenceSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R354_FORGE_ATTACK_EVENT_FENCE_JAVA8_PASS")
  message(FATAL_ERROR "R354 bounded attack observation failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
