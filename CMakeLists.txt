cmake_minimum_required(VERSION 3.22)

project(otus_home_works)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(UNITTEST "on/off unittest" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(TEST_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

if(UNITTEST)
    enable_testing()
endif()

add_subdirectory(libs)
add_subdirectory(main)

