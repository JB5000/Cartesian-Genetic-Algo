cmake_minimum_required(VERSION 3.20)
project(cgp_mcu_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

add_library(cgp_mcu INTERFACE)
target_include_directories(cgp_mcu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cgp_mcu INTERFACE Threads::Threads)

add_executable(cgp_mcu_demo src/demo.cpp)
target_link_libraries(cgp_mcu_demo PRIVATE cgp_mcu)
add_executable(cgp_mcu_benchmark src/benchmark.cpp)
target_link_libraries(cgp_mcu_benchmark PRIVATE cgp_mcu)

enable_testing()
add_executable(cgp_mcu_tests tests/tests.cpp)
target_link_libraries(cgp_mcu_tests PRIVATE cgp_mcu)
add_test(NAME cgp_mcu_tests COMMAND cgp_mcu_tests)
