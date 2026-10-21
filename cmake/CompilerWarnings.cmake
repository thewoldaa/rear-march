# SPDX-License-Identifier: GPL-3.0-or-later
# Shared warning configuration for all first-party targets.
function(rearmarch_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /Zc:preprocessor
            /utf-8
            /MP
            /wd4100  # unreferenced formal parameter (interface stubs)
        )
        if(REARMARCH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
            target_compile_options(${target} PRIVATE /O2 /GL)
            target_link_options(${target} PRIVATE /LTCG /OPT:REF /OPT:ICF)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(REARMARCH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
