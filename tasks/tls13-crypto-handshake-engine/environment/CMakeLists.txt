cmake_minimum_required(VERSION 3.22)
project(tls13_engine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/main\\.cpp$")

add_library(tls13_engine STATIC ${SRC_FILES})
target_link_libraries(tls13_engine PUBLIC pthread)

add_executable(tls13_bin src/main.cpp)
target_link_libraries(tls13_bin PRIVATE tls13_engine)

enable_testing()
add_subdirectory(tests)
