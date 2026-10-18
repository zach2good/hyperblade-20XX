# What every target of ours shares: warnings, source settings and the src/ include root.
add_library(hyperx_options INTERFACE)
target_include_directories(hyperx_options INTERFACE "${PROJECT_SOURCE_DIR}/src")

if(MSVC)
    # The decompiled unions name their members through anonymous structs, as windows.h let them,
    # and its classes delete through base pointers whose derived destructors are empty.
    target_compile_options(hyperx_options INTERFACE /W4 /WX /wd4201 /wd5205 /utf-8 /EHsc)
    target_compile_definitions(hyperx_options INTERFACE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    target_compile_options(hyperx_options INTERFACE -Wall -Werror -Wno-delete-non-virtual-dtor -finput-charset=UTF-8)
endif()

# The decompiled source is C++98 written against VC6, so MSVC's conformance checks are relaxed for it.
add_library(hyperx_permissive INTERFACE)
if(MSVC)
    target_compile_options(hyperx_permissive INTERFACE /permissive)
endif()

# SDL3.dll goes beside each Windows image. Elsewhere the build tree's rpath finds the library.
function(hyperx_copy_runtime_dlls target destination)
    if(WIN32)
        # A target with no DLLs runs cmake -E true, which takes the same arguments and does nothing.
        set(dlls $<TARGET_RUNTIME_DLLS:${target}>)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E $<IF:$<BOOL:${dlls}>,copy_if_different,true> ${dlls} "${destination}"
            COMMAND_EXPAND_LISTS
            VERBATIM)
    endif()
endfunction()

# A check exits 0 when it passes and this when it cannot run here, such as with no GPU.
set(HYPERX_CHECK_SKIPPED 77)

# hyperx_add_check(<name> SOURCES <files...> [LIBRARIES <targets...>] [SHADERS <hlsl...>]
#                  [ARGS <args...>] [LABELS <labels...>] [DEPENDS <targets...>] [PERMISSIVE] [DATA])
# builds <name>_check and registers it as test <name>. A DATA check reads the game's own files, so
# it is built always and registered only when HYPERX_GAME_DATA names them.
function(hyperx_add_check name)
    cmake_parse_arguments(PARSE_ARGV 1 check "PERMISSIVE;DATA" "" "SOURCES;LIBRARIES;SHADERS;ARGS;LABELS;DEPENDS")

    set(target ${name}_check)
    add_executable(${target} ${check_SOURCES})
    target_link_libraries(${target} PRIVATE hyperx_options ${check_LIBRARIES})

    if(check_PERMISSIVE)
        target_link_libraries(${target} PRIVATE hyperx_permissive)
    endif()

    if(check_SHADERS)
        hyperx_add_shaders(${target} ${check_SHADERS})
    endif()

    if(check_DEPENDS)
        add_dependencies(${target} ${check_DEPENDS})
    endif()

    hyperx_copy_runtime_dlls(${target} $<TARGET_FILE_DIR:${target}>)

    if(check_DATA AND NOT HYPERX_GAME_DATA)
        return()
    endif()

    add_test(NAME ${name} COMMAND ${target} ${check_ARGS})
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE ${HYPERX_CHECK_SKIPPED} LABELS "${check_LABELS}")
endfunction()
