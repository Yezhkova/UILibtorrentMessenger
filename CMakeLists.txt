cmake_minimum_required(VERSION 3.5)

project(UILibtorrentMessenger VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_compile_definitions(LIBTORRENT_MESSENGER_PET_PROJECT)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui

	DhtRequestHandler.hpp
	log.hpp
	SessionWrapperDelegate.hpp
	SessionWrapper.hpp
        StandaloneTest.cpp
        utils.h
        utils.cpp
)

include_directories(C:/Users/Admin/source/repos/QtProjects/libtorrentCopy/include)
find_package(Boost 1.79.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(UILibtorrentMessenger
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )


target_link_libraries(UILibtorrentMessenger PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(UILibtorrentMessenger PRIVATE C:/Users/Admin/source/repos/QtProjects/build-libtorrentCopy-Desktop_Qt_6_3_1_MSVC2019_64bit-Debug/torrent-rasterbar.lib )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET UILibtorrentMessenger APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(UILibtorrentMessenger SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(UILibtorrentMessenger
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(UILibtorrentMessenger PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(UILibtorrentMessenger PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(UILibtorrentMessenger)
endif()
