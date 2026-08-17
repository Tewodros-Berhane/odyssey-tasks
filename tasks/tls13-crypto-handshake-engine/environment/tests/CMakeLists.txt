cmake_minimum_required(VERSION 3.22)

add_executable(unit_tests test_basic.cpp)
target_link_libraries(unit_tests PRIVATE tls13_engine)

add_test(NAME BasicTLS COMMAND unit_tests)
