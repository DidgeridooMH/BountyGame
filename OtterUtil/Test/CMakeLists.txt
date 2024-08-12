set(SOURCE
  BitMapTest.cpp
  HashMapTest.cpp
  SparseAutoArrayTest.cpp
)

add_executable(UtilTest ${SOURCE})
target_link_libraries(UtilTest
  OtterUtil
  gtest_main
)
add_test(NAME UtilTest COMMAND UtilTest)

set_target_properties(
  UtilTest
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_SOURCE_DIR}/bin/test/${CMAKE_BUILD_TYPE}
)

