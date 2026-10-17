# hyperx_add_pak(<target> <output> <files...>) zips files from res/ into <output> before <target>
# builds, each named in the zip by its path below res/.

# Run with cmake -P from res/, this file zips FILES, a comma-separated list, into OUTPUT.
if(CMAKE_SCRIPT_MODE_FILE)
    string(REPLACE "," ";" files "${FILES}")
    file(REMOVE "${OUTPUT}")
    file(ARCHIVE_CREATE OUTPUT "${OUTPUT}" PATHS ${files} FORMAT zip MTIME 0)
    return()
endif()

set(HYPERX_PAK_SCRIPT "${CMAKE_CURRENT_LIST_FILE}")

function(hyperx_add_pak target output)
    set(sources)
    foreach(file IN LISTS ARGN)
        list(APPEND sources "${CMAKE_SOURCE_DIR}/res/${file}")
    endforeach()

    list(JOIN ARGN "," files)

    add_custom_command(
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" -DOUTPUT=${output} -DFILES=${files} -P "${HYPERX_PAK_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/res"
        DEPENDS ${sources} "${HYPERX_PAK_SCRIPT}"
        COMMENT "Packing ${output}"
        VERBATIM)

    add_custom_target(${target}-pak DEPENDS "${output}")
    add_dependencies(${target} ${target}-pak)
endfunction()
