# R362 exact original click-return clock through real JNI source Lag Range.
if(NOT PROJECT_ROOT OR NOT BUILD_ROOT OR NOT JAVA_COMPILER OR
   NOT JAVA_EXECUTABLE OR NOT LIBRARY_DIR)
  message(FATAL_ERROR "Missing R362 source JNI inputs")
endif()
set(STAGE "${BUILD_ROOT}/r362-click-exit-native-lag-jni")
file(MAKE_DIRECTORY "${STAGE}")
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -encoding UTF-8
  -d "${STAGE}"
  "${PROJECT_ROOT}/java/dev/slinkywinky/SourceRuntimePreview.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/Vanilla189CombatClock.java"
  "${PROJECT_ROOT}/loader/java/dev/slinkywinky/instrument/SourceClickAttackFence.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/R358OriginalSourceFixture.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/attach/Vanilla189ClickNativeLagSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R362 Java8 JNI original click compilation failed: ${OUT} ${ERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}"
  "-Djava.library.path=${LIBRARY_DIR}" -cp "${STAGE}"
  dev.slinkywinky.attach.Vanilla189ClickNativeLagSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR)
if(NOT RC EQUAL 0 OR
   NOT OUT MATCHES "R362_REAL_ORIGINAL_CLICK_E15_THEN_NATIVE_E18_JNI_PASS")
  message(FATAL_ERROR "R362 actual native source e15/e18 order failed: ${OUT} ${ERR}")
endif()
message(STATUS "${OUT}")
