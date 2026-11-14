find_package(GTest CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)

add_executable(vefr_tests unit/graphs.cpp)
target_link_libraries(vefr_tests PRIVATE vefr::vefr GTest::gtest_main)
add_test(NAME vefr_tests COMMAND vefr_tests)
set_tests_properties(vefr_tests PROPERTIES FAIL_REGULAR_EXPRESSION "Running 0 tests")
