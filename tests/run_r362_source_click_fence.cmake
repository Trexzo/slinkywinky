# R362 exact original 1.8.9 click return, source clock one-shot.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R362 Java8 inputs")
endif()
set(STAGE "${BUILD_ROOT}/r362-source-click-fence")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189CombatClock.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/SourceClickAttackFence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R358OriginalSourceFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/SourceClickAttackFenceSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R362 Java8 compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.attach.SourceClickAttackFenceSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R362_ORIGINAL_CLICK_RETURN_HIT_FENCE_ONESHOT_PASS")
  message(FATAL_ERROR "R362 click-return acceptance failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
