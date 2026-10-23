cmake_minimum_required (VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

FUNCTION(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
ENDFUNCTION()

project (Reference-FMUs)

set(FMI_VERSION 3 CACHE STRING "FMI Version")
set_property(CACHE FMI_VERSION PROPERTY STRINGS 2 3)

set(FMI_TARGET "native" CACHE STRING "Build native shared libraries or WebAssembly FMUs")
set_property(CACHE FMI_TARGET PROPERTY STRINGS native wasm)

set(FMI_ARCHITECTURE "" CACHE STRING "FMI Architecture")
set_property(CACHE FMI_ARCHITECTURE PROPERTY STRINGS "" "aarch64" "x86" "x86_64")

if (NOT FMI_ARCHITECTURE)
  if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "AMD64|x86_64")
    set(FMI_ARCHITECTURE "x86_64")
  elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64|arm64")
    set(FMI_ARCHITECTURE "aarch64")
  else ()
    message(FATAL_ERROR "Unknown System Architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
  endif ()
endif ()

if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

  # Minimum supported MSVC version is 1800 = Visual Studio 12.0/2013
  # See also https://cmake.org/cmake/help/latest/variable/MSVC_VERSION.html
  if (MSVC_VERSION VERSION_LESS 1800)
    message (SEND_ERROR "MSVC_VERSION ${MSVC_VERSION} is not a supported Visual Studio compiler version. Please use Visual Studio 2013 or any later version.")
  endif ()
endif ()

# hide internal functions
if (UNIX AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()

if (FMI_TARGET STREQUAL "wasm")
  if (NOT ${FMI_VERSION} EQUAL 3)
    message(FATAL_ERROR "FMI_TARGET=wasm requires FMI_VERSION=3.")
  endif ()
  set(FMI_PLATFORM "wasm32-wasip2")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "x86" AND WIN32)
  set(FMI_PLATFORM "win32")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "x86_64" AND WIN32)
  set(FMI_PLATFORM "win64")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "x86" AND UNIX AND NOT APPLE)
  set(FMI_PLATFORM "linux32")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "x86_64" AND UNIX AND NOT APPLE)
  set(FMI_PLATFORM "linux64")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "x86_64" AND APPLE)
  set(FMI_PLATFORM "darwin64")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "aarch64" AND UNIX AND NOT APPLE)
  set(FMI_PLATFORM "aarch64-linux")
elseif (${FMI_VERSION} EQUAL 2 AND "${FMI_ARCHITECTURE}" STREQUAL "aarch64" AND APPLE)
  set(FMI_PLATFORM "aarch64-darwin")
else ()
  if (WIN32)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-windows")
  elseif (APPLE)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-darwin")
  else ()
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-linux")
  endif ()
endif ()

message("FMI_PLATFORM: " ${FMI_PLATFORM})

if (${FMI_VERSION} LESS 3)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/all.c" "#define FMI_VERSION ${FMI_VERSION}

#include \"fmi${FMI_VERSION}Functions.c\"
#include \"model.c\"
#include \"common.c\"
#include \"cosimulation.c\"
")
endif ()

set (MODEL_NAMES BouncingBall Dahlquist Feedthrough Resource Stair VanDerPol)

if (${FMI_VERSION} GREATER 2)
  set (MODEL_NAMES ${MODEL_NAMES} Clocks Roberts StateSpace)
endif ()

if (FMI_TARGET STREQUAL "wasm")
  list(REMOVE_ITEM MODEL_NAMES Clocks)
  include(${CMAKE_CURRENT_SOURCE_DIR}/Wasm/WasmFMU.cmake)
endif ()

foreach (MODEL_NAME ${MODEL_NAMES})

set(TARGET_NAME ${MODEL_NAME})

SET(HEADERS
    ${MODEL_NAME}/config.h
    include/cosimulation.h
    include/model.h
)

if (${FMI_VERSION} EQUAL 2)
  SET(HEADERS
    ${HEADERS}
    include/fmi2Functions.h
    include/fmi2FunctionTypes.h
    include/fmi2TypesPlatform.h
  )
elseif (${FMI_VERSION} EQUAL 3)
  SET(HEADERS
    ${HEADERS}
    include/fmi3Functions.h
    include/fmi3FunctionTypes.h
    include/fmi3PlatformTypes.h
  )
endif()

SET(SOURCES
  ${MODEL_NAME}/model.c
  src/fmi${FMI_VERSION}Functions.c
  src/common.c
  src/cosimulation.c
  ${MODEL_NAME}/buildDescription.xml
  ${MODEL_NAME}/FMI${FMI_VERSION}.xml
)

if("${MODEL_NAME}" STREQUAL "Resource")
  list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/y.txt")
endif()

if(NOT "${MODEL_NAME}" STREQUAL "Clocks")
  list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/fmi-ls-ref.xml")
endif()

if("${MODEL_NAME}" STREQUAL "Roberts")
  list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/fmi-ls-dae.xml")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmus)

