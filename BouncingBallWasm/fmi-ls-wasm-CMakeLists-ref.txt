cmake_minimum_required(VERSION 3.20)
project(fmi3-wasm C)

# ---------------------------------------------------------------------------
# Find Cargo (Rust build tool) – required for the Rust FMU examples
# ---------------------------------------------------------------------------
find_program(CARGO_EXECUTABLE cargo
    HINTS ENV CARGO_HOME
    PATH_SUFFIXES bin
    DOC "Path to the cargo build tool"
)

if(NOT CARGO_EXECUTABLE)
    message(WARNING
        "cargo not found – Rust FMU example targets will be skipped.\n"
        "Install Rust from https://rustup.rs/ to enable them."
    )
endif()

# ---------------------------------------------------------------------------
# Find wasm-tools
# ---------------------------------------------------------------------------
find_program(WASM_TOOLS_EXECUTABLE wasm-tools
    HINTS ENV WASM_TOOLS_PATH
    DOC "Path to the wasm-tools binary"
)

if(NOT WASM_TOOLS_EXECUTABLE)
    message(FATAL_ERROR
        "wasm-tools not found. Install it with:\n"
        "  cargo install wasm-tools\n"
        "or set WASM_TOOLS_PATH to the directory containing wasm-tools."
    )
endif()

message(STATUS "Found wasm-tools: ${WASM_TOOLS_EXECUTABLE}")

# ---------------------------------------------------------------------------
# Find wit-bindgen – required for the C FMU examples
# ---------------------------------------------------------------------------
find_program(WIT_BINDGEN_EXECUTABLE wit-bindgen
    DOC "Path to the wit-bindgen CLI"
)

if(NOT WIT_BINDGEN_EXECUTABLE)
    message(WARNING
        "wit-bindgen not found – C FMU example targets will be skipped.\n"
        "Install it with: cargo install wit-bindgen-cli"
    )
endif()

message(STATUS "Found wit-bindgen: ${WIT_BINDGEN_EXECUTABLE}")

# ---------------------------------------------------------------------------
# Find curl – required for downloading dependencies for the C runner example
# ---------------------------------------------------------------------------
find_program(CURL_EXECUTABLE curl
    DOC "Path to curl executable"
)

if(NOT CURL_EXECUTABLE)
    message(WARNING
        "curl not found – adder-fmu-c-runner will be skipped.\n"
        "Install curl to enable it."
    )
endif()

