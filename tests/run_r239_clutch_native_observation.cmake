# R239: Java8 -> actual C++ native Clutch source planner, no game.
foreach(required PROJECT_ROOT BUILD_ROOT JAVA_COMPILER JAVA_EXECUTABLE LIBRARY_DIR)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "R239 requires ${required}")
  endif()
endforeach()
set(stage "${BUILD_ROOT}/r239_clutch_native_observation")
file(MAKE_DIRECTORY "${stage}")
set(sampler "${PROJECT_ROOT}/java/dev/slinkywinky/ClutchWorldSampler.java")
set(wrapper "${PROJECT_ROOT}/java/dev/slinkywinky/ClutchNativePreview.java")
set(smoke "${PROJECT_ROOT}/tests/java/dev/slinkywinky/ClutchNativePreviewSmoke.java")
foreach(source "${sampler}" "${wrapper}" "${smoke}")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "R239 missing source ${source}")
  endif()
endforeach()
execute_process(
  COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -d "${stage}"
    "${sampler}" "${wrapper}" "${smoke}"
  RESULT_VARIABLE compile_status
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(NOT compile_status EQUAL 0)
  message(FATAL_ERROR "R239 Java8 compile failed: ${compiler_out}\n${compiler_err}")
endif()
execute_process(
  COMMAND "${JAVA_EXECUTABLE}" "-Djava.library.path=${LIBRARY_DIR}"
    -cp "${stage}" dev.slinkywinky.ClutchNativePreviewSmoke
  RESULT_VARIABLE exec_status
  OUTPUT_VARIABLE runtime_out
  ERROR_VARIABLE runtime_err
)
if(NOT exec_status EQUAL 0 OR
   NOT runtime_out MATCHES "R239_JAVA8_NATIVE_CLUTCH_SOURCE_PLANNER_PASS" OR
   NOT runtime_out MATCHES "R244_JAVA8_NATIVE_CLUTCH_PRIOR_RAY_PASS")
  message(FATAL_ERROR "R239 native Clutch JNI smoke failed (${exec_status}): ${runtime_out}\n${runtime_err}")
endif()
message(STATUS "${runtime_out}")
