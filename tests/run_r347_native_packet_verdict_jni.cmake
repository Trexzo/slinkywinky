# R347: real source JNI e2/e3 per-packet verdicts across client -> Netty.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R347 Java8 JNI inputs")
endif()
set(STAGE "${BUILD_ROOT}/r347-jni-packet-verdict-bridge")
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
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R347 Java8 verdict bridge compile: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.SourcePacketVerdictBridgeSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R347_REAL_NATIVE_PACKET_VERDICT_TO_NETTY_FIFO_PASS")
  message(FATAL_ERROR "R347 real JNI packet verdict handoff: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
