cmake_minimum_required (VERSION 3.17)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

project (FMUSimulator)

add_executable(fmusim
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
  ../ThirdParty/zlib-1.2.13/contrib/minizip/iowin32.c
  ../ThirdParty/zlib-1.2.13/contrib/minizip/unzip.c
)

target_include_directories(fmusim PRIVATE
  .
  ../include
  ../ThirdParty/libxml2-2.10.3/windows-x64/install/include/libxml2
  ../ThirdParty/zlib-1.2.13/windows-x64/install/include
  ../ThirdParty/zlib-1.2.13/contrib/minizip
)

add_compile_definitions(LIBXML_STATIC)

target_link_libraries(fmusim
  ${CMAKE_SOURCE_DIR}/../ThirdParty/libxml2-2.10.3/windows-x64/install/lib/libxml2s.lib
  ${CMAKE_SOURCE_DIR}/../ThirdParty/zlib-1.2.13/windows-x64/install/lib/zlibstatic.lib
  wsock32
  ws2_32
)

install(TARGETS fmusim RUNTIME DESTINATION .)
