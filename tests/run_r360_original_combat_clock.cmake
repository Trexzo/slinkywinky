# R360 source-derived original game tick clock, bounded one-shot phase proxy.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R360 Java8 test inputs")
endif()
set(STAGE "${BUILD_ROOT}/r360-vanilla189-combat-clock")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189CombatClock.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/SourceAttackTickFence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R358OriginalSourceFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189CombatClockSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R360 Java8 compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189CombatClockSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R360_ORIGINAL_VANILLA_PLAYER_CLOCK_AND_ONESHOT_E15_PROXY_PASS")
  message(FATAL_ERROR "R360 source tick/attack admission failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
