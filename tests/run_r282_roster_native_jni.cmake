# R282: Java8 -> real JNI -> R281 C++ read-only AntiBot event0.
foreach(required PROJECT_ROOT BUILD_ROOT JAVA_COMPILER JAVA_EXECUTABLE LIBRARY_DIR)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "R282 requires ${required}")
  endif()
endforeach()
set(STAGE "${BUILD_ROOT}/r282-roster-native-java")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/RelationshipRosterSampler.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/RelationshipRosterNativePreview.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/RelationshipRosterNativePreviewSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R282 javac failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.RelationshipRosterNativePreviewSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR NOT OUT MATCHES "R282_JAVA8_NATIVE_ANTIBOT_ROSTER_ROUNDTRIP_PASS")
  message(FATAL_ERROR "R282 native roster test failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
