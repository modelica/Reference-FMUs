cmake_minimum_required (VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

project (FMUSimulator)

set(sources
  ../include/FMI.h
  ../src/FMI.c
  ../include/FMI2.h
  ../src/FMI2.c
  ../include/FMI3.h
  ../src/FMI3.c
  csv.h
  csv.c
  FMIModelDescription.h
  FMIModelDescription.c
  FMISimulationResult.h
  FMISimulationResult.c
  FMIZip.h
  FMIZip.c
  fmusim.c
  fmusim_fmi2.h
  fmusim_fmi2.c
  fmusim_fmi2_cs.h
  fmusim_fmi2_cs.c
  fmusim_fmi2_me.h
  fmusim_fmi2_me.c
  fmusim_fmi3.h
  fmusim_fmi3.c
  fmusim_fmi3_cs.h
  fmusim_fmi3_cs.c
  fmusim_fmi3_me.h
  fmusim_fmi3_me.c
  fmusim_input.h
  fmusim_input.c
  miniunzip.c
  ../ThirdParty/zlib-1.2.13/contrib/minizip/ioapi.c
  ../ThirdParty/zlib-1.2.13/contrib/minizip/unzip.c
)

if (WIN32)
    set (sources ${sources} ../ThirdParty/zlib-1.2.13/contrib/minizip/iowin32.c)
endif ()

add_executable(fmusim ${sources})

target_include_directories(fmusim PRIVATE
  .
  ../include
  ../ThirdParty/libxml2-2.10.3/build/install/include/libxml2
  ../ThirdParty/zlib-1.2.13/build/install/include
  ../ThirdParty/zlib-1.2.13/contrib/minizip
)

add_compile_definitions(LIBXML_STATIC)

if (WIN32)
    set(libraries
      ${CMAKE_SOURCE_DIR}/../ThirdParty/libxml2-2.10.3/build/install/lib/libxml2s.lib
      ${CMAKE_SOURCE_DIR}/../ThirdParty/zlib-1.2.13/build/install/lib/zlibstatic.lib
      wsock32
      ws2_32
    )
elseif(UNIX AND NOT APPLE)
    set(libraries
      ${CMAKE_SOURCE_DIR}/../ThirdParty/libxml2-2.10.3/build/install/lib/libxml2.a
      ${CMAKE_SOURCE_DIR}/../ThirdParty/zlib-1.2.13/build/install/lib/libz.a
      ${CMAKE_DL_LIBS}
      m
    )
else ()
    set(libraries
      ${CMAKE_SOURCE_DIR}/../ThirdParty/libxml2-2.10.3/build/install/lib/libxml2.a
      ${CMAKE_SOURCE_DIR}/../ThirdParty/zlib-1.2.13/build/install/lib/libz.a
    )
endif ()

target_link_libraries(fmusim ${libraries})

install(TARGETS fmusim RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install)
