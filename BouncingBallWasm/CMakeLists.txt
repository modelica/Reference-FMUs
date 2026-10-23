cmake_minimum_required(VERSION 3.20)

message(STATUS "BouncingBallWasm: use the root project with -DBUILD_WASM=ON (see Wasm/README.md).")

project(BouncingBallWasmStub C)

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    return()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../Wasm/CMakeLists.txt")
