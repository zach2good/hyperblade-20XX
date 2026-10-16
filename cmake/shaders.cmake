# hyperx_add_shaders(<target> <hlsl files...>) compiles each shader to DXIL and SPIR-V and embeds
# the bytecode in a header. The stage comes from the name: foo.vert.hlsl or foo.frag.hlsl, entry
# point main. Include "foo.vert.dxil.h" or "foo.vert.spv.h" for foo_vert_dxil or foo_vert_spv.

# Run with cmake -P, this file writes INPUT as a constexpr byte array called SYMBOL into OUTPUT.
if(CMAKE_SCRIPT_MODE_FILE)
    file(READ "${INPUT}" hex HEX)
    string(REGEX REPLACE "(..)" "0x\\1," bytes "${hex}")
    string(REPEAT "0x..," 16 line)
    string(REGEX REPLACE "(${line})" "\\1\n    " bytes "${bytes}")
    file(WRITE "${OUTPUT}" "#pragma once\n\ninline constexpr unsigned char ${SYMBOL}[] = {\n    ${bytes}\n};\n")
    return()
endif()

set(HYPERX_SHADERS_SCRIPT "${CMAKE_CURRENT_LIST_FILE}")

function(hyperx_add_shaders target)
    set(out_dir "${CMAKE_BINARY_DIR}/generated/shaders")
    file(MAKE_DIRECTORY "${out_dir}")

    foreach(hlsl IN LISTS ARGN)
        get_filename_component(hlsl "${hlsl}" ABSOLUTE)
        get_filename_component(name "${hlsl}" NAME_WLE)
        get_filename_component(stage "${name}" LAST_EXT)

        if(stage STREQUAL ".vert")
            set(profile vs_6_0)
        elseif(stage STREQUAL ".frag")
            set(profile ps_6_0)
        else()
            message(FATAL_ERROR "${hlsl}: name must end in .vert.hlsl or .frag.hlsl")
        endif()

        string(MAKE_C_IDENTIFIER "${name}" symbol)

        # Register spaces already match SDL's descriptor sets, so SPIR-V needs no binding shifts.
        foreach(format dxil spv)
            set(format_flags)
            if(format STREQUAL "spv")
                set(format_flags -spirv)
            endif()

            set(binary "${out_dir}/${name}.${format}")
            set(header "${binary}.h")

            add_custom_command(
                OUTPUT "${binary}" "${header}"
                COMMAND "${DXC_EXECUTABLE}" ${format_flags} -T ${profile} -E main -Fo "${binary}" "${hlsl}"
                COMMAND "${CMAKE_COMMAND}" -DINPUT=${binary} -DOUTPUT=${header} -DSYMBOL=${symbol}_${format}
                    -P "${HYPERX_SHADERS_SCRIPT}"
                DEPENDS "${hlsl}" "${HYPERX_SHADERS_SCRIPT}"
                COMMENT "Compiling ${name}.hlsl to ${format}"
                VERBATIM)

            target_sources(${target} PRIVATE "${header}")
        endforeach()
    endforeach()

    target_include_directories(${target} PRIVATE "${out_dir}")
endfunction()
