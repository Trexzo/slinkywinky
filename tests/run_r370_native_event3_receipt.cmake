# R370 pure Java8 independent native packet receipt consistency controls.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R370 Java8 inputs")
endif()
set(STAGE "${BUILD_ROOT}/r370-native-event3-receipt-contract")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189LagPacketReceiptVerifier.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189LagPacketReceiptSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R370 Java8 receipt compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189LagPacketReceiptSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R370_ORIGINAL_NATIVE_LAG_E3_RECEIPT_CONTRACT_PASS")
  message(FATAL_ERROR "R370 native packet verdict contract refused: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
