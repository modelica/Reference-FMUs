cmake_minimum_required (VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#set(FMI_VERSION 2 CACHE STRING "FMI Version")
#set_property(CACHE FMI_VERSION PROPERTY STRINGS 2 3)

set (MODEL_IDENTIFIER BouncingBall CACHE STRING "Model Identifier")
set (INCLUDE "E:/Development/Reference-FMUs/include" "E:/Development/Reference-FMUs/build/fmi2-x86_64-windows/fmus/BouncingBall/sources" CACHE STRING "Include directories")
set (SOURCES "E:/Development/Reference-FMUs/build/fmi2-x86_64-windows/fmus/BouncingBall/sources/all.c" CACHE STRING "Source files")
set (UNZIPDIR "E:/Development/Reference-FMUs/build/fmi2-x86_64-windows/fmus/BouncingBall" CACHE STRING "Unzip directory")

project (${MODEL_IDENTIFIER})

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

add_library(${MODEL_IDENTIFIER} SHARED ${SOURCES})

target_include_directories(${MODEL_IDENTIFIER} PUBLIC ${INCLUDE})

set_target_properties(${MODEL_IDENTIFIER} PROPERTIES PREFIX "")

install(TARGETS ${MODEL_IDENTIFIER} DESTINATION "${UNZIPDIR}/binaries/${FMI_PLATFORM}")
install(FILES $<TARGET_PDB_FILE:${MODEL_IDENTIFIER}> DESTINATION "${UNZIPDIR}/binaries/${FMI_PLATFORM}" OPTIONAL)
