cmake_host_system_information(RESULT TARGET_64_BITS QUERY IS_64BIT)

include(ExternalProject)
set(EXTERNAL_BASE_DIR ${CMAKE_BINARY_DIR}/_deps CACHE STRING "External base directory")
file(MAKE_DIRECTORY ${EXTERNAL_BASE_DIR}/lib)
if(UNIX AND TARGET_64_BITS GREATER_EQUAL 1)
    file(CREATE_LINK lib ${EXTERNAL_BASE_DIR}/lib64 SYMBOLIC)
endif()

ExternalProject_Add(
    zlib_src
    PREFIX ${EXTERNAL_BASE_DIR}
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf # v1.3.1
    GIT_SHALLOW True
    UPDATE_DISCONNECTED True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BASE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
    # for zlib version v1.3.2+ add "-DZLIB_BUILD_MINIZIP=1"
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release
    BUILD_BYPRODUCTS ${EXTERNAL_BASE_DIR}/lib/libz.a
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --config Release
)
add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES IMPORTED_LOCATION ${EXTERNAL_BASE_DIR}/lib/libz.a)
add_dependencies(zlib zlib_src)

set(ZLIB_SRC_DIR ${EXTERNAL_BASE_DIR}/src/zlib_src)

ExternalProject_Add(
    xml2_src
    DEPENDS zlib
    PREFIX ${EXTERNAL_BASE_DIR}
    GIT_REPOSITORY https://github.com/GNOME/libxml2.git
    GIT_TAG 60d3056c97067e6cb2125284878ed7c99c90ed81 # v2.13.4
    GIT_SHALLOW True
    UPDATE_DISCONNECTED True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BASE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -DBUILD_SHARED_LIBS=OFF -DLIBXML2_WITH_ICONV=OFF -DLIBXML2_WITH_LZMA=OFF -DLIBXML2_WITH_PYTHON=OFF -DLIBXML2_WITH_ZLIB=OFF -DLIBXML2_WITH_TESTS=OFF
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release
    BUILD_BYPRODUCTS ${EXTERNAL_BASE_DIR}/lib/libxml2.a
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --config Release
)
add_library(xml2 STATIC IMPORTED)
set_target_properties(xml2 PROPERTIES IMPORTED_LOCATION ${EXTERNAL_BASE_DIR}/lib/libxml2.a)
add_dependencies(xml2 xml2_src)

ExternalProject_Add(
    cvcode_src
    PREFIX ${EXTERNAL_BASE_DIR}
    GIT_REPOSITORY https://github.com/LLNL/sundials.git
    GIT_TAG c28eaa3764a03705d61decb6025b409360e9d53f # v7.1.1
    GIT_SHALLOW True
    UPDATE_DISCONNECTED True
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BASE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DEXAMPLES_INSTALL=OFF -DSUNDIALS_ENABLE_ERROR_CHECKS=OFF
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release
    BUILD_BYPRODUCTS ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --config Release
)
add_library(cvcode STATIC IMPORTED)
set_target_properties(cvcode PROPERTIES IMPORTED_LOCATION ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a)
add_dependencies(cvcode cvcode_src)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

if(WIN32)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-windows")
elseif(APPLE)
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-darwin")
else()
    set(FMI_PLATFORM "${FMI_ARCHITECTURE}-linux")
endif()

set(FMUSIM_SOURCES
    ../include/FMI.h
    ../src/FMI.c
    ../include/FMI1.h
    ../src/FMI1.c
    ../include/FMI2.h
    ../src/FMI2.c
    ../include/FMI3.h
    ../src/FMI3.c
    ../fmusim/FMIUtil.h
    ../fmusim/FMIUtil.c
    ../fmusim/FMIStaticInput.h
    ../fmusim/FMIStaticInput.c
    ../fmusim/FMISimulation.h
    ../fmusim/FMISimulation.c
    ../fmusim/FMI1CSSimulation.h
    ../fmusim/FMI1CSSimulation.c
    ../fmusim/FMI1MESimulation.h
    ../fmusim/FMI1MESimulation.c
    ../fmusim/FMI2CSSimulation.h
    ../fmusim/FMI2CSSimulation.c
    ../fmusim/FMI2MESimulation.h
    ../fmusim/FMI2MESimulation.c
    ../fmusim/FMI3CSSimulation.h
    ../fmusim/FMI3CSSimulation.c
    ../fmusim/FMI3MESimulation.h
    ../fmusim/FMI3MESimulation.c
    ../fmusim/FMIRecorder.h
    ../fmusim/FMIRecorder.c
    ../fmusim/FMIEuler.h
    ../fmusim/FMIEuler.c
    ../fmusim/FMICVode.h
    ../fmusim/FMICVode.c
    ../fmusim/csv.h
    ../fmusim/csv.c
    ../fmusim/FMIZip.h
    ../fmusim/FMIZip.c
    ../fmusim/miniunzip.c
    ../fmusim/FMIModelDescription.c
    ../fmusim/FMIModelDescription.h
    ../fmusim/FMIBuildDescription.c
    ../fmusim/FMIBuildDescription.h
    ../include/structured_variable_name.tab.h
    ../src/structured_variable_name.tab.c
    ../src/structured_variable_name.yy.c
    ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
    ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
    )
if(WIN32)
    set(FMUSIM_SOURCES ${FMUSIM_SOURCES} ${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c)
endif()

set(FMUSIM_INCLUDES
    ../fmusim
    ../include
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
