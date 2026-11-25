# R342: exact vanilla serverPos and primitive packet confirmation + real JNI.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R342 Java8 JNI configuration")
endif()
set(STAGE "${BUILD_ROOT}/r342-native-historical-serverpos")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRemotePositionHistory.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceWorldReadOnlyHost.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceWorldReadOnlyHostSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceRemotePositionHistorySmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceHistoricalNativeRenderSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R342 Java8 compile failure: ${OUT} ${ERR}")
endif()
foreach(test SourceRemotePositionHistorySmoke SourceHistoricalNativeRenderSmoke)
  execute_process(COMMAND "${JAVA_EXECUTABLE}"
    "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
    "dev.slinkywinky.${test}"
    RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
  if(NOT RC EQUAL 0 OR NOT OUT MATCHES "R342_.*_PASS")
    message(FATAL_ERROR "R342 history + native e6 ${test}: ${OUT} ${ERR}")
  endif()
  message(STATUS "${OUT}")
endforeach()
