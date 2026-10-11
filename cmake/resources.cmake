# The .rsrc section comes from the user's own copy of the game, extracted to a .res under the
# build directory. LINK.EXE runs CVTRES.EXE on it, which writes the 000606c7 Rich header stamp.

if(NOT HYPERX_REFERENCE)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/reference.py"
        OUTPUT_VARIABLE HYPERX_REFERENCE_FOUND
        ERROR_VARIABLE HYPERX_REFERENCE_ERROR
        RESULT_VARIABLE HYPERX_REFERENCE_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT HYPERX_REFERENCE_STATUS EQUAL 0)
        message(FATAL_ERROR "${HYPERX_REFERENCE_ERROR}")
    endif()

    set(HYPERX_REFERENCE "${HYPERX_REFERENCE_FOUND}"
        CACHE FILEPATH "The original HYPERX.EXE" FORCE)
endif()

if(NOT EXISTS "${HYPERX_REFERENCE}")
    message(FATAL_ERROR
        "The original executable is not at '${HYPERX_REFERENCE}'. Name your copy in "
        "reccmp-user.yml or pass -DHYPERX_REFERENCE=<path>.")
endif()

set(HYPERX_RES "${CMAKE_BINARY_DIR}/generated/HYPERX.res")

add_custom_command(
    OUTPUT "${HYPERX_RES}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/extract_resources.py"
            "${HYPERX_REFERENCE}" --output "${HYPERX_RES}"
    DEPENDS "${HYPERX_REFERENCE}"
            "${CMAKE_SOURCE_DIR}/tools/extract_resources.py"
            "${CMAKE_SOURCE_DIR}/tools/reference.py"
    COMMENT "Extracting the resource directory from the original"
    VERBATIM)

# EXTERNAL_OBJECT puts the .res on the link line in place of an object.
set_source_files_properties("${HYPERX_RES}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
