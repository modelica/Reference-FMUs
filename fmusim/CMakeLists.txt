cmake_minimum_required (VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

if (WIN32)
    set(FMI_PLATFORM windows)
elseif (APPLE)
    set(FMI_PLATFORM darwin)
else ()
    set(FMI_PLATFORM linux)
endif ()

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" OR "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
    set (FMI_PLATFORM aarch64-${FMI_PLATFORM})
elseif ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set (FMI_PLATFORM x86_64-${FMI_PLATFORM})
else ()
    set (FMI_PLATFORM x86-${FMI_PLATFORM})
endif ()

message("CMAKE_OSX_ARCHITECTURES: " ${CMAKE_OSX_ARCHITECTURES})
message("CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})
message("CMAKE_HOST_SYSTEM_PROCESSOR: " ${CMAKE_HOST_SYSTEM_PROCESSOR})
message("FMI_PLATFORM: " ${FMI_PLATFORM})

set(CVODE_DIR    ${CMAKE_SOURCE_DIR}/build/cvode-${FMI_PLATFORM}/install/)
set(LIBXML2_DIR  ${CMAKE_SOURCE_DIR}/build/libxml2-${FMI_PLATFORM}/install/)
set(ZLIB_DIR     ${CMAKE_SOURCE_DIR}/build/zlib-${FMI_PLATFORM}/install/)
set(ZLIB_SRC_DIR ${CMAKE_SOURCE_DIR}/build/zlib-1.3/)

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
  fmusim.h
  fmusim.c
  fmusim_fmi1_cs.h
  fmusim_fmi1_cs.c
  fmusim_fmi1_me.h
  fmusim_fmi1_me.c
  fmusim_fmi2_cs.h
  fmusim_fmi2_cs.c
  fmusim_fmi2_me.h
  fmusim_fmi2_me.c
  fmusim_fmi3_cs.h
  fmusim_fmi3_cs.c
  fmusim_fmi3_me.h
  fmusim_fmi3_me.c
  fmusim_input.h
  fmusim_input.c
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

target_include_directories(fmusim PRIVATE
  .
  ../include
  ${LIBXML2_DIR}/include/libxml2
  ${ZLIB_DIR}/include
  ${ZLIB_SRC_DIR}/contrib/minizip
  ${CVODE_DIR}/include
)

if (WIN32)
    target_compile_definitions(fmusim PRIVATE YY_NO_UNISTD_H LIBXML_STATIC)
else ()
    target_compile_definitions(fmusim PRIVATE LIBXML_STATIC)
endif ()

if (WIN32)
    set(libraries
      ${LIBXML2_DIR}/lib/libxml2s.lib
      ${ZLIB_DIR}/lib/zlibstatic.lib
      ${CVODE_DIR}/lib/sundials_cvode.lib
      wsock32
      ws2_32
    )
elseif(UNIX AND NOT APPLE)
    set(libraries
      ${LIBXML2_DIR}/lib/libxml2.a
      ${ZLIB_DIR}lib/libz.a
      ${CVODE_DIR}/lib/libsundials_cvode.a
      ${CMAKE_DL_LIBS}
      m
    )
else ()
    set(libraries
      ${LIBXML2_DIR}/lib/libxml2.a
      ${ZLIB_DIR}lib/libz.a
      ${CVODE_DIR}/lib/libsundials_cvode.a
    )
endif ()

target_link_libraries(fmusim ${libraries})

install(TARGETS fmusim DESTINATION ${CMAKE_INSTALL_PREFIX})
