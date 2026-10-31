cmake_minimum_required(VERSION 3.20)
project(slinkywinky VERSION 0.1.0 LANGUAGES CXX)

add_library(slinkywinky_core STATIC src/core.cpp src/runtime.cpp src/combat.cpp src/settings.cpp src/session.cpp)
find_package(Threads REQUIRED)
target_link_libraries(slinkywinky_core PUBLIC Threads::Threads)
target_include_directories(slinkywinky_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(slinkywinky_core PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(slinkywinky_core PRIVATE /W4)
else()
  target_compile_options(slinkywinky_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(slinkywinky_bridge SHARED src/bridge.cpp)
target_compile_definitions(slinkywinky_bridge PRIVATE SLINKYWINKY_BRIDGE_BUILD)
target_link_libraries(slinkywinky_bridge PRIVATE slinkywinky_core)
target_include_directories(slinkywinky_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(slinkywinky_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_executable(slinkywinky_offline_demo examples/offline_demo.cpp)
target_link_libraries(slinkywinky_offline_demo PRIVATE slinkywinky_core)
include(CTest)
if(BUILD_TESTING)
  add_executable(slinkywinky_bridge_tests tests/test_bridge.cpp)
  target_link_libraries(slinkywinky_bridge_tests PRIVATE slinkywinky_bridge)
  add_test(NAME bridge_abi COMMAND slinkywinky_bridge_tests)
  add_executable(slinkywinky_core_tests tests/test_core.cpp)
  target_link_libraries(slinkywinky_core_tests PRIVATE slinkywinky_core)
  add_test(NAME core_state_models COMMAND slinkywinky_core_tests)
  foreach(unit runtime combat settings session stress)
    add_executable(slinkywinky_${unit}_tests tests/test_${unit}.cpp)
    target_link_libraries(slinkywinky_${unit}_tests PRIVATE slinkywinky_core)
    add_test(NAME ${unit}_models COMMAND slinkywinky_${unit}_tests)
  endforeach()
endif()

option(SLINKYWINKY_BUILD_JNI "Build optional JVM integration smoke bridge" OFF)
if(SLINKYWINKY_BUILD_JNI)
  find_package(JNI REQUIRED)
  add_library(slinkywinky_jni SHARED src/jni_bridge.cpp src/bridge.cpp)
  target_compile_definitions(slinkywinky_jni PRIVATE SLINKYWINKY_BRIDGE_BUILD)
  target_link_libraries(slinkywinky_jni PRIVATE slinkywinky_core)
  target_include_directories(slinkywinky_jni PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${JNI_INCLUDE_DIRS})
  if(BUILD_TESTING)
    find_package(Java COMPONENTS Runtime Development REQUIRED)
    add_test(NAME jni_java_roundtrip COMMAND ${CMAKE_COMMAND}
      -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}
      -DJAVA_COMPILER=${Java_JAVAC_EXECUTABLE}
      -DJAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}
      -DLIBRARY_DIR=$<TARGET_FILE_DIR:slinkywinky_jni>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_java_smoke.cmake)
  endif()
endif()

# Injectable Windows architecture: official Java 8 Attach API -> JVMTI native agent.
# Forge is optional test code only and is NOT a runtime dependency.
option(SLINKYWINKY_BUILD_ATTACH "Build the standalone JVM attach loader and agent" OFF)
if(SLINKYWINKY_BUILD_ATTACH)
  if(NOT WIN32)
    message(FATAL_ERROR "The standalone loader currently targets Windows x64")
  endif()
  find_package(JNI REQUIRED)
  find_package(Java REQUIRED COMPONENTS Runtime Development)
  get_filename_component(SW_JDK_BIN "${Java_JAVA_EXECUTABLE}" DIRECTORY)
  get_filename_component(SW_JDK_HOME "${SW_JDK_BIN}" DIRECTORY)
  set(SW_TOOLS_JAR "${SW_JDK_HOME}/lib/tools.jar")
  if(NOT EXISTS "${SW_TOOLS_JAR}")
    message(FATAL_ERROR "The standalone loader requires a full Java 8 JDK containing lib/tools.jar")
  endif()
  add_library(slinkywinky_agent SHARED src/agent_entry.cpp src/bridge.cpp)
  target_compile_features(slinkywinky_agent PRIVATE cxx_std_17)
  target_compile_definitions(slinkywinky_agent PRIVATE SLINKYWINKY_BRIDGE_BUILD)
  target_include_directories(slinkywinky_agent PRIVATE ${JNI_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(slinkywinky_agent PRIVATE slinkywinky_core)
  set_target_properties(slinkywinky_agent PROPERTIES OUTPUT_NAME "slinkywinky_agent")

  add_executable(slinkywinky_loader src/windows_loader.cpp)
  target_compile_features(slinkywinky_loader PRIVATE cxx_std_17)
  add_dependencies(slinkywinky_loader slinkywinky_agent)
  add_custom_command(TARGET slinkywinky_loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8 -cp "${SW_TOOLS_JAR}"
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/attach/AttachMain.java"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/attach-test-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/java/dev/slinkywinky/attach/AttachTarget.java"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes"
    COMMAND "${Java_JAVAC_EXECUTABLE}" -source 8 -target 8
      -d "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/java/dev/slinkywinky/instrument/InstrumentationProbe.java"
    COMMAND "${Java_JAR_EXECUTABLE}" cfm
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_instrument.jar"
      "${CMAKE_CURRENT_SOURCE_DIR}/loader/instrument-agent.mf"
      -C "$<TARGET_FILE_DIR:slinkywinky_loader>/instrument-classes" .
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:slinkywinky_agent>"
      "$<TARGET_FILE_DIR:slinkywinky_loader>/slinkywinky_agent.dll"
    VERBATIM)
  if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
      add_test(NAME native_agent_attach
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/attach_smoke.py"
          "$<TARGET_FILE:slinkywinky_loader>" "${SW_JDK_HOME}")
      set_tests_properties(native_agent_attach PROPERTIES TIMEOUT 45)
    endif()
  endif()
endif()
