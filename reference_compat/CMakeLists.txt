cmake_minimum_required(VERSION 3.20)
project(slinkywinky_reference_compat VERSION 0.1 LANGUAGES CXX)

# Entirely independent test target. Existing SlinkyWinky runtime CMake
# remains untouched while the parallel R-series branch is active.
add_library(slinkywinky_reference_event_model INTERFACE)
target_compile_features(slinkywinky_reference_event_model INTERFACE cxx_std_17)
target_include_directories(slinkywinky_reference_event_model INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(slinkywinky_reference_event_model INTERFACE /W4 /WX)
else()
  target_compile_options(slinkywinky_reference_event_model INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()
include(CTest)
if(BUILD_TESTING)
  add_executable(slinkywinky_reference_event_replay_test tests/test_event_replay.cpp)
  target_link_libraries(slinkywinky_reference_event_replay_test PRIVATE slinkywinky_reference_event_model)
  add_test(NAME r174_native_event_replay COMMAND slinkywinky_reference_event_replay_test)
endif()
