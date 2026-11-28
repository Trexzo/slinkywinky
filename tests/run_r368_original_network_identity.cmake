# R368 strict original NetHandler pointer, weak identity, no original game.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE)
  message(FATAL_ERROR "Missing R368 Java8 prerequisites")
endif()
set(STAGE "${BUILD_ROOT}/r368-vanilla-network-owner")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189ActiveNetworkBinding.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R365OriginalPacketFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189ActiveNetworkBindingSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R368 Java8 active network owner compile: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189ActiveNetworkBindingSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R368_ORIGINAL_NETWORK_MANAGER_WEAK_IDENTITY_GATE_PASS")
  message(FATAL_ERROR "R368 original weak owner identity test: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
