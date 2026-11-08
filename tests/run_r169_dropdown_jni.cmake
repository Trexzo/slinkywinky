if(NOT EXISTS "${LIBRARY_PATH}")
  message(FATAL_ERROR "R169 disposable JNI test library missing")
endif()
file(MAKE_DIRECTORY "${BUILD_ROOT}/r169_dropdown_java_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r169_dropdown_java_classes"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R166RetainedPointerJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R167RecoveredSliderJniSmoke.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R169DropdownJniSmoke.java"
  RESULT_VARIABLE compile_result)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "R169 Java8 popup integration compilation failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r169_dropdown_java_classes"
  dev.slinkywinky.attach.R169DropdownJniSmoke "${LIBRARY_PATH}"
  RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT result EQUAL 0 OR
   NOT out MATCHES "R169_JAVA8_JNI_SOURCE_DROPDOWN_POINTER_SELECTION_PASS" OR
   NOT out MATCHES "R169_POPUP_MODAL_CLICK_THROUGH_AND_CANCEL_DENIAL_PASS")
  message(FATAL_ERROR "R169 Java8 native popup regression failed:\n${out}\n${err}")
endif()
message(STATUS "${out}")