set(FMU_BUILD_DIR temp/${MODEL_NAME})

if (FMI_TARGET STREQUAL "native")

add_library(${TARGET_NAME} SHARED ${HEADERS} ${SOURCES})

target_compile_definitions(${TARGET_NAME} PRIVATE
  FMI_VERSION=${FMI_VERSION}
  DISABLE_PREFIX
)

if (MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_link_options(${TARGET_NAME} PRIVATE "-static-libgcc")
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "Intel")
  target_link_options(${TARGET_NAME} PRIVATE "-static-intel" "-static-libgcc")
endif()

target_include_directories(${TARGET_NAME} PRIVATE
  include
  ${MODEL_NAME}
)

set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    LIBRARY_OUTPUT_DIRECTORY         "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    ARCHIVE_OUTPUT_DIRECTORY         "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}"
)

set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${MODEL_NAME})

elseif (FMI_TARGET STREQUAL "wasm")

reference_fmu_add_wasm_component(${MODEL_NAME})

endif ()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# modelDescription.xml
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/FMI${FMI_VERSION}.xml"
  "${FMU_BUILD_DIR}/modelDescription.xml"
)

set(ARCHIVE_FILES "modelDescription.xml" "binaries" "sources")

if(NOT "${MODEL_NAME}" STREQUAL "Clocks")
  # fmi-ls-ref.xml
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/fmi-ls-ref.xml"
    "${FMU_BUILD_DIR}/extra/org.fmi-standard.fmi-ls-ref/fmi-ls-manifest.xml"
  )

  # {model_name}_out.csv
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/${MODEL_NAME}_out.csv"
    "${FMU_BUILD_DIR}/extra/org.fmi-standard.fmi-ls-ref/${MODEL_NAME}_out.csv"
  )

  list(APPEND ARCHIVE_FILES "extra")
endif()

if("${MODEL_NAME}" STREQUAL "Roberts")
  # fmi-ls-dae.xml
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/fmi-ls-dae.xml"
    "${FMU_BUILD_DIR}/extra/org.fmi-standard.fmi-ls-dae/fmi-ls-manifest.xml"
  )
  list(APPEND ARCHIVE_FILES "extra")
endif()

# model specific header and source
foreach (SOURCE_FILE config.h model.c)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/${SOURCE_FILE}"
    "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
  )
endforeach(SOURCE_FILE)

# common headers
foreach (SOURCE_FILE model.h cosimulation.h)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/include/${SOURCE_FILE}"
    "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
  )
endforeach(SOURCE_FILE)

# common sources
if (FMI_TARGET STREQUAL "wasm")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/Wasm/fmu_wasm.c"
    "${FMU_BUILD_DIR}/sources/fmu_wasm.c"
  )
  foreach (SOURCE_FILE common.c cosimulation.c)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/src/${SOURCE_FILE}"
      "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
    )
  endforeach(SOURCE_FILE)
  foreach (SOURCE_FILE co_simulation_fmu.c co_simulation_fmu.h)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
      "${WASM_BINDGEN_DIR}/${SOURCE_FILE}"
      "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
    )
  endforeach(SOURCE_FILE)
else()
  foreach (SOURCE_FILE fmi${FMI_VERSION}Functions.c common.c cosimulation.c)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/src/${SOURCE_FILE}"
      "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
    )
  endforeach(SOURCE_FILE)
endif()

# all.c / buildDescription.xml
if (${FMI_VERSION} LESS 3)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_BINARY_DIR}/all.c"
    "${FMU_BUILD_DIR}/sources/all.c"
  )
else()
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/buildDescription.xml"
    "${FMU_BUILD_DIR}/sources/buildDescription.xml"
  )
endif()

# resources/y.txt
if("${MODEL_NAME}" STREQUAL "Resource")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/y.txt"
    "${FMU_BUILD_DIR}/resources/y.txt"
  )
  list(APPEND ARCHIVE_FILES "resources")
endif()

# delete unintended files from binaries (Release configuration only)
if (MSVC AND FMI_TARGET STREQUAL "native")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND if $<CONFIG:Release> neq 0 (
      ${CMAKE_COMMAND} -E rm -f
      "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}/${MODEL_NAME}.exp"
      "${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}/${MODEL_NAME}.lib"
     )
  )
endif()

# create ZIP archive
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E tar "cfv" ${CMAKE_CURRENT_BINARY_DIR}/fmus/${MODEL_NAME}.fmu --format=zip
  ${ARCHIVE_FILES}
  WORKING_DIRECTORY ${FMU_BUILD_DIR} COMMENT "Creating ZIP archive"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fmus/${MODEL_NAME}.fmu DESTINATION ${CMAKE_INSTALL_PREFIX})

endforeach(MODEL_NAME)

# Examples (native shared libraries only)
if (FMI_TARGET STREQUAL "native")
  include(examples/Examples.cmake)
endif ()
