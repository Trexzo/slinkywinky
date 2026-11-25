# R337 Java 8 client-world owner -> real JNI source bridge smoke.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R337 Java/JNI settings")
endif()
set(STAGE "${BUILD_ROOT}/r337-forge-world-source-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceWorldReadOnlyHost.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRemotePositionHistory.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceWorldReadOnlyHostSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R337 Java8 compilation failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.SourceWorldReadOnlyHostSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R337_JAVA8_REAL_WORLD_SOURCE_JNI_PASS")
  message(FATAL_ERROR "R337 live-shaped world JNI test: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
