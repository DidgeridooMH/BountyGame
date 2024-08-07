set(SOURCE
  EntityComponentMapTest.cpp
  SystemRegistryTest.cpp
)

add_executable(ECSTest ${SOURCE})
target_link_libraries(ECSTest OtterECS_static gtest_main)
add_test(NAME ECSTest COMMAND ECSTest)

set_target_properties(
  ECSTest
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test
)

