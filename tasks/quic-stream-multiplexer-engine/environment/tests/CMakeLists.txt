cmake_minimum_required(VERSION 3.22)

add_executable(unit_tests test_basic.cpp)
target_link_libraries(unit_tests PRIVATE quic_engine)

add_test(NAME BasicQuic COMMAND unit_tests)
