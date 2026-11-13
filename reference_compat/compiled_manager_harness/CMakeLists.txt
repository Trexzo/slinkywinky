cmake_minimum_required(VERSION 3.20)
project(slinkywinky_compiled_manager_acceptance LANGUAGES CXX)

# This first-party harness links only the separately built public OpenSlinky
# module_manager.cpp COFF object. It never links or loads the full DLL.
if(NOT WIN32 OR NOT MSVC)
  message(FATAL_ERROR "R195 compiled-manager harness requires the Windows x64 MSVC ABI")
endif()
# Public CI must compile this first-party harness WITHOUT requiring an
# unlicensed external COFF object. It does not link or run anything.
option(SLINKY_MANAGER_SOURCE_ONLY "Compile first-party harness source only" OFF)
if(SLINKY_MANAGER_SOURCE_ONLY)
  add_library(slinkywinky_r195_harness_source OBJECT manager_acceptance.cpp)
  set_property(TARGET slinkywinky_r195_harness_source PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded")
  target_compile_features(slinkywinky_r195_harness_source PRIVATE cxx_std_17)
  target_compile_options(slinkywinky_r195_harness_source PRIVATE /W4 /WX /EHsc)
  return()
endif()
if(NOT DEFINED SLINKY_MANAGER_OBJECT OR NOT EXISTS "${SLINKY_MANAGER_OBJECT}")
  message(FATAL_ERROR "Set -DSLINKY_MANAGER_OBJECT=<absolute path to compiled module_manager.cpp.obj>")
endif()

add_executable(slinkywinky_r195_manager_acceptance
  manager_acceptance.cpp
  "${SLINKY_MANAGER_OBJECT}"
)
set_source_files_properties("${SLINKY_MANAGER_OBJECT}" PROPERTIES
  EXTERNAL_OBJECT TRUE GENERATED TRUE
)
# The pinned public clang-cl COFF object declares /DEFAULTLIB:libcmt.lib.
# Match that static CRT for the harness in both configs, avoiding LNK4098.
set_property(TARGET slinkywinky_r195_manager_acceptance PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_compile_features(slinkywinky_r195_manager_acceptance PRIVATE cxx_std_17)
target_compile_options(slinkywinky_r195_manager_acceptance PRIVATE /W4 /WX /EHsc)
target_link_options(slinkywinky_r195_manager_acceptance PRIVATE /INCREMENTAL:NO)
enable_testing()
add_test(NAME r195_actual_compiled_manager_synthetic_runtime
  COMMAND slinkywinky_r195_manager_acceptance)
set_tests_properties(r195_actual_compiled_manager_synthetic_runtime PROPERTIES TIMEOUT 10)
# Optional local-only R196 acceptance. Fixture is produced from a linked,
# verified DLL by the independent metadata extractor; CI has no third-party DLL.
if(DEFINED SLINKY_DESCRIPTOR_FIXTURE)
  if(NOT EXISTS "${SLINKY_DESCRIPTOR_FIXTURE}")
    message(FATAL_ERROR "R196 linked descriptor fixture path does not exist")
  endif()
  add_test(NAME r196_compiled_manager_real_linked_metadata
    COMMAND slinkywinky_r195_manager_acceptance "${SLINKY_DESCRIPTOR_FIXTURE}")
  set_tests_properties(r196_compiled_manager_real_linked_metadata PROPERTIES TIMEOUT 10)
endif()
# Private R197 fixture contains original linked Windows native addresses.
# Generated locally, never checked into the public reference CI.
if(DEFINED SLINKY_NATIVE_RECORD_FIXTURE)
  if(NOT DEFINED SLINKY_DESCRIPTOR_FIXTURE
      OR NOT EXISTS "${SLINKY_DESCRIPTOR_FIXTURE}"
      OR NOT EXISTS "${SLINKY_NATIVE_RECORD_FIXTURE}")
    message(FATAL_ERROR "R197 requires both local record and sanitized manifest files")
  endif()
  add_test(NAME r197_compiled_manager_private_native_record_layout
    COMMAND slinkywinky_r195_manager_acceptance
      --native-records "${SLINKY_NATIVE_RECORD_FIXTURE}" "${SLINKY_DESCRIPTOR_FIXTURE}")
  set_tests_properties(r197_compiled_manager_private_native_record_layout
    PROPERTIES TIMEOUT 10)
endif()
