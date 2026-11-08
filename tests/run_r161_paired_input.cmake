file(MAKE_DIRECTORY "${BUILD_ROOT}/r161_paired_input_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r161_paired_input_classes"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/MouseQueueOwner.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java"
  "${PROJECT_ROOT}/tests/java/org/lwjgl/input/Mouse.java"
  "${PROJECT_ROOT}/tests/java/org/lwjgl/input/Keyboard.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R161InputCoverageLedger.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R161PairedInputRollbackSmoke.java"
  RESULT_VARIABLE compiled)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R161 Java 8 paired-input mock compilation failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r161_paired_input_classes"
  dev.slinkywinky.attach.R161PairedInputRollbackSmoke
  RESULT_VARIABLE executed)
if(NOT executed EQUAL 0)
  message(FATAL_ERROR "R161 paired-input recovery test failed")
endif()
