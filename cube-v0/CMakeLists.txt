# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.25)

project(cube VERSION 1.0 LANGUAGES CXX)

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    OpenGL
    OpenGLWidgets
    Widgets
)

qt_standard_project_setup()

qt_add_executable(cube
    geometryengine.cpp
    geometryengine.h
    main.cpp
    mainwidget.cpp
    mainwidget.h
)

target_compile_features(cube PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(cube PRIVATE /W4 /permissive-)
endif()

target_link_libraries(cube PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Widgets
)

qt_add_resources(cube "shaders"
    PREFIX "/"
    FILES
        fshader.glsl
        vshader.glsl
)

qt_add_resources(cube "textures"
    PREFIX "/"
    FILES
        cube.png
)

set_target_properties(cube PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS cube
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET cube
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
