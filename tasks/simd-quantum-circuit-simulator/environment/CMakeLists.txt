cmake_minimum_required(VERSION 3.22)
project(quantum_simulator CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread -march=native -fopenmp")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -mavx2 -mfma")

find_package(OpenMP REQUIRED)

include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/main\\.cpp$")

add_library(quantum_engine STATIC ${SRC_FILES})
target_link_libraries(quantum_engine PUBLIC pthread OpenMP::OpenMP_CXX)

add_executable(quantum_bin src/main.cpp)
target_link_libraries(quantum_bin PRIVATE quantum_engine)

enable_testing()
add_subdirectory(tests)
