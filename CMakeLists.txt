cmake_minimum_required(VERSION 3.21)
project(MultiMonitorTools LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project only builds on Windows.")
endif()

if(NOT (MSVC AND CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v143"))
    message(FATAL_ERROR "This project requires MSVC 2022 (v143).")
endif()

if(CMAKE_SYSTEM_VERSION VERSION_LESS "10.0.22621.0")
    message(FATAL_ERROR "Windows SDK too old: ${CMAKE_SYSTEM_VERSION}. Need at least 10.0.22621.0")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(downloadQt)

set(QT_VERSION "6.9.1")

if(Qt6_DIR)
    message(STATUS "Qt6_DIR is set to: ${Qt6_DIR}")
else()
    message(STATUS "Qt6_DIR is NOT set → downloading with aqt...")

     # Download Qt
    download_qt(
    ${QT_VERSION}
    ""
    "windows"
    "win64_msvc2022_64"
    "${CMAKE_BINARY_DIR}/Qt"
	)

    # Append the downloaded Qt to CMAKE_PREFIX_PATH
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/Qt/${QT_VERSION}/msvc2022_64")
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE PATH "Prefix path for CMake packages" FORCE)
endif()

# Find Qt
find_package(Qt6 6.8.0 REQUIRED COMPONENTS Widgets Core Gui Concurrent)

# Enable automatic handling of Qt stuff
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/_autogen")
set(CMAKE_AUTOGEN_TARGETS_DIR "${CMAKE_BINARY_DIR}/_autogen")
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTORCC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOUIC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Generated Files")

# --- Sources ---
set(SOURCES
    src/main.cpp
    src/MMT.cpp
    src/MMTCursorAdjust.cpp
    src/MMTKeyboardHook.cpp
    src/MMTMainWindow.cpp
    src/MMTMonitorLayoutEditor.cpp
    src/MMTScreenOverlay.cpp
    src/MMTVirtualDesktop.cpp
    src/MMTMonitorManager.cpp
    src/MMTMonitorConfigWidget.cpp
    src/MMTSettings.cpp
	src/MMTHotkeys.cpp
	src/MMTHotkeyEditor.cpp
	src/MMTTrayIconManager.cpp
)

# --- Headers ---
set(HEADERS
    src/MMT.hpp
    src/MMTVirtualDesktop.hpp
    src/MMTSettings.hpp
	src/MMTCursorAdjust.hpp
    src/MMTKeyboardHook.hpp
    src/MMTMainWindow.hpp
    src/MMTScreenOverlay.hpp
    src/MMTMonitorLayoutEditor.hpp
    src/MMTMonitorManager.hpp
    src/MMTMonitorConfigWidget.hpp
    src/MMTHotkeys.hpp
	src/MMTHotkeyEditor.hpp
	src/MMTTrayIconManager.hpp
)

# --- VirtualDesktopAPI ---
set(VIRTUALDESKTOPAPI
	src/VirtualDesktopInterface.hpp
    src/VirtualDesktopGlobals.hpp
    src/VirtualDesktop_Win11_23.hpp
    src/VirtualDesktop_Win11_23H2.hpp
	src/VirtualDesktopInterface.cpp
)

# --- Resources ---
set(RESOURCES
    src/MMT.rc
)
qt_add_resources(RESOURCES src/MultiMonitorTools.qrc)

# Define executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${VIRTUALDESKTOPAPI}
    ${RESOURCES}
)
add_compile_definitions(UNICODE _UNICODE)

# Link Qt
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Concurrent)

# Link extra Windows system libraries
target_link_libraries(${PROJECT_NAME} PRIVATE SetupAPI WindowsApp)

# Mark it as a GUI app (Windows subsystem, no console)
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
)

# --- Organize files in Visual Studio Solution Explorer ---
source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})
source_group("VirtualDesktopWinApi" FILES ${VIRTUALDESKTOPAPI})
source_group("Resources" FILES ${RESOURCES})

# Put the target into a solution folder
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Apps")

#--- Qt deployment (windeployqt) ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
            --no-translations
            --no-compiler-runtime
			--no-opengl-sw
			--no-system-dxc-compiler
			--no-system-d3d-compiler
			--no-network
            "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Deploying Qt runtime..."
)
