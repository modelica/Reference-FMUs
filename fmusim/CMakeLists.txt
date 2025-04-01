cmake_minimum_required (VERSION 3.18)

set(FMUSIM_VERSION "" CACHE STRING "")

project (FMUSim)

include(${CMAKE_SOURCE_DIR}/build/sim_common.cmake)

if(WIN32)
    set(FMUSIM_DIST_DIR ${CMAKE_BINARY_DIR}/dist/fmusim-windows/)
elseif(APPLE)
    set(FMUSIM_DIST_DIR ${CMAKE_BINARY_DIR}/dist/fmusim-darwin/)
else()
    set(FMUSIM_DIST_DIR ${CMAKE_BINARY_DIR}/dist/fmusim-linux/)
endif()

file(MAKE_DIRECTORY ${FMUSIM_DIST_DIR})

set(FMUSIM_CLI_SOURCES
  ${FMUSIM_SOURCES}
  FMISolver.h
  fmi1schema.h
  fmi2schema.h
  fmi3schema.h
  fmusim.c
)

add_executable(fmusim ${FMUSIM_CLI_SOURCES})
add_dependencies(fmusim ${FMUSIM_DEPENDS})

target_include_directories(fmusim PRIVATE ${FMUSIM_INCLUDES})

if (WIN32)
    target_compile_definitions(fmusim PRIVATE FMUSIM_VERSION=${FMUSIM_VERSION} YY_NO_UNISTD_H LIBXML_STATIC)
else ()
    target_compile_definitions(fmusim PRIVATE FMUSIM_VERSION=${FMUSIM_VERSION} LIBXML_STATIC)
endif ()

if (WIN32)
    set(FMUSIM_CLI_LIBS ${FMUSIM_LIBS} wsock32 ws2_32 bcrypt)
else ()
    set(FMUSIM_CLI_LIBS ${FMUSIM_LIBS})
endif ()

target_link_libraries(fmusim ${FMUSIM_CLI_LIBS})

install(TARGETS fmusim DESTINATION ${CMAKE_INSTALL_PREFIX})
