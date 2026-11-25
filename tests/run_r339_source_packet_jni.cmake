# R339 Java8 primitive packet FIFO, source owner, and actual JNI callbacks.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "R339 required Java/JNI settings missing")
endif()
set(STAGE "${BUILD_ROOT}/r339-source-packet-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceWorldReadOnlyHost.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceWorldReadOnlyHostSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourcePacketReadOnlyQueueSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourcePacketNativeReplaySmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R339 Java8 compile failed: ${OUT} ${ERR}")
endif()
foreach(test SourcePacketReadOnlyQueueSmoke SourcePacketNativeReplaySmoke)
  execute_process(COMMAND "${JAVA_EXECUTABLE}"
    "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
    "dev.slinkywinky.${test}"
    RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
  if(NOT RC EQUAL 0 OR NOT OUT MATCHES "R339_.*_PASS")
    message(FATAL_ERROR "R339 real JNI packet test ${test}: ${OUT} ${ERR}")
  endif()
  message(STATUS "${OUT}")
endforeach()
