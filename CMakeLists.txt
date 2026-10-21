# SPDX-License-Identifier: GPL-3.0-or-later
# CMake build for RearMarch. See docs/BUILDING.md for prerequisites.

cmake_minimum_required(VERSION 3.24)

project(RearMarch
    VERSION 0.1.0
    DESCRIPTION "One key turns your game camera 180 degrees"
    HOMEPAGE_URL "https://github.com/thewoldaa/rear-march"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "RearMarch currently targets Windows only.")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REARMARCH_BUILD_TESTS "Build the unit test suite" ON)
option(REARMARCH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(REARMARCH_STATIC_RUNTIME "Link the MSVC runtime statically (/MT)" ON)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

# ---------------------------------------------------------------------------
# rearmarch_core -- platform independent. Must not include <windows.h>.
# ---------------------------------------------------------------------------
add_library(rearmarch_core STATIC
    src/core/chord.cpp
    src/core/config.cpp
    src/core/i18n.cpp
    src/core/keycodes.cpp
    src/core/runtime_config.cpp
    src/core/turn.cpp
)

target_include_directories(rearmarch_core PUBLIC src)
target_link_libraries(rearmarch_core PUBLIC nlohmann_json::nlohmann_json)
rearmarch_set_warnings(rearmarch_core)

# Single source for the version string used by the dashboard and --version.
target_compile_definitions(rearmarch_core PUBLIC
    REARMARCH_VERSION="${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# rearmarch_win32 -- input backend and platform services.
# ---------------------------------------------------------------------------
add_library(rearmarch_win32 STATIC
    src/input/backend_win32.cpp
    src/input/injection.cpp
    src/input/keycodes_win32.cpp
    src/input/turn_worker.cpp
    src/platform/beep_win32.cpp
    src/platform/dpi_win32.cpp
    src/platform/locale_win32.cpp
    src/platform/mouse_state_win32.cpp
    src/platform/paths_win32.cpp
    src/platform/session_win32.cpp
    src/platform/startup_win32.cpp
    src/platform/timer_win32.cpp
)

target_include_directories(rearmarch_win32 PUBLIC src)
target_link_libraries(rearmarch_win32 PUBLIC rearmarch_core)
target_compile_definitions(rearmarch_win32 PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(rearmarch_win32 PUBLIC
    user32
    shell32
    advapi32
    ole32
    dwmapi
    wtsapi32
    winmm
)
rearmarch_set_warnings(rearmarch_win32)

# ---------------------------------------------------------------------------
# rearmarch_ui -- Dear ImGui dashboard.
# ---------------------------------------------------------------------------
add_library(rearmarch_ui STATIC
    src/ui/capture.cpp
    src/ui/dashboard.cpp
    src/ui/fonts.cpp
    src/ui/theme.cpp
    src/ui/tray.cpp
)

target_include_directories(rearmarch_ui PUBLIC src)
target_link_libraries(rearmarch_ui PUBLIC rearmarch_core imgui_lib)
target_link_libraries(rearmarch_ui PRIVATE rearmarch_win32)
rearmarch_set_warnings(rearmarch_ui)

# ---------------------------------------------------------------------------
# rearmarch -- the executable.
# ---------------------------------------------------------------------------
add_executable(rearmarch
    src/main.cpp
    src/app/application.cpp
    src/app/cli.cpp
    src/app/single_instance.cpp
)

target_link_libraries(rearmarch PRIVATE rearmarch_ui rearmarch_win32 rearmarch_core)
rearmarch_set_warnings(rearmarch)

set_target_properties(rearmarch PROPERTIES
    OUTPUT_NAME "RearMarch"
    WIN32_EXECUTABLE ON
)

# Keep the translation files beside the binary so a plain build tree behaves
# like an installed one.
add_custom_command(TARGET rearmarch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/locales"
        "$<TARGET_FILE_DIR:rearmarch>/locales"
    COMMENT "Copying locales next to the executable")

# Per-monitor DPI awareness and no elevation. The manifest is embedded rather
# than shipped as a sidecar so the portable zip is a single exe.
set(REARMARCH_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/resources/rearmarch.manifest")
if(EXISTS "${REARMARCH_MANIFEST}")
    target_sources(rearmarch PRIVATE "${REARMARCH_MANIFEST}")
    set_target_properties(rearmarch PROPERTIES
        VS_DPI_AWARE "PerMonitor")
endif()

# Application icon and version block, generated from a template so the version
# shown in the exe's properties cannot drift from project(VERSION ...).
if(MSVC)
    set(REARMARCH_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/rearmarch.ico")
    if(EXISTS "${REARMARCH_ICON_PATH}")
        string(REPLACE "." ","
            REARMARCH_VERSION_COMMA "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/rearmarch.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/rearmarch.rc"
            @ONLY)
        target_sources(rearmarch PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/rearmarch.rc")
    else()
        message(WARNING
            "resources/rearmarch.ico is missing; the build will have no icon. "
            "Regenerate it with: powershell -File tools/make_icon.ps1")
    endif()
endif()

if(REARMARCH_STATIC_RUNTIME)
    foreach(tgt rearmarch rearmarch_core rearmarch_win32 rearmarch_ui)
        set_property(TARGET ${tgt} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Install rules and packaging
# ---------------------------------------------------------------------------
install(TARGETS rearmarch RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/locales/" DESTINATION locales
        FILES_MATCHING PATTERN "*.json")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/" DESTINATION docs)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION .)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "RearMarch")
set(CPACK_PACKAGE_VENDOR "RearMarch contributors")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_FILE_NAME "RearMarch-${PROJECT_VERSION}-win64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_VERBATIM_VARIABLES ON)
include(CPack)

if(REARMARCH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
