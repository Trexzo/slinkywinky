file(MAKE_DIRECTORY "${BUILD_ROOT}/java_classes")
execute_process(COMMAND "${JAVA_COMPILER}" --release 8 -d "${BUILD_ROOT}/java_classes"
  "${PROJECT_ROOT}/java/dev/slinkywinky/NativeEngine.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/NativeSmoke.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/GameHost.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/HostSettings.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/HostSmoke.java"
  RESULT_VARIABLE compile_result)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "JNI Java smoke compilation failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" "-Djava.library.path=${BUILD_ROOT}"
  -cp "${BUILD_ROOT}/java_classes" dev.slinkywinky.NativeSmoke
  RESULT_VARIABLE java_result)
if(NOT java_result EQUAL 0)
  message(FATAL_ERROR "JNI roundtrip failed")
endif()

execute_process(COMMAND "${JAVA_EXECUTABLE}" "-Djava.library.path=${BUILD_ROOT}"
  -cp "${BUILD_ROOT}/java_classes" dev.slinkywinky.HostSmoke
  RESULT_VARIABLE host_result)
if(NOT host_result EQUAL 0)
  message(FATAL_ERROR "JNI host lifecycle failed")
endif()
