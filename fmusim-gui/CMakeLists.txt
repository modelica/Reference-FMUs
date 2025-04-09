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

get_directory_property(NestedProject PARENT_DIRECTORY)
if(NOT NestedProject)
    include(../build/external.cmake)
endif()
include(../build/sim_common.cmake)

set(FMUSIM_GUI_SOURCES
    ${FMUSIM_SOURCES}
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(fmusim-gui
        MANUAL_FINALIZATION
        ${FMUSIM_GUI_SOURCES}

        ModelVariablesTableModel.h
        ModelVariablesTableModel.cpp

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
    add_dependencies(fmusim-gui ${FMUSIM_DEPENDS})

    target_include_directories(fmusim-gui PRIVATE ${FMUSIM_INCLUDES})

    if(WIN32)
        target_compile_definitions(fmusim-gui PRIVATE
            YY_NO_UNISTD_H
        )
    endif()

    target_link_libraries(fmusim-gui PRIVATE
        Qt6::WebEngineWidgets
        ${FMUSIM_LIBS}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET fmusim-gui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(fmusim-gui SHARED
            ${FMUSIM_GUI_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(fmusim-gui
            ${FMUSIM_GUI_SOURCES}
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
