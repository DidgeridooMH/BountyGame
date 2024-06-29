set(SOURCE
  HashMapTest.cpp
)

add_executable(HashMapTest ${SOURCE})
target_link_libraries(HashMapTest OtterUtil_static gtest_main)
add_test(NAME HashMapTest COMMAND HashMapTest)

set_target_properties(
  HashMapTest
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test
)

