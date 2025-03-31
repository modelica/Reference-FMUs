cmake_minimum_required(VERSION 3.18)

project(fmusim-gui VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS WebEngineWidgets)
endif()

set_source_files_properties(
    ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
    ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
    ${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c
    PROPERTIES GENERATED 1)

set(PROJECT_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    ../src/FMI.c
    ../include/FMI.h
    ../fmusim/FMIZip.c
    ../fmusim/FMIZip.h
    ../fmusim/miniunzip.c
    ${ZLIB_SRC_DIR}/contrib/minizip/unzip.c
    ${ZLIB_SRC_DIR}/contrib/minizip/ioapi.c
    ../fmusim/FMIModelDescription.c
    ../fmusim/FMIModelDescription.h
    ../fmusim/FMIBuildDescription.c
    ../fmusim/FMIBuildDescription.h
    ../src/structured_variable_name.tab.c
    ../src/structured_variable_name.yy.c
)
if (WIN32)
    set (PROJECT_SOURCES ${PROJECT_SOURCES} ${ZLIB_SRC_DIR}/contrib/minizip/iowin32.c)
endif ()

if(WIN32)
    set(PROJECT_LIBRARIES
        ${EXTERNAL_BASE_DIR}/lib/libxml2s.lib
        ${EXTERNAL_BASE_DIR}/lib/zlibstatic.lib
        ${EXTERNAL_BASE_DIR}/lib/sundials_cvode_static.lib
        ${EXTERNAL_BASE_DIR}/lib/sundials_core_static.lib
        )
elseif(UNIX AND NOT APPLE)
    set(PROJECT_LIBRARIES
        ${EXTERNAL_BASE_DIR}/lib/libxml2.a
        ${EXTERNAL_BASE_DIR}/lib/libz.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
        ${CMAKE_DL_LIBS}
        m
        )
else()
    set(PROJECT_LIBRARIES
        ${EXTERNAL_BASE_DIR}/lib/libxml2.a
        ${EXTERNAL_BASE_DIR}/lib/libz.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_cvode.a
        ${EXTERNAL_BASE_DIR}/lib/libsundials_core.a
        )
endif()


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(fmusim-gui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ModelVariablesTableModel.h
        ModelVariablesTableModel.cpp
        ../src/FMI1.c
        ../src/FMI2.c
        ../include/FMI1.h
        ../include/FMI2.h
        ../src/FMI3.c
        ../include/FMI3.h
        ../fmusim/FMIUtil.c
        ../fmusim/FMIUtil.h
        ../fmusim/csv.c
        ../fmusim/csv.h

        ../fmusim/FMIStaticInput.h
        ../fmusim/FMIStaticInput.c

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

        ../fmusim/FMISimulation.h
        ../fmusim/FMISimulation.c

        ../fmusim/FMIEuler.h
        ../fmusim/FMIEuler.c

        ../fmusim/FMICVode.h
        ../fmusim/FMICVode.c

        resources/buttons/buttons.qrc
        resources/tools/tools.qrc

        resources/plot/plot.html
        resources/plot/plot.qrc

        resources/variables/variables.qrc

        VariablesFilterModel.h VariablesFilterModel.cpp
        SimulationThread.h SimulationThread.cpp
        BuildPlatformBinaryDialog.h BuildPlatformBinaryDialog.cpp BuildPlatformBinaryDialog.ui
        BuildPlatformBinaryThread.h BuildPlatformBinaryThread.cpp

        ModelVariablesTreeModel.h ModelVariablesTreeModel.cpp
        AbstractModelVariablesModel.h AbstractModelVariablesModel.cpp
        PlotUtil.h PlotUtil.cpp

        resources/icon/icon.qrc
        resources/icon/icon.rc
        resources/build/build.qrc
    )

    add_dependencies(fmusim-gui zlib xml2 cvcode)

    target_include_directories(fmusim-gui PRIVATE
        ../include
        ../fmusim
        ${EXTERNAL_BASE_DIR}/include
        ${EXTERNAL_BASE_DIR}/include/libxml2
        ${ZLIB_SRC_DIR}/contrib/minizip
        ${ZLIB_SRC_DIR}/contrib/minizip
    )

    if(WIN32)
        target_compile_definitions(fmusim-gui PRIVATE
            YY_NO_UNISTD_H
        )
    endif()

    target_link_libraries(fmusim-gui PRIVATE
        Qt6::WebEngineWidgets
        ${PROJECT_LIBRARIES}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET fmusim-gui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(fmusim-gui SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(fmusim-gui
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(fmusim-gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.fmusim-gui)
endif()
set_target_properties(fmusim-gui PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS fmusim-gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(fmusim-gui)
endif()
