# R285 pure Java8 TEMP gate regression; no Minecraft or native game.
foreach(required PROJECT_ROOT BUILD_ROOT JAVA_COMPILER JAVA_EXECUTABLE)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "R285 requires ${required}")
  endif()
endforeach()
set(stage "${BUILD_ROOT}/r285_roster_gate")
file(MAKE_DIRECTORY "${stage}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${stage}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/RosterIsolationGate.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/RosterIsolationGateSmoke.java"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "R285 javac failed: ${out} ${err}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${stage}"
  dev.slinkywinky.RosterIsolationGateSmoke
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "R285_JAVA8_INDEPENDENT_ROSTER_TEMP_GATE_PASS")
  message(FATAL_ERROR "R285 gate refused: ${out} ${err}")
endif()
message(STATUS "${out}")
