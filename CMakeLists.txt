cmake_minimum_required(VERSION 3.20)
project(cartesian_genetic_algo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CGA_BUILD_TESTS "Build unit tests" ON)
option(CGA_BUILD_EXAMPLES "Build examples" ON)

add_library(cga INTERFACE)
target_include_directories(cga INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CGA_BUILD_TESTS)
  enable_testing()
  add_executable(cga_smoke_tests tests/smoke_tests.cpp)
  target_link_libraries(cga_smoke_tests PRIVATE cga)
  add_test(NAME cga_smoke_tests COMMAND cga_smoke_tests)
  add_executable(cga_determinism_tests tests/determinism_tests.cpp)
  target_link_libraries(cga_determinism_tests PRIVATE cga)
  add_test(NAME cga_determinism_tests COMMAND cga_determinism_tests)
endif()

if(CGA_BUILD_EXAMPLES)
  add_executable(cga_xor examples/xor.cpp)
  target_link_libraries(cga_xor PRIVATE cga)
  add_executable(cga_xor_checkpoint examples/xor_checkpoint.cpp)
  target_link_libraries(cga_xor_checkpoint PRIVATE cga)
endif()
