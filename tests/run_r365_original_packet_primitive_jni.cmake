# R365 original 1.8.9 ff/ip packet samples into real read-only native e3.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R365 original packet JNI test inputs")
endif()
set(STAGE "${BUILD_ROOT}/r365-vanilla189-primitive-source-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourcePacketReadOnlyQueue.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189OutboundPacketSampler.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189OutboundPrimitiveQueue.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R365OriginalPacketFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189OutboundPacketSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R365 Java8 packet primitive/JNI compilation: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189OutboundPacketSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R365_ORIGINAL_OUTBOUND_PACKET_PRIMITIVE_NATIVE_E3_READONLY_PASS")
  message(FATAL_ERROR "R365 packet -> native source e3 regression: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
