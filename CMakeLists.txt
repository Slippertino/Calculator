cmake_minimum_required(VERSION 3.25)

project(Calculator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(windeployqt target)
    string(TOLOWER ${CMAKE_BUILD_TYPE} buildtype)

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${_qt5Core_install_prefix}/bin/windeployqt.exe"         
                --verbose 1
                --${buildtype}
                --no-svg
                --no-angle
                --no-opengl
                --no-opengl-sw
                --no-compiler-runtime
                --no-system-d3d-compiler
                \"$<TARGET_FILE:${target}>\"
        COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}' ..."
    )
endfunction()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)

file(GLOB SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/model/*
    ${CMAKE_CURRENT_LIST_DIR}/model/mpreal/*
    ${CMAKE_CURRENT_LIST_DIR}/presenter/*
    ${CMAKE_CURRENT_LIST_DIR}/view/*
)
qt5_create_translation(QM_FILES
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/resources/${PROJECT_NAME}_ru.ts
    ${CMAKE_CURRENT_LIST_DIR}/resources/${PROJECT_NAME}_en.ts
)
set(PROJECT_SOURCES
        main.cpp
        ${SOURCES}
        ${QM_FILES}
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Calculator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Calculator APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Calculator SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Calculator
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(Calculator PRIVATE ${gmpxx_INCLUDE_DIRS})
target_link_libraries(Calculator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets PkgConfig::mpfr)
target_compile_definitions(Calculator PRIVATE TRANSLATION_PREFIX="${PROJECT_NAME}")
set_target_properties(Calculator PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(WIN32)
    WINDEPLOYQT(Calculator)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Calculator)
endif()
