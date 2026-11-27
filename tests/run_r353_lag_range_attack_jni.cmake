# R353 JNI event15 e18 e3 handshake under a real Java8 native owner.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R353 Java8/JNI settings")
endif()
set(STAGE "${BUILD_ROOT}/r353-lag-range-attack-phase")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRemotePositionHistory.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceWorldReadOnlyHost.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceLagRangeAttackPhaseSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R353 Java8 compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.SourceLagRangeAttackPhaseSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R353_NATIVE_LAG_RANGE_EVENT15_EVENT18_JNI_PASS")
  message(FATAL_ERROR "R353 native attack phase chain failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