# ---------------------------------------------------------------------------
# Collect all .wit files under wit/
# ---------------------------------------------------------------------------
file(GLOB_RECURSE WIT_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/wit/*.wit")

if(NOT WIT_FILES)
    message(FATAL_ERROR "No .wit files found under ${CMAKE_SOURCE_DIR}/wit/")
endif()

message(STATUS "WIT files to validate: ${WIT_FILES}")

# ---------------------------------------------------------------------------
# Custom target: validate-wit
#
# wasm-tools component wit <dir> parses and resolves the entire WIT package
# in the given directory, reporting any syntax or resolution errors.
# ---------------------------------------------------------------------------
add_custom_target(validate-wit ALL
    COMMAND ${WASM_TOOLS_EXECUTABLE} component wit "${CMAKE_SOURCE_DIR}/wit"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Validating WIT files with wasm-tools..."
    VERBATIM
)

# ---------------------------------------------------------------------------
# Also expose a standalone 'wit-check' target that is not part of ALL,
# so developers can run it explicitly:  cmake --build . --target wit-check
# ---------------------------------------------------------------------------
add_custom_target(wit-check
    COMMAND ${WASM_TOOLS_EXECUTABLE} component wit "${CMAKE_SOURCE_DIR}/wit"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Checking WIT files with wasm-tools..."
    VERBATIM
)

# ---------------------------------------------------------------------------
# CTest integration – running ctest will also validate the WIT files
# ---------------------------------------------------------------------------
enable_testing()

add_test(
    NAME wit-validation
    COMMAND ${WASM_TOOLS_EXECUTABLE} component wit "${CMAKE_SOURCE_DIR}/wit"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# ---------------------------------------------------------------------------
# Example FMU: adder-rust-fmu
#
# A Co-Simulation FMU with two Float64 inputs and one Float64 output
# (sum = input_a + input_b), compiled to a Wasm component.
#
# Build:   cmake --build <build-dir> --target adder-rust-fmu
# Output:  examples/adder-rust-fmu/target/wasm32-wasip2/release/adder_rust_fmu.wasm
# ---------------------------------------------------------------------------
if(CARGO_EXECUTABLE)
    set(ADDER_RUST_FMU_DIR  "${CMAKE_SOURCE_DIR}/examples/adder-rust-fmu")
    set(ADDER_RUST_FMU_WASM
        "${ADDER_RUST_FMU_DIR}/target/wasm32-wasip2/release/adder_rust_fmu.wasm"
    )

    add_custom_command(
        OUTPUT  "${ADDER_RUST_FMU_WASM}"
        COMMAND ${CARGO_EXECUTABLE} build --release
        WORKING_DIRECTORY "${ADDER_RUST_FMU_DIR}"
        COMMENT "Building adder-rust-fmu Wasm component..."
        VERBATIM
        # Rebuild when any source or WIT file changes.
        DEPENDS
            "${ADDER_RUST_FMU_DIR}/Cargo.toml"
            "${ADDER_RUST_FMU_DIR}/src/lib.rs"
            ${WIT_FILES}
    )

    add_custom_target(adder-rust-fmu
        DEPENDS "${ADDER_RUST_FMU_WASM}"
    )

    # Validate the compiled component against the WIT package.
    add_custom_target(validate-adder-rust-fmu
        COMMAND ${WASM_TOOLS_EXECUTABLE} validate --features component-model
                "${ADDER_RUST_FMU_WASM}"
        DEPENDS adder-rust-fmu
        COMMENT "Validating adder-rust-fmu Wasm component..."
        VERBATIM
    )

    add_test(
        NAME adder-rust-fmu-wasm-validation
        COMMAND ${WASM_TOOLS_EXECUTABLE} validate --features component-model
                "${ADDER_RUST_FMU_WASM}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(adder-rust-fmu-wasm-validation
        PROPERTIES DEPENDS wit-validation
    )

    # -----------------------------------------------------------------------
    # Package adder-rust-fmu as a proper FMU archive (.fmu = zip)
    #
    # Layout inside the archive:
    #   modelDescription.xml
    #   binaries/wasm32-wasip2/adder_rust_fmu.wasm
    # -----------------------------------------------------------------------
    set(ADDER_RUST_FMU_ARCHIVE  "${CMAKE_BINARY_DIR}/adder-rust-fmu.fmu")
    set(ADDER_RUST_FMU_STAGING  "${CMAKE_BINARY_DIR}/adder-rust-fmu-staging")
    set(ADDER_RUST_FMU_MODEL_DESC "${ADDER_RUST_FMU_DIR}/modelDescription.xml")

    add_custom_command(
        OUTPUT "${ADDER_RUST_FMU_ARCHIVE}"
        # (Re-)create a clean staging tree
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADDER_RUST_FMU_STAGING}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${ADDER_RUST_FMU_STAGING}/binaries/wasm32-wasip2"
        # Populate the staging tree
        COMMAND ${CMAKE_COMMAND} -E copy
                "${ADDER_RUST_FMU_MODEL_DESC}"
                "${ADDER_RUST_FMU_STAGING}/modelDescription.xml"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${ADDER_RUST_FMU_WASM}"
                "${ADDER_RUST_FMU_STAGING}/binaries/wasm32-wasip2/adder_rust_fmu.wasm"
        # Zip the staging tree into <build>/adder-rust-fmu.fmu
        # cmake -E chdir changes directory before running the nested command,
        # so relative paths in the archive match the desired layout.
        COMMAND ${CMAKE_COMMAND} -E chdir "${ADDER_RUST_FMU_STAGING}"
                ${CMAKE_COMMAND} -E tar cf "${ADDER_RUST_FMU_ARCHIVE}" --format=zip
                -- modelDescription.xml binaries
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        DEPENDS
            adder-rust-fmu
            "${ADDER_RUST_FMU_MODEL_DESC}"
        COMMENT "Packaging adder-rust-fmu FMU archive (adder-rust-fmu.fmu)..."
        VERBATIM
    )

    add_custom_target(adder-rust-fmu-archive
        DEPENDS "${ADDER_RUST_FMU_ARCHIVE}"
    )

    # -----------------------------------------------------------------------
    # Example host runner: adder-fmu-rust-runner
    #
    # Runs the adder FMU with wasmtime from t = 0.0 to 5.0 in 0.1 s steps,
    # checking that sum = input_a + input_b at each communication point.
    # -----------------------------------------------------------------------
    set(ADDER_FMU_RUST_RUNNER_DIR  "${CMAKE_SOURCE_DIR}/examples/adder-fmu-rust-runner")
    set(ADDER_FMU_RUST_RUNNER_BUILD_DIR "${CMAKE_BINARY_DIR}/adder-fmu-rust-runner-build")
    set(ADDER_FMU_RUST_RUNNER_NATIVE_BIN "${ADDER_FMU_RUST_RUNNER_DIR}/target/debug/adder-fmu-rust-runner${CMAKE_EXECUTABLE_SUFFIX}")
    set(ADDER_FMU_RUST_RUNNER_BIN "${ADDER_FMU_RUST_RUNNER_BUILD_DIR}/adder-fmu-rust-runner${CMAKE_EXECUTABLE_SUFFIX}")

    add_custom_command(
        OUTPUT "${ADDER_FMU_RUST_RUNNER_BIN}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDER_FMU_RUST_RUNNER_BUILD_DIR}"
        COMMAND ${CARGO_EXECUTABLE} build --manifest-path "${ADDER_FMU_RUST_RUNNER_DIR}/Cargo.toml"
        COMMAND ${CMAKE_COMMAND} -E copy "${ADDER_FMU_RUST_RUNNER_NATIVE_BIN}" "${ADDER_FMU_RUST_RUNNER_BIN}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS
            "${ADDER_FMU_RUST_RUNNER_DIR}/Cargo.toml"
            "${ADDER_FMU_RUST_RUNNER_DIR}/src/main.rs"
            ${WIT_FILES}
        COMMENT "Building adder-fmu-rust-runner standalone executable..."
        VERBATIM
    )

    add_custom_target(adder-fmu-rust-runner-build
        DEPENDS "${ADDER_FMU_RUST_RUNNER_BIN}"
    )

    add_custom_target(adder-fmu-rust-runner
        COMMAND "${ADDER_FMU_RUST_RUNNER_BIN}" "${ADDER_RUST_FMU_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS
            adder-fmu-rust-runner-build
            adder-rust-fmu-archive
        COMMENT "Running adder-fmu-rust-runner against adder-rust-fmu.fmu..."
        VERBATIM
    )

    add_test(
        NAME adder-fmu-rust-runner
        COMMAND "${ADDER_FMU_RUST_RUNNER_BIN}" "${ADDER_RUST_FMU_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(adder-fmu-rust-runner
        PROPERTIES DEPENDS adder-rust-fmu-wasm-validation
    )

    # -----------------------------------------------------------------------
    # Example host runner: adder-fmu-c-runner
    #
    # Native C host implementation using Wasmtime's C embedding API.
    # Download a pinned Wasmtime prebuilt C API bundle and compile the runner
    # with the host C compiler.
    # -----------------------------------------------------------------------
    if(CURL_EXECUTABLE)
        set(ADDER_FMU_C_RUNNER_DIR "${CMAKE_SOURCE_DIR}/examples/adder-fmu-c-runner")
        set(ADDER_FMU_C_RUNNER_BUILD_DIR "${CMAKE_BINARY_DIR}/adder-fmu-c-runner-build")
        set(ADDER_FMU_C_RUNNER_BIN "${ADDER_FMU_C_RUNNER_BUILD_DIR}/adder-fmu-c-runner")
        set(WASMTIME_VERSION "v45.0.1")
        set(WASMTIME_C_API_ARCHIVE_NAME "wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api.tar.xz")
        set(WASMTIME_C_API_URL "https://github.com/bytecodealliance/wasmtime/releases/download/${WASMTIME_VERSION}/${WASMTIME_C_API_ARCHIVE_NAME}")
        set(WASMTIME_C_API_ARCHIVE "${CMAKE_BINARY_DIR}/third_party/${WASMTIME_C_API_ARCHIVE_NAME}")
        set(WASMTIME_C_API_DIR "${CMAKE_BINARY_DIR}/third_party/wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api")

        add_custom_command(
            OUTPUT "${ADDER_FMU_C_RUNNER_BIN}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDER_FMU_C_RUNNER_BUILD_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/third_party"
            COMMAND /bin/sh -c "if [ ! -f '${WASMTIME_C_API_ARCHIVE}' ]; then '${CURL_EXECUTABLE}' --fail --location --output '${WASMTIME_C_API_ARCHIVE}' '${WASMTIME_C_API_URL}'; fi"
            COMMAND /bin/sh -c "if [ ! -d '${WASMTIME_C_API_DIR}' ]; then cd '${CMAKE_BINARY_DIR}/third_party' && tar -xf '${WASMTIME_C_API_ARCHIVE}'; fi"
            COMMAND cc -std=c11 -O2 -Wall -Wextra
                    -I${WASMTIME_C_API_DIR}/include
                    "${ADDER_FMU_C_RUNNER_DIR}/main.c"
                    -L${WASMTIME_C_API_DIR}/lib
                    -Wl,-rpath,${WASMTIME_C_API_DIR}/lib
                    -lwasmtime -lpthread -ldl -lm
                    -o "${ADDER_FMU_C_RUNNER_BIN}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            DEPENDS
                "${ADDER_FMU_C_RUNNER_DIR}/main.c"
                adder-rust-fmu-archive
            COMMENT "Building adder-fmu-c-runner (C host with Wasmtime C API)..."
            VERBATIM
        )

        add_custom_target(adder-fmu-c-runner-build
            DEPENDS "${ADDER_FMU_C_RUNNER_BIN}"
        )

        add_custom_target(adder-fmu-c-runner
            COMMAND "${ADDER_FMU_C_RUNNER_BIN}" "${ADDER_RUST_FMU_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            DEPENDS
                adder-fmu-c-runner-build
                adder-rust-fmu-archive
                "${ADDER_FMU_C_RUNNER_DIR}/main.c"
            COMMENT "Running adder-fmu-c-runner against adder-rust-fmu.fmu..."
            VERBATIM
        )

        add_test(
            NAME adder-fmu-c-runner
            COMMAND "${ADDER_FMU_C_RUNNER_BIN}" "${ADDER_RUST_FMU_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
        set_tests_properties(adder-fmu-c-runner
            PROPERTIES DEPENDS adder-rust-fmu-wasm-validation
        )

        if(TARGET run-all-examples)
            add_dependencies(run-all-examples adder-fmu-c-runner)
        endif()
    endif()

    # -----------------------------------------------------------------------
    # Top-level convenience target for all FMU examples and runners.
    #
    # Build/run with:
    #   cmake --build <build-dir> --target run-all-examples
    # -----------------------------------------------------------------------
    add_custom_target(run-all-examples
        DEPENDS
            validate-wit
            validate-adder-rust-fmu
            adder-fmu-rust-runner
    )
endif()

# ---------------------------------------------------------------------------
# Example FMU: adder-c-fmu
#
# A Co-Simulation FMU identical in behaviour to adder-rust-fmu but implemented in
# pure C, using wit-bindgen-generated C bindings compiled via the WASI SDK
# toolchain (wasm32-wasip2).
#
# Build:   cmake --build <build-dir> --target adder-c-fmu
# Output:  <build-dir>/adder_c_fmu.wasm
# ---------------------------------------------------------------------------
if(WIT_BINDGEN_EXECUTABLE)
    set(ADDER_C_FMU_DIR    "${CMAKE_SOURCE_DIR}/examples/adder-c-fmu")
    set(ADDER_C_FMU_BINDGEN_DIR "${CMAKE_BINARY_DIR}/adder-c-fmu-bindgen")

    # Generated file names produced by: wit-bindgen c --world co-simulation-fmu
    set(ADDER_C_FMU_BINDGEN_H   "${ADDER_C_FMU_BINDGEN_DIR}/co_simulation_fmu.h")
    set(ADDER_C_FMU_BINDGEN_C   "${ADDER_C_FMU_BINDGEN_DIR}/co_simulation_fmu.c")
    set(ADDER_C_FMU_BINDGEN_OBJ "${ADDER_C_FMU_BINDGEN_DIR}/co_simulation_fmu_component_type.o")

    # -----------------------------------------------------------------------
    # Generate C bindings from the WIT package
    # -----------------------------------------------------------------------
    add_custom_command(
        OUTPUT
            "${ADDER_C_FMU_BINDGEN_H}"
            "${ADDER_C_FMU_BINDGEN_C}"
            "${ADDER_C_FMU_BINDGEN_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDER_C_FMU_BINDGEN_DIR}"
        COMMAND ${WIT_BINDGEN_EXECUTABLE} c
                --world co-simulation-fmu
                "${CMAKE_SOURCE_DIR}/wit"
                --out-dir "${ADDER_C_FMU_BINDGEN_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating C bindings for adder-c-fmu..."
        VERBATIM
        DEPENDS ${WIT_FILES}
    )

    # -----------------------------------------------------------------------
    # Compile the C FMU to a wasm32-wasip2 component
    # -----------------------------------------------------------------------
    set(ADDER_C_FMU_WASM "${CMAKE_BINARY_DIR}/adder_c_fmu.wasm")

    add_executable(adder-c-fmu-wasm
        "${ADDER_C_FMU_DIR}/adder-c-fmu.c"
        "${ADDER_C_FMU_BINDGEN_C}"
    )

    target_include_directories(adder-c-fmu-wasm PRIVATE "${ADDER_C_FMU_BINDGEN_DIR}")

    set_target_properties(adder-c-fmu-wasm PROPERTIES
        OUTPUT_NAME "adder_c_fmu"
        SUFFIX      ".wasm"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    # Link the component-type object that wit-bindgen produces so the wasm
    # binary carries the correct component-model metadata section.
    target_link_options(adder-c-fmu-wasm PRIVATE
        "${ADDER_C_FMU_BINDGEN_OBJ}"
    )

    # Ensure bindings are generated before compilation.
    add_custom_target(adder-c-fmu-bindgen
        DEPENDS
            "${ADDER_C_FMU_BINDGEN_H}"
            "${ADDER_C_FMU_BINDGEN_C}"
            "${ADDER_C_FMU_BINDGEN_OBJ}"
    )
    add_dependencies(adder-c-fmu-wasm adder-c-fmu-bindgen)

    add_custom_target(adder-c-fmu DEPENDS adder-c-fmu-wasm)

    # -----------------------------------------------------------------------
    # Compile the C FMU to a native x86_64-linux shared library
    #
    # The same source file is used with the native toolchain (not the WASI SDK)
    # and without any WIT-bindgen headers; __wasm__ is not defined, so the
    # #else branch in adder-c-fmu.c activates the standard FMI 3.0 C API.
    # -----------------------------------------------------------------------
    set(ADDER_C_FMU_NATIVE_SO "${CMAKE_BINARY_DIR}/adder_c_fmu.so")

    add_custom_command(
        OUTPUT "${ADDER_C_FMU_NATIVE_SO}"
        COMMAND cc -std=c11 -O2 -Wall -Wextra -shared -fPIC
                "${ADDER_C_FMU_DIR}/adder-c-fmu.c"
                -o "${ADDER_C_FMU_NATIVE_SO}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS "${ADDER_C_FMU_DIR}/adder-c-fmu.c"
        COMMENT "Building adder-c-fmu native shared library (x86_64-linux)..."
        VERBATIM
    )

    add_custom_target(adder-c-fmu-native
        DEPENDS "${ADDER_C_FMU_NATIVE_SO}"
    )

    add_test(
        NAME adder-c-fmu-native-symbols
        COMMAND /bin/sh -c
            "nm -D '${ADDER_C_FMU_NATIVE_SO}' | grep -q ' T fmi3InstantiateCoSimulation'"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(adder-c-fmu-native-symbols
        PROPERTIES DEPENDS adder-c-fmu-wasm-validation
    )

    # Validate the compiled component against the WIT package.
    add_custom_target(validate-adder-c-fmu
        COMMAND ${WASM_TOOLS_EXECUTABLE} validate --features component-model
                "${ADDER_C_FMU_WASM}"
        DEPENDS adder-c-fmu-wasm
        COMMENT "Validating adder-c-fmu Wasm component..."
        VERBATIM
    )

    add_test(
        NAME adder-c-fmu-wasm-validation
        COMMAND ${WASM_TOOLS_EXECUTABLE} validate --features component-model
                "${ADDER_C_FMU_WASM}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(adder-c-fmu-wasm-validation
        PROPERTIES DEPENDS wit-validation
    )

    # -----------------------------------------------------------------------
    # Package adder-c-fmu as a proper FMU archive (.fmu = zip)
    # -----------------------------------------------------------------------
    set(ADDER_C_FMU_ARCHIVE  "${CMAKE_BINARY_DIR}/adder-c-fmu.fmu")
    set(ADDER_C_FMU_STAGING  "${CMAKE_BINARY_DIR}/adder-c-fmu-staging")
    set(ADDER_C_FMU_MODEL_DESC "${ADDER_C_FMU_DIR}/modelDescription.xml")

    add_custom_command(
        OUTPUT "${ADDER_C_FMU_ARCHIVE}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADDER_C_FMU_STAGING}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${ADDER_C_FMU_STAGING}/binaries/wasm32-wasip2"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${ADDER_C_FMU_STAGING}/binaries/x86_64-linux"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${ADDER_C_FMU_MODEL_DESC}"
                "${ADDER_C_FMU_STAGING}/modelDescription.xml"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${ADDER_C_FMU_WASM}"
                "${ADDER_C_FMU_STAGING}/binaries/wasm32-wasip2/adder_c_fmu.wasm"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${ADDER_C_FMU_NATIVE_SO}"
            "${ADDER_C_FMU_STAGING}/binaries/x86_64-linux/adder_c_fmu.so"
        COMMAND ${CMAKE_COMMAND} -E chdir "${ADDER_C_FMU_STAGING}"
                ${CMAKE_COMMAND} -E tar cf "${ADDER_C_FMU_ARCHIVE}" --format=zip
                -- modelDescription.xml binaries
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        DEPENDS
            adder-c-fmu-wasm
            "${ADDER_C_FMU_NATIVE_SO}"
            "${ADDER_C_FMU_MODEL_DESC}"
        COMMENT "Packaging adder-c-fmu FMU archive (adder-c-fmu.fmu)..."
        VERBATIM
    )

    add_custom_target(adder-c-fmu-archive
        DEPENDS "${ADDER_C_FMU_ARCHIVE}"
    )

    # -----------------------------------------------------------------------
    # Run the native C runner against adder-c-fmu to validate end-to-end
    # -----------------------------------------------------------------------
    if(TARGET adder-fmu-c-runner-build)
        add_custom_target(adder-c-fmu-runner
            COMMAND "${ADDER_FMU_C_RUNNER_BIN}" "${ADDER_C_FMU_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            DEPENDS
                adder-fmu-c-runner-build
                adder-c-fmu-archive
                "${ADDER_FMU_C_RUNNER_DIR}/main.c"
            COMMENT "Running adder-fmu-c-runner against adder-c-fmu.fmu..."
            VERBATIM
        )

        add_test(
            NAME adder-c-fmu-runner
            COMMAND "${ADDER_FMU_C_RUNNER_BIN}" "${ADDER_C_FMU_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
        set_tests_properties(adder-c-fmu-runner
            PROPERTIES DEPENDS adder-c-fmu-wasm-validation
        )

        if(TARGET run-all-examples)
            add_dependencies(run-all-examples
                validate-adder-c-fmu
                adder-c-fmu-runner
            )
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# Example FMU: adder-wat-fmu
#
# A Co-Simulation FMU with the same observable behaviour as adder-rust-fmu,
# provided as a checked-in WebAssembly text (WAT) component source and built
# with wasm-tools.
#
# Build:   cmake --build <build-dir> --target adder-wat-fmu
# Output:  <build-dir>/adder_wat_fmu.wasm
# ---------------------------------------------------------------------------
set(ADDER_WAT_FMU_DIR    "${CMAKE_SOURCE_DIR}/examples/adder-wat-fmu")
set(ADDER_WAT_FMU_WAT    "${ADDER_WAT_FMU_DIR}/adder-wat-fmu.wat")
set(ADDER_WAT_FMU_WASM   "${CMAKE_BINARY_DIR}/adder_wat_fmu.wasm")
set(ADDER_WAT_FMU_ARCHIVE  "${CMAKE_BINARY_DIR}/adder-wat-fmu.fmu")
set(ADDER_WAT_FMU_STAGING  "${CMAKE_BINARY_DIR}/adder-wat-fmu-staging")
set(ADDER_WAT_FMU_MODEL_DESC "${ADDER_WAT_FMU_DIR}/modelDescription.xml")

add_custom_command(
    OUTPUT "${ADDER_WAT_FMU_WASM}"
    COMMAND ${WASM_TOOLS_EXECUTABLE} parse
            "${ADDER_WAT_FMU_WAT}"
            -o "${ADDER_WAT_FMU_WASM}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS
        "${ADDER_WAT_FMU_WAT}"
    COMMENT "Building adder-wat-fmu Wasm component from WAT..."
    VERBATIM
)

add_custom_target(adder-wat-fmu
    DEPENDS "${ADDER_WAT_FMU_WASM}"
)

add_custom_target(validate-adder-wat-fmu
    COMMAND ${WASM_TOOLS_EXECUTABLE} validate --features component-model
            "${ADDER_WAT_FMU_WASM}"
    DEPENDS adder-wat-fmu
    COMMENT "Validating adder-wat-fmu Wasm component..."
    VERBATIM
)

add_test(
    NAME adder-wat-fmu-wasm-validation
    COMMAND ${WASM_TOOLS_EXECUTABLE} validate --features component-model
            "${ADDER_WAT_FMU_WASM}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
set_tests_properties(adder-wat-fmu-wasm-validation
    PROPERTIES DEPENDS wit-validation
)

add_custom_command(
    OUTPUT "${ADDER_WAT_FMU_ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADDER_WAT_FMU_STAGING}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${ADDER_WAT_FMU_STAGING}/binaries/wasm32-wasip2"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${ADDER_WAT_FMU_MODEL_DESC}"
            "${ADDER_WAT_FMU_STAGING}/modelDescription.xml"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${ADDER_WAT_FMU_WASM}"
            "${ADDER_WAT_FMU_STAGING}/binaries/wasm32-wasip2/adder_wat_fmu.wasm"
    COMMAND ${CMAKE_COMMAND} -E chdir "${ADDER_WAT_FMU_STAGING}"
            ${CMAKE_COMMAND} -E tar cf "${ADDER_WAT_FMU_ARCHIVE}" --format=zip
            -- modelDescription.xml binaries
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS
        adder-wat-fmu
        "${ADDER_WAT_FMU_MODEL_DESC}"
    COMMENT "Packaging adder-wat-fmu FMU archive (adder-wat-fmu.fmu)..."
    VERBATIM
)

add_custom_target(adder-wat-fmu-archive
    DEPENDS "${ADDER_WAT_FMU_ARCHIVE}"
)

if(TARGET adder-fmu-rust-runner-build)
    add_custom_target(adder-wat-fmu-rust-runner
        COMMAND "${ADDER_FMU_RUST_RUNNER_BIN}" "${ADDER_WAT_FMU_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS
            adder-fmu-rust-runner-build
            adder-wat-fmu-archive
        COMMENT "Running adder-fmu-rust-runner against adder-wat-fmu.fmu..."
        VERBATIM
    )

    add_test(
        NAME adder-wat-fmu-rust-runner
        COMMAND "${ADDER_FMU_RUST_RUNNER_BIN}" "${ADDER_WAT_FMU_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(adder-wat-fmu-rust-runner
        PROPERTIES DEPENDS adder-wat-fmu-wasm-validation
    )
endif()

if(TARGET adder-fmu-c-runner-build)
    add_custom_target(adder-wat-fmu-c-runner
        COMMAND "${ADDER_FMU_C_RUNNER_BIN}" "${ADDER_WAT_FMU_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS
            adder-fmu-c-runner-build
            adder-wat-fmu-archive
        COMMENT "Running adder-fmu-c-runner against adder-wat-fmu.fmu..."
        VERBATIM
    )

    add_test(
        NAME adder-wat-fmu-c-runner
        COMMAND "${ADDER_FMU_C_RUNNER_BIN}" "${ADDER_WAT_FMU_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(adder-wat-fmu-c-runner
        PROPERTIES DEPENDS adder-wat-fmu-wasm-validation
    )
endif()

if(TARGET run-all-examples)
    add_dependencies(run-all-examples validate-adder-wat-fmu)
    if(TARGET adder-wat-fmu-rust-runner)
        add_dependencies(run-all-examples adder-wat-fmu-rust-runner)
    endif()
    if(TARGET adder-wat-fmu-c-runner)
        add_dependencies(run-all-examples adder-wat-fmu-c-runner)
    endif()
endif()
