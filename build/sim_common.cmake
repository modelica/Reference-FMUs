set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

if(WIN32)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-windows")
elseif(APPLE)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-darwin")
else()
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-linux")
endif()

set_source_files_properties(
    ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
    ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
    ${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c
    PROPERTIES GENERATED 1)

set(FMUSIM_SOURCES
    ${CMAKE_SOURCE_DIR}/include/FMI.h
    ${CMAKE_SOURCE_DIR}/src/FMI.c
    ${CMAKE_SOURCE_DIR}/include/FMI1.h
    ${CMAKE_SOURCE_DIR}/src/FMI1.c
    ${CMAKE_SOURCE_DIR}/include/FMI2.h
    ${CMAKE_SOURCE_DIR}/src/FMI2.c
    ${CMAKE_SOURCE_DIR}/include/FMI3.h
    ${CMAKE_SOURCE_DIR}/src/FMI3.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIUtil.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMIUtil.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIStaticInput.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMIStaticInput.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMISimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMISimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMI1CSSimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMI1CSSimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMI1MESimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMI1MESimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMI2CSSimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMI2CSSimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMI2MESimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMI2MESimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMI3CSSimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMI3CSSimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMI3MESimulation.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMI3MESimulation.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIRecorder.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMIRecorder.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIEuler.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMIEuler.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMICVode.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMICVode.c
    ${CMAKE_SOURCE_DIR}/fmusim/csv.h
    ${CMAKE_SOURCE_DIR}/fmusim/csv.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIZip.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMIZip.c
    ${CMAKE_SOURCE_DIR}/fmusim/miniunzip.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIModelDescription.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIModelDescription.h
    ${CMAKE_SOURCE_DIR}/fmusim/FMIBuildDescription.c
    ${CMAKE_SOURCE_DIR}/fmusim/FMIBuildDescription.h
    ${CMAKE_SOURCE_DIR}/include/structured_variable_name.tab.h
    ${CMAKE_SOURCE_DIR}/src/structured_variable_name.tab.c
    ${CMAKE_SOURCE_DIR}/src/structured_variable_name.yy.c
    ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
    ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
    )
if(WIN32)
    set(FMUSIM_SOURCES ${FMUSIM_SOURCES} ${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c)
endif()

set(FMUSIM_INCLUDES
    ${CMAKE_SOURCE_DIR}/fmusim
    ${CMAKE_SOURCE_DIR}/include
    ${EXTERNAL_BASE_DIR}/include
    ${EXTERNAL_BASE_DIR}/include/libxml2
    ${ZLIB_SRC_DIR}/contrib/minizip
    )

if(WIN32)
    set(FMUSIM_LIBS
        ${EXTERNAL_BASE_DIR}/lib/libxml2s.lib
        ${EXTERNAL_BASE_DIR}/lib/zlibstatic.lib
        ${EXTERNAL_BASE_DIR}/lib/sundials_cvode_static.lib
        ${EXTERNAL_BASE_DIR}/lib/sundials_core_static.lib
        )
elseif(UNIX AND NOT APPLE)
    set(FMUSIM_LIBS
        ${EXTERNAL_BASE_DIR}/lib/libxml2.a
        ${EXTERNAL_BASE_DIR}/lib/libz.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
        ${CMAKE_DL_LIBS}
        m
        )
else()
    set(FMUSIM_LIBS
        ${EXTERNAL_BASE_DIR}/lib/libxml2.a
        ${EXTERNAL_BASE_DIR}/lib/libz.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
        )
endif()

set(FMUSIM_DEPENDS zlib xml2 cvcode)
