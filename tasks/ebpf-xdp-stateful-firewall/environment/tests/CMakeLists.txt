cmake_minimum_required(VERSION 3.22)

add_executable(unit_tests test_basic.cpp)
target_link_libraries(unit_tests PRIVATE firewall_engine)

add_test(NAME BasicFirewall COMMAND unit_tests)
