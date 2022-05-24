cmake_minimum_required (VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

FUNCTION(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
ENDFUNCTION()

project (Reference-FMUs)

find_package (Python3)
find_package (Git)

set(FMI_VERSION 2 CACHE STRING "FMI Version")
set_property(CACHE FMI_VERSION PROPERTY STRINGS 1 2 3)

set(FMI_TYPE ME CACHE STRING "FMI Type (FMI 1.0 only)")
set_property(CACHE FMI_TYPE PROPERTY STRINGS ME CS)

if (${FMI_VERSION} GREATER 1)
  set(FMI_TYPE "")
endif ()

if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

  # Minimum supported MSVC version is 1800 = Visual Studio 12.0/2013
  # See also https://cmake.org/cmake/help/latest/variable/MSVC_VERSION.html
  if (MSVC_VERSION VERSION_LESS 1800)
    message (SEND_ERROR "MSVC_VERSION ${MSVC_VERSION} is not a supported Visual Studio compiler version. Please use Visual Studio 2013 or any later version.")
  endif ()
endif ()

add_compile_definitions(FMI_VERSION=${FMI_VERSION})

if (${FMI_VERSION} GREATER 2)

  if (WIN32)
    set(FMI_PLATFORM windows)
  elseif (APPLE)
    set(FMI_PLATFORM darwin)
  else ()
    set(FMI_PLATFORM linux)
  endif ()

  if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set (FMI_PLATFORM x86_64-${FMI_PLATFORM})
  else ()
    set (FMI_PLATFORM x86-${FMI_PLATFORM})
  endif ()

else ()

  if (WIN32)
    set(FMI_PLATFORM win)
  elseif (APPLE)
    set(FMI_PLATFORM darwin)
  else ()
    set(FMI_PLATFORM linux)
  endif ()

  if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set (FMI_PLATFORM ${FMI_PLATFORM}64)
  else ()
    set (FMI_PLATFORM ${FMI_PLATFORM}32)
  endif ()

endif ()

MESSAGE("FMI_PLATFORM: " ${FMI_PLATFORM})

if (${FMI_VERSION} LESS 3)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/all.c" "#define FMI_VERSION ${FMI_VERSION}

#include \"fmi${FMI_VERSION}Functions.c\"
#include \"model.c\"
#include \"cosimulation.c\"
")
endif ()

set (MODEL_NAMES BouncingBall Dahlquist Stair Feedthrough VanDerPol)

if (${FMI_VERSION} GREATER 1 OR "${FMI_TYPE}" STREQUAL "CS")
  set (MODEL_NAMES ${MODEL_NAMES} Resource)
endif ()

if (${FMI_VERSION} EQUAL 3)
  set (MODEL_NAMES ${MODEL_NAMES} LinearTransform Clocks Controller Plant Supervisor)
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
  src/cosimulation.c
)

add_library(${TARGET_NAME} SHARED
  ${HEADERS}
  ${SOURCES}
  ${MODEL_NAME}/FMI${FMI_VERSION}${FMI_TYPE}.xml
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist)

target_compile_definitions(${TARGET_NAME} PRIVATE
  DISABLE_PREFIX
)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_link_options(${TARGET_NAME} PRIVATE "-static-libgcc")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
  target_link_options(${TARGET_NAME} PRIVATE "-static-intel" "-static-libgcc")
endif()

if (${FMI_VERSION} EQUAL 1 AND "${FMI_TYPE}" STREQUAL CS)
  target_compile_definitions(${TARGET_NAME} PRIVATE FMI_COSIMULATION)
endif()

target_include_directories(${TARGET_NAME} PRIVATE
  include
  ${MODEL_NAME}
)

set(FMU_BUILD_DIR temp/${MODEL_NAME})

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

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# modelDescription.xml
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/FMI${FMI_VERSION}${FMI_TYPE}.xml
  "${FMU_BUILD_DIR}/modelDescription.xml"
)

if (${Python3_Interpreter_FOUND} AND ${Git_FOUND})
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/set_tool_version.py"
    "${FMU_BUILD_DIR}/modelDescription.xml" "${GIT_EXECUTABLE}"
  )
endif()

# model specific header and source
foreach (SOURCE_FILE config.h model.c)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/${SOURCE_FILE}"
    "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
  )
endforeach(SOURCE_FILE)

# documentation
if (${FMI_VERSION} EQUAL 1)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/readme.html"
    "${FMU_BUILD_DIR}/documentation/_main.html"
  )
else()
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/readme.html"
    "${FMU_BUILD_DIR}/documentation/index.html"
  )
endif()

# plot
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/${MODEL_NAME}_ref.svg"
  "${FMU_BUILD_DIR}/documentation/${MODEL_NAME}_ref.svg"
)

# reference csv
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/${MODEL_NAME}_ref.csv"
  "${FMU_BUILD_DIR}/documentation/${MODEL_NAME}_ref.csv"
)

# license
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
  "${FMU_BUILD_DIR}/documentation/LICENSE.txt"
)

# common headers
foreach (SOURCE_FILE model.h cosimulation.h)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/include/${SOURCE_FILE}"
    "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
  )
endforeach(SOURCE_FILE)

# common sources
foreach (SOURCE_FILE fmi${FMI_VERSION}Functions.c cosimulation.c)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${SOURCE_FILE}"
    "${FMU_BUILD_DIR}/sources/${SOURCE_FILE}"
  )
endforeach(SOURCE_FILE)

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

set(ARCHIVE_FILES "modelDescription.xml" "binaries" "documentation" "sources")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/resources")
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}/resources"
    "${FMU_BUILD_DIR}/resources/"
  )
  set(ARCHIVE_FILES ${ARCHIVE_FILES} "resources")
endif()

# create ZIP archive
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E tar "cfv" ${CMAKE_CURRENT_BINARY_DIR}/dist/${MODEL_NAME}.fmu --format=zip
  ${ARCHIVE_FILES}
  WORKING_DIRECTORY ${FMU_BUILD_DIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dist/${MODEL_NAME}.fmu DESTINATION .)

endforeach(MODEL_NAME)

# Examples
include(examples/Examples.cmake)
