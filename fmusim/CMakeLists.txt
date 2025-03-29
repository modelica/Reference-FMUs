cmake_minimum_required(VERSION 3.18)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

if (WIN32)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-windows")
elseif (APPLE)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-darwin")
else ()
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-linux")
endif ()

set(FMUSIM_VERSION "" CACHE STRING "")

include(ExternalProject)
set(EXTERNAL_BASE_DIR ${CMAKE_BINARY_DIR}/external CACHE STRING "External base directory")

ExternalProject_Add(
    zlib_src
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf # v1.3.1
    GIT_SHALLOW True
    UPDATE_DISCONNECTED True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BASE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -DZLIB_BUILD_MINIZIP=1
    PREFIX ${EXTERNAL_BASE_DIR}
    BUILD_BYPRODUCTS ${EXTERNAL_BASE_DIR}/lib/libz.a
)
add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES IMPORTED_LOCATION ${EXTERNAL_BASE_DIR}/lib/libz.a)
add_dependencies(zlib zlib_src)

set(ZLIB_SRC_DIR ${EXTERNAL_BASE_DIR}/src/zlib_src)
set_source_files_properties(${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c PROPERTIES GENERATED 1)
set_source_files_properties(${ZLIB_SRC_DIR}/contrib/minizip/unzip.c PROPERTIES GENERATED 1)
set_source_files_properties(${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c PROPERTIES GENERATED 1)

ExternalProject_Add(
    xml2_src
    GIT_REPOSITORY https://github.com/GNOME/libxml2.git
    GIT_TAG 60d3056c97067e6cb2125284878ed7c99c90ed81 # v2.13.4
    GIT_SHALLOW True
    UPDATE_DISCONNECTED True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BASE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -DBUILD_SHARED_LIBS=OFF -DLIBXML2_WITH_ICONV=OFF -DLIBXML2_WITH_LZMA=OFF -DLIBXML2_WITH_PYTHON=OFF -DLIBXML2_WITH_ZLIB=OFF -DLIBXML2_WITH_TESTS=OFF
    PREFIX ${EXTERNAL_BASE_DIR}
    BUILD_BYPRODUCTS ${EXTERNAL_BASE_DIR}/lib/libxml2.a
)
add_library(xml2 STATIC IMPORTED)
set_target_properties(xml2 PROPERTIES IMPORTED_LOCATION ${EXTERNAL_BASE_DIR}/lib/libxml2.a)
add_dependencies(xml2 xml2_src)

ExternalProject_Add(
    cvcode_src
    GIT_REPOSITORY https://github.com/LLNL/sundials.git
    GIT_TAG c28eaa3764a03705d61decb6025b409360e9d53f # v7.1.1
    GIT_SHALLOW True
    UPDATE_DISCONNECTED True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BASE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DEXAMPLES_INSTALL=OFF -DSUNDIALS_ENABLE_ERROR_CHECKS=OFF
    PREFIX ${EXTERNAL_BASE_DIR}
    BUILD_BYPRODUCTS ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
)
add_library(cvcode STATIC IMPORTED)
set_target_properties(cvcode PROPERTIES IMPORTED_LOCATION ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a)
add_dependencies(cvcode cvcode_src)

if (WIN32)
    set(FMUSIM_DIST_DIR ${CMAKE_BINARY_DIR}/dist/fmusim-windows/)
elseif (APPLE)
    set(FMUSIM_DIST_DIR ${CMAKE_BINARY_DIR}/dist/fmusim-darwin/)
else ()
    set(FMUSIM_DIST_DIR ${CMAKE_BINARY_DIR}/dist/fmusim-linux/)
endif ()

project (FMUSim)

file(MAKE_DIRECTORY ${FMUSIM_DIST_DIR})

set(sources
  ../include/FMI.h
  ../src/FMI.c
  ../include/FMI1.h
  ../src/FMI1.c
  ../include/FMI2.h
  ../src/FMI2.c
  ../include/FMI3.h
  ../src/FMI3.c
  csv.h
  csv.c
  FMIUtil.h
  FMIUtil.c
  FMISolver.h
  FMIEuler.h
  FMIEuler.c
  FMICVode.h
  FMICVode.c
  FMIModelDescription.h
  FMIModelDescription.c
  FMIRecorder.h
  FMIRecorder.c
  FMIZip.h
  FMIZip.c
  fmi1schema.h
  fmi2schema.h
  fmi3schema.h
  FMISimulation.h
  FMISimulation.c
  FMIStaticInput.h
  FMIStaticInput.c
  FMI1CSSimulation.h
  FMI1CSSimulation.c
  FMI1MESimulation.h
  FMI1MESimulation.c
  FMI2CSSimulation.h
  FMI2CSSimulation.c
  FMI2MESimulation.h
  FMI2MESimulation.c
  FMI3CSSimulation.h
  FMI3CSSimulation.c
  FMI3MESimulation.h
  FMI3MESimulation.c
  fmusim.c
  miniunzip.c
  ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
  ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
  ../src/structured_variable_name.yy.c
  ../include/structured_variable_name.tab.h
  ../src/structured_variable_name.tab.c
)

if (WIN32)
    set (sources ${sources} ${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c)
endif ()

add_executable(fmusim ${sources})
add_dependencies(fmusim zlib xml2 cvcode)

target_include_directories(fmusim PRIVATE
  .
  ../include
  ${EXTERNAL_BASE_DIR}/include
  ${EXTERNAL_BASE_DIR}/include/libxml2
  ${ZLIB_SRC_DIR}/contrib/minizip
)

if (WIN32)
    target_compile_definitions(fmusim PRIVATE FMUSIM_VERSION=${FMUSIM_VERSION} YY_NO_UNISTD_H LIBXML_STATIC)
else ()
    target_compile_definitions(fmusim PRIVATE FMUSIM_VERSION=${FMUSIM_VERSION} LIBXML_STATIC)
endif ()

if (WIN32)
    set(libraries
      ${EXTERNAL_BASE_DIR}/lib/libxml2s.lib
      ${EXTERNAL_BASE_DIR}/lib/zlibstatic.lib
      ${EXTERNAL_BASE_DIR}/lib/sundials_cvode_static.lib
      ${EXTERNAL_BASE_DIR}/lib/sundials_core_static.lib
      wsock32
      ws2_32
      bcrypt
    )
elseif(UNIX AND NOT APPLE)
    set(libraries
      ${EXTERNAL_BASE_DIR}/lib/libxml2.a
      ${EXTERNAL_BASE_DIR}/lib/libz.a
      ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a
      ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
      ${CMAKE_DL_LIBS}
      m
    )
else ()
    set(libraries
      ${EXTERNAL_BASE_DIR}/lib/libxml2.a
      ${EXTERNAL_BASE_DIR}/lib/libz.a
      ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a
      ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
    )
endif ()

target_link_libraries(fmusim ${libraries})

install(TARGETS fmusim DESTINATION ${CMAKE_INSTALL_PREFIX})
