# Invoked by Wasm/CMakeLists.txt to compile one model component with wasm32-wasip2-clang.
cmake_minimum_required(VERSION 3.20)

if(NOT WASI_CLANG OR NOT MODEL_NAME OR NOT WASM_OUTPUT OR NOT FMU_WASM_C OR NOT BINDGEN_C OR NOT BINDGEN_OBJ)
    message(FATAL_ERROR "compile_wasm.cmake: missing required -D variables")
endif()

if(NOT REFERENCE_FMU_ROOT OR NOT BINDGEN_DIR)
    message(FATAL_ERROR "compile_wasm.cmake: missing REFERENCE_FMU_ROOT or BINDGEN_DIR")
endif()

set(_sources
    "${FMU_WASM_C}"
    "${BINDGEN_C}"
    "${REFERENCE_FMU_ROOT}/${MODEL_NAME}/model.c"
    "${REFERENCE_FMU_ROOT}/src/common.c"
    "${REFERENCE_FMU_ROOT}/src/cosimulation.c"
    "${BINDGEN_OBJ}"
)

execute_process(
    COMMAND "${WASI_CLANG}"
        -mexec-model=reactor
        -DFMI_VERSION=3
        -DDISABLE_PREFIX
        -Wno-error
        "-I${BINDGEN_DIR}"
        "-I${REFERENCE_FMU_ROOT}/include"
        "-I${REFERENCE_FMU_ROOT}/${MODEL_NAME}"
        ${_sources}
        -o "${WASM_OUTPUT}"
    RESULT_VARIABLE _result
    ERROR_VARIABLE _stderr
)

if(_result OR NOT EXISTS "${WASM_OUTPUT}")
    message(FATAL_ERROR "Failed to build ${MODEL_NAME}.wasm: ${_stderr}")
endif()
