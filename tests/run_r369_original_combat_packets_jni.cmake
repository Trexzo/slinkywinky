# R369 original C02 and C0B action ordinal 4 -> native C++ e3; no packet IO.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R369 Java8 JNI inputs")
endif()
set(STAGE "${BUILD_ROOT}/r369-original-combat-packets-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189OutboundPacketSampler.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189OutboundPrimitiveQueue.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R365OriginalPacketFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189CombatPacketNativeSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R369 Java8 combat packet JNI compile failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189CombatPacketNativeSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R369_REAL_ORIGINAL_COMBAT_CATEGORIES_NATIVE_E3_JNI_PASS")
  message(FATAL_ERROR "R369 native e3 classification test failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
