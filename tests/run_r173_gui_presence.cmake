if(NOT EXISTS "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar")
  message(FATAL_ERROR "R173 original game GUI source observer absent from agent")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r173_gui_fixture_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -cp "${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  -d "${BUILD_ROOT}/r173_gui_fixture_classes"
  "${PROJECT_ROOT}/tests/java/r173_fixture/ave.java"
  "${PROJECT_ROOT}/tests/java/r173_fixture/axu.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/instrument/R173OriginalGuiScreenProbeSmoke.java"
  RESULT_VARIABLE compiled OUTPUT_VARIABLE compiler_stdout ERROR_VARIABLE compiler_stderr)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R173 fake game GUI Java8 fixture failed: ${compiler_stdout} ${compiler_stderr}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r173_gui_fixture_classes;${BUILD_ARTIFACT_DIR}/slinkywinky_instrument.jar"
  dev.slinkywinky.instrument.R173OriginalGuiScreenProbeSmoke
  RESULT_VARIABLE tested OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT tested EQUAL 0 OR
   NOT stdout MATCHES "R173_JAVA8_SOURCE_SHAPED_GUI_OPEN_CLOSE_READ_ONLY_PASS" OR
   NOT stdout MATCHES "R173_GUI_RENDER_OWNER_AND_CLASSLOADER_REFUSAL_PASS")
  message(FATAL_ERROR "R173 original game GUI fixture failed: ${stdout} ${stderr}")
endif()
message(STATUS "R173_JAVA8_ORIGINAL_GUI_READ_ONLY_FIXTURE_PASS")
