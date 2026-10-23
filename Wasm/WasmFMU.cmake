# Included from the root CMakeLists.txt when FMI_TARGET=wasm.

find_program(WIT_BINDGEN_EXECUTABLE wit-bindgen REQUIRED)
find_program(WASM_TOOLS_EXECUTABLE wasm-tools REQUIRED)

if(DEFINED ENV{WASI_SDK_PATH})
    set(WASI_SDK_PREFIX "$ENV{WASI_SDK_PATH}")
elseif(NOT WASI_SDK_PREFIX)
    message(FATAL_ERROR "FMI_TARGET=wasm requires WASI_SDK_PATH (wasi-sdk installation).")
endif()

find_program(WASI_CLANG
    NAMES wasm32-wasip2-clang wasm32-wasip2-clang.exe
    HINTS "${WASI_SDK_PREFIX}/bin"
    REQUIRED
    DOC "WASI SDK clang for wasm32-wasip2"
)

set(REFERENCE_FMU_WASM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(WASM_FMU_WIT_DIR "${REFERENCE_FMU_WASM_ROOT}/Wasm/wit")
set(WASM_BINDGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/wasm_bindgen")
set(WASM_FMU_WASM_C "${REFERENCE_FMU_WASM_ROOT}/Wasm/fmu_wasm.c")
set(WASM_COMPILE_SCRIPT "${REFERENCE_FMU_WASM_ROOT}/Wasm/compile_wasm.cmake")

file(GLOB_RECURSE WASM_WIT_FILES CONFIGURE_DEPENDS "${WASM_FMU_WIT_DIR}/*.wit")

set(WASM_BINDGEN_H "${WASM_BINDGEN_DIR}/co_simulation_fmu.h")
set(WASM_BINDGEN_C "${WASM_BINDGEN_DIR}/co_simulation_fmu.c")
set(WASM_BINDGEN_OBJ "${WASM_BINDGEN_DIR}/co_simulation_fmu_component_type.o")

add_custom_command(
    OUTPUT "${WASM_BINDGEN_H}" "${WASM_BINDGEN_C}" "${WASM_BINDGEN_OBJ}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${WASM_BINDGEN_DIR}"
    COMMAND ${WIT_BINDGEN_EXECUTABLE} c
            --world co-simulation-fmu
            "${WASM_FMU_WIT_DIR}"
            --out-dir "${WASM_BINDGEN_DIR}"
    DEPENDS ${WASM_WIT_FILES}
    COMMENT "Generating WIT C bindings for co-simulation-fmu"
    VERBATIM
)

add_custom_target(reference_fmu_wasm_bindgen
    DEPENDS "${WASM_BINDGEN_H}" "${WASM_BINDGEN_C}" "${WASM_BINDGEN_OBJ}"
)

add_custom_target(reference_fmu_validate_wit
    COMMAND ${WASM_TOOLS_EXECUTABLE} component wit "${WASM_FMU_WIT_DIR}"
    COMMENT "Validating WIT package"
    VERBATIM
)

function(reference_fmu_add_wasm_component MODEL_NAME)

    set(FMU_BUILD_DIR "temp/${MODEL_NAME}")
    set(WASM_OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}/${MODEL_NAME}.wasm")

    set(_model_sources
        "${REFERENCE_FMU_WASM_ROOT}/${MODEL_NAME}/model.c"
        "${REFERENCE_FMU_WASM_ROOT}/src/common.c"
        "${REFERENCE_FMU_WASM_ROOT}/src/cosimulation.c"
    )

    add_custom_command(
        OUTPUT "${WASM_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
        COMMAND ${CMAKE_COMMAND}
            -DWASI_CLANG=${WASI_CLANG}
            -DMODEL_NAME=${MODEL_NAME}
            -DWASM_OUTPUT=${WASM_OUTPUT}
            -DFMU_WASM_C=${WASM_FMU_WASM_C}
            -DBINDGEN_C=${WASM_BINDGEN_C}
            -DBINDGEN_OBJ=${WASM_BINDGEN_OBJ}
            -DBINDGEN_DIR=${WASM_BINDGEN_DIR}
            -DREFERENCE_FMU_ROOT=${REFERENCE_FMU_WASM_ROOT}
            -P ${WASM_COMPILE_SCRIPT}
        DEPENDS
            "${WASM_BINDGEN_H}"
            "${WASM_BINDGEN_C}"
            "${WASM_BINDGEN_OBJ}"
            "${WASM_FMU_WASM_C}"
            "${WASM_COMPILE_SCRIPT}"
            ${_model_sources}
        COMMENT "Building Wasm component ${MODEL_NAME}"
        VERBATIM
    )

    add_custom_target(${MODEL_NAME} ALL DEPENDS "${WASM_OUTPUT}")
    add_dependencies(${MODEL_NAME} reference_fmu_wasm_bindgen reference_fmu_validate_wit)

endfunction()
