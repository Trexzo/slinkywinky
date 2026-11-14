# R204: Java 8 -> first-party native Sprint observation roundtrip.
# Compiles only project-owned sources; no Minecraft or injected process.
foreach(required PROJECT_ROOT BUILD_ROOT JAVA_COMPILER JAVA_EXECUTABLE LIBRARY_DIR)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "R204 requires ${required}")
  endif()
endforeach()
set(stage "${BUILD_ROOT}/r204_java_sprint_observation")
file(MAKE_DIRECTORY "${stage}")
set(wrapper "${PROJECT_ROOT}/java/dev/slinkywinky/SprintObservationProbe.java")
set(smoke "${PROJECT_ROOT}/tests/java/dev/slinkywinky/SprintObservationProbeSmoke.java")
# R212 adds the game-agnostic action owner and its mock-backed Java8
# regression to the same reproducible Java/native smoke environment.
set(action_owner "${PROJECT_ROOT}/java/dev/slinkywinky/OwnedModuleActions.java")
set(action_smoke "${PROJECT_ROOT}/tests/java/dev/slinkywinky/OwnedModuleActionsSmoke.java")
foreach(source "${wrapper}" "${smoke}" "${action_owner}" "${action_smoke}")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "R204 missing Java source: ${source}")
  endif()
endforeach()
execute_process(
  COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -d "${stage}"
          "${wrapper}" "${smoke}" "${action_owner}" "${action_smoke}"
  RESULT_VARIABLE javac_result
  OUTPUT_VARIABLE javac_stdout
  ERROR_VARIABLE javac_stderr
)
if(NOT javac_result EQUAL 0)
  message(FATAL_ERROR "R204 Java8 compilation failed: ${javac_stdout}\n${javac_stderr}")
endif()
execute_process(
  COMMAND "${JAVA_EXECUTABLE}" "-Djava.library.path=${LIBRARY_DIR}" -cp "${stage}"
          dev.slinkywinky.SprintObservationProbeSmoke
  RESULT_VARIABLE java_result
  OUTPUT_VARIABLE java_stdout
  ERROR_VARIABLE java_stderr
)
if(NOT java_result EQUAL 0 OR
   NOT java_stdout MATCHES "R204_JAVA8_NATIVE_SPRINT_OBSERVATION_ROUNDTRIP_PASS")
  message(FATAL_ERROR "R204 read-only JNI test failed (${java_result}): ${java_stdout}\n${java_stderr}")
endif()
message(STATUS "${java_stdout}")

# R212 test is intentionally game-agnostic: its Port writes a fake
# movement/jump object, verifying ownership and conditional cleanup
# without launching Minecraft or touching user files.
execute_process(
  COMMAND "${JAVA_EXECUTABLE}" -cp "${stage}"
          dev.slinkywinky.OwnedModuleActionsSmoke
  RESULT_VARIABLE action_result
  OUTPUT_VARIABLE action_stdout
  ERROR_VARIABLE action_stderr
)
if(NOT action_result EQUAL 0 OR
   NOT action_stdout MATCHES "R212_OWNED_MINECRAFT_ACTIONS_PASS")
  message(FATAL_ERROR "R212 Java8 action ownership test failed (${action_result}): ${action_stdout}\n${action_stderr}")
endif()
message(STATUS "${action_stdout}")
