# Fails unless -sim turns down bad arguments with the usage and exit code 2, so a script never takes
# a run that played nothing for a result.
# Run by ctest as: cmake -DGAME=<the built game> -P sim_usage.cmake

get_filename_component(folder "${GAME}" DIRECTORY)

function(expect_usage expected)
    execute_process(
        COMMAND "${GAME}" -s -sim ${ARGN}
        WORKING_DIRECTORY "${folder}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_QUIET)

    if(NOT result EQUAL 2)
        message(FATAL_ERROR "-sim ${ARGN} exited with ${result}, wanted 2")
    endif()

    foreach(text IN ITEMS "usage:" ${expected})
        string(FIND "${output}" "${text}" at)

        if(at EQUAL -1)
            message(FATAL_ERROR "-sim ${ARGN} printed no \"${text}\":\n${output}")
        endif()
    endforeach()
endfunction()

expect_usage("unknown argument: bogus" LOSAN SEATTLE 3 bogus)
expect_usage("" LOSAN SEATTLE 3x)
expect_usage("" NOWHERE SEATTLE 1)
