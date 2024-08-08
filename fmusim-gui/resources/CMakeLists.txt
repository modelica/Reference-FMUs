cmake_minimum_required (VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(FMI_VERSION 2 CACHE STRING "FMI Version")
set_property(CACHE FMI_VERSION PROPERTY STRINGS 2 3)

set (MODEL_IDENTIFIER CACHE STRING "Model Identifier")
set (INCLUDE CACHE STRING "Include directories")
set (SOURCES CACHE STRING "Source files")
set (UNZIPDIR CACHE STRING "Unzip directory")

project (${MODEL_IDENTIFIER})

#message(STATUS "MODEL_IDENTIFIER: ${MODEL_IDENTIFIER}")
#message(STATUS "INCLUDE: ${INCLUDE}")
#message(STATUS "SOURCES: ${SOURCES}")
#message(STATUS "UNZIPDIR: ${UNZIPDIR}")

if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "AMD64|x86_64")
  set(FMI_ARCHITECTURE "x86_64")
elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(FMI_ARCHITECTURE "aarch64")
else ()
  message(FATAL_ERROR "Unknown system architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

if (${FMI_VERSION} GREATER 2)

  if (WIN32)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-windows")
  elseif (APPLE)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-darwin")
  else ()
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-linux")
  endif ()

else ()

  if (WIN32)
    set(FMI_PLATFORM win)
  elseif (APPLE)
    set(FMI_PLATFORM darwin)
  else ()
    set(FMI_PLATFORM linux)
  endif ()

  if ("${FMI_ARCHITECTURE}" STREQUAL "x86_64")
    set (FMI_PLATFORM ${FMI_PLATFORM}64)
  elseif ("${FMI_ARCHITECTURE}" STREQUAL "x86")
    set (FMI_PLATFORM ${FMI_PLATFORM}32)
  else ()
    message(FATAL_ERROR "Unsupported architecture for FMI version < 3.0: ${FMI_ARCHITECTURE}")
  endif ()

endif ()

add_library(${MODEL_IDENTIFIER} SHARED ${SOURCES})

target_include_directories(${MODEL_IDENTIFIER} PUBLIC ${INCLUDE})

set_target_properties(${MODEL_IDENTIFIER} PROPERTIES PREFIX "")

install(TARGETS ${MODEL_IDENTIFIER} DESTINATION "${UNZIPDIR}/binaries/${FMI_PLATFORM}")

if (MSVC)
    install(FILES $<TARGET_PDB_FILE:${MODEL_IDENTIFIER}> DESTINATION "${UNZIPDIR}/binaries/${FMI_PLATFORM}" OPTIONAL)
endif ()
