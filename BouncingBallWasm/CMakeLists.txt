cmake_minimum_required(VERSION 3.20)

project(BouncingBallWasm C)

find_program(WIT_BINDGEN_EXECUTABLE wit-bindgen REQUIRED)
find_program(WASM_TOOLS_EXECUTABLE wasm-tools REQUIRED)

if(DEFINED ENV{WASI_SDK_PATH})
    set(WASI_SDK_PREFIX "$ENV{WASI_SDK_PATH}")
elseif(NOT WASI_SDK_PREFIX)
    message(FATAL_ERROR "Set WASI_SDK_PATH to your wasi-sdk installation (e.g. E:/wasi-sdk-34.0-x86_64-windows)")
endif()

find_program(WASI_CLANG
    NAMES wasm32-wasip2-clang wasm32-wasip2-clang.exe
    HINTS "${WASI_SDK_PREFIX}/bin"
    REQUIRED
    DOC "WASI SDK clang for wasm32-wasip2"
)

file(GLOB_RECURSE WIT_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wit/*.wit")

set(BINDGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bindgen")
set(BINDGEN_H "${BINDGEN_DIR}/co_simulation_fmu.h")
set(BINDGEN_C "${BINDGEN_DIR}/co_simulation_fmu.c")
set(BINDGEN_OBJ "${BINDGEN_DIR}/co_simulation_fmu_component_type.o")
set(WASM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bouncing_ball.wasm")

set(REFERENCE_FMU_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_custom_command(
    OUTPUT "${BINDGEN_H}" "${BINDGEN_C}" "${BINDGEN_OBJ}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BINDGEN_DIR}"
    COMMAND ${WIT_BINDGEN_EXECUTABLE} c
            --world co-simulation-fmu
            "${CMAKE_CURRENT_SOURCE_DIR}/wit"
            --out-dir "${BINDGEN_DIR}"
    DEPENDS ${WIT_FILES}
    COMMENT "Generating WIT C bindings for co-simulation-fmu"
    VERBATIM
)

set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bouncing_ball_wasm.c"
    "${BINDGEN_C}"
    "${REFERENCE_FMU_ROOT}/BouncingBall/model.c"
    "${REFERENCE_FMU_ROOT}/src/fmi3Functions.c"
    "${REFERENCE_FMU_ROOT}/src/cosimulation.c"
)

add_custom_command(
    OUTPUT "${WASM_OUTPUT}"
    COMMAND ${WASI_CLANG}
            -mexec-model=reactor
            -DFMI_VERSION=3
            -DDISABLE_PREFIX
            -Wno-error
            -I"${BINDGEN_DIR}"
            -I"${REFERENCE_FMU_ROOT}/include"
            -I"${REFERENCE_FMU_ROOT}/BouncingBall"
            ${SOURCES}
            "${BINDGEN_OBJ}"
            -o "${WASM_OUTPUT}"
    DEPENDS
        "${BINDGEN_H}"
        "${BINDGEN_C}"
        "${BINDGEN_OBJ}"
        "${CMAKE_CURRENT_SOURCE_DIR}/bouncing_ball_wasm.c"
        "${REFERENCE_FMU_ROOT}/BouncingBall/model.c"
        "${REFERENCE_FMU_ROOT}/src/fmi3Functions.c"
        "${REFERENCE_FMU_ROOT}/src/cosimulation.c"
    COMMENT "Building BouncingBall Wasm component"
)

add_custom_target(bouncing_ball_wasm DEPENDS "${WASM_OUTPUT}")

add_custom_target(validate-wit
    COMMAND ${WASM_TOOLS_EXECUTABLE} component wit "${CMAKE_CURRENT_SOURCE_DIR}/wit"
    COMMENT "Validating WIT package"
    VERBATIM
)

add_custom_target(BouncingBallWasm ALL DEPENDS bouncing_ball_wasm validate-wit)
