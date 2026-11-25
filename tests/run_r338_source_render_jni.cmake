# R338: genuine Java8 to native JNI world/render callback sequence.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R338 Java/JNI settings")
endif()
set(STAGE "${BUILD_ROOT}/r338-source-world-render-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceWorldReadOnlyHost.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRemotePositionHistory.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceWorldReadOnlyHostSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SourceWorldRenderPhaseSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R338 Java8 compilation failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.SourceWorldRenderPhaseSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R338_JAVA8_REAL_RENDER_SOURCE_JNI_PASS")
  message(FATAL_ERROR "R338 render native smoke: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
