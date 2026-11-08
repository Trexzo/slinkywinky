file(MAKE_DIRECTORY "${BUILD_ROOT}/r158_profile_receipt_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r158_profile_receipt_classes"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/RendererWglEvidence.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/OriginalGuiScreenProbe.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/OriginalPointerArbitrationAudit.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R158ProfileReceiptSmoke.java"
  RESULT_VARIABLE compiled)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R158 original-render Java8 receipt compile failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r158_profile_receipt_classes"
  dev.slinkywinky.attach.R158ProfileReceiptSmoke
  RESULT_VARIABLE executed)
if(NOT executed EQUAL 0)
  message(FATAL_ERROR "R158 Java8 runtime receipt validation failed")
endif()
