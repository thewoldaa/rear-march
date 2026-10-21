# SPDX-License-Identifier: GPL-3.0-or-later
# Pinned third-party dependencies. All are MIT licensed and GPL-3.0 compatible.
#
# Downloaded as release tarballs rather than git clones: no submodule setup for
# contributors, no dependence on the user's git ownership configuration, and
# the checksums below pin the exact bytes.
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# --- Dear ImGui -------------------------------------------------------------
FetchContent_Declare(imgui
    URL      https://github.com/ocornut/imgui/archive/refs/tags/v1.93.0.tar.gz
    URL_HASH SHA256=f7bf6bc76e5d79d40d751096663e6c5d18daf08129dd52e7f7b09e4251c8eeee
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui_lib PUBLIC
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    IMGUI_DEFINE_MATH_OPERATORS
)
target_link_libraries(imgui_lib PUBLIC d3d11 dxgi)
target_compile_features(imgui_lib PUBLIC cxx_std_17)

# ImGui must use the same CRT as the rest of the project. Mixing /MT and /MD
# produces unresolved __imp_* symbols at link time.
if(REARMARCH_STATIC_RUNTIME)
    set_property(TARGET imgui_lib PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- nlohmann/json ----------------------------------------------------------
FetchContent_Declare(nlohmann_json
    URL      https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz
    URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)
