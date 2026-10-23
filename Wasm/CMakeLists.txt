cmake_minimum_required(VERSION 3.17)

message(STATUS "Configure the repository root with -DFMI_TARGET=wasm (see Wasm/README.md).")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR
        "Use: cmake -S .. -B build-wasm -DFMI_TARGET=wasm -DFMI_VERSION=3")
endif()
