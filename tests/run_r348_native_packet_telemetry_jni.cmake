# R348: real source JNI e2/e3/e1 packet proposal counts in a live-shaped host.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R348 Java8/JNI settings")
endif()
set(STAGE "${BUILD_ROOT}/r348-forge-native-packet-telemetry")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRemotePositionHistory.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceWorldReadOnlyHost.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketVerdictBridge.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceWorldReadOnlyHostSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourcePacketVerdictBridgeSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceNativeVerdictTelemetrySmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R348 Java8 telemetry compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.SourceNativeVerdictTelemetrySmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R348_REAL_FORGE_SHAPED_NATIVE_PACKET_PROPOSALS_PASS")
  message(FATAL_ERROR "R348 Forge-shaped native packet telemetry: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
