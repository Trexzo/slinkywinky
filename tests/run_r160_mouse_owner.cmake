file(MAKE_DIRECTORY "${BUILD_ROOT}/r160_mouse_owner_classes")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8
  -d "${BUILD_ROOT}/r160_mouse_owner_classes"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/MouseQueueOwner.java"
  "${PROJECT_ROOT}/tests/java/org/lwjgl/input/Mouse.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/MouseQueueOwnerSmoke.java"
  RESULT_VARIABLE compiled)
if(NOT compiled EQUAL 0)
  message(FATAL_ERROR "R160 Java 8 LWJGL2 mock mouse owner compile failed")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  -cp "${BUILD_ROOT}/r160_mouse_owner_classes"
  dev.slinkywinky.attach.MouseQueueOwnerSmoke
  RESULT_VARIABLE executed)
if(NOT executed EQUAL 0)
  message(FATAL_ERROR "R160 isolated mouse input restoration test failed")
endif()
