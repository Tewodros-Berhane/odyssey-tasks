cmake_minimum_required(VERSION 3.22)
project(storage_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread -march=native")
include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_library(storage_lib STATIC ${SRC_FILES})

enable_testing()
add_subdirectory(tests)
