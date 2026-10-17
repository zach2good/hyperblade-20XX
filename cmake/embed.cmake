# hyperx_embed(<target> <files...>) embeds each file's bytes in a header, foo.png becoming
# "foo.png.h" with the array foo_png, through the same writer the shaders use.

function(hyperx_embed target)
    set(out_dir "${CMAKE_BINARY_DIR}/generated/embed")
    file(MAKE_DIRECTORY "${out_dir}")

    foreach(source IN LISTS ARGN)
        get_filename_component(source "${source}" ABSOLUTE)
        get_filename_component(name "${source}" NAME)

        string(MAKE_C_IDENTIFIER "${name}" symbol)

        set(header "${out_dir}/${name}.h")

        add_custom_command(
            OUTPUT "${header}"
            COMMAND "${CMAKE_COMMAND}" -DINPUT=${source} -DOUTPUT=${header} -DSYMBOL=${symbol}
                -P "${HYPERX_SHADERS_SCRIPT}"
            DEPENDS "${source}" "${HYPERX_SHADERS_SCRIPT}"
            COMMENT "Embedding ${name}"
            VERBATIM)

        target_sources(${target} PRIVATE "${header}")
    endforeach()

    target_include_directories(${target} PRIVATE "${out_dir}")
endfunction()
