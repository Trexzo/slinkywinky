if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R178 original Java 8 renderer instrument agent missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r178_hover_schedule_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r178_hover_schedule_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R178RetainedHoverScheduleSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R178 Java8 source hover schedule build failed: ${out} ${err}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r178_hover_schedule_classes;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R178RetainedHoverScheduleSmoke
  RESULT_VARIABLE tested OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT tested EQUAL 0 OR
   NOT stdout MATCHES "R178_JAVA8_60_FRAME_SOURCE_SLIDER_HOVER_WINDOW_PASS" OR
   NOT stdout MATCHES "R178_FOUR_INERT_ACTIONS_AND_FINAL_TAB_SOURCE_STATE_PASS" OR
   NOT stdout MATCHES "R178_PER_FRAME_SNAPSHOT_MUTATION_NEGATIVES_PASS" OR
   NOT stdout MATCHES "R178_NO_MOUSE_MOVEMENT_OR_CLICK_OWNERSHIP")
  message(FATAL_ERROR "R178 native source slider hover schedule failed: ${stdout} ${stderr}")
endif()
message(STATUS "R178_JAVA8_ORIGINAL_RETENTION_HOVER_SCHEDULE_PASS")
