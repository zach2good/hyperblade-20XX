# Plays the same simulated matches in two games at once and fails unless both write the same CSV,
# and that CSV keeps the rules in sim_stats.cmake.
# Run by ctest as: cmake -DGAME=<the built game> -DOUT=<a scratch folder> -P sim_replay.cmake
# Two at once, because the uninitialised reads this guards against showed up only under load.

file(MAKE_DIRECTORY "${OUT}")

get_filename_component(folder "${GAME}" DIRECTORY)

# Two COMMANDs run together, piped; the sim reads no input, so the pipe joins nothing.
execute_process(
    COMMAND "${GAME}" -s -sim LOSAN SEATTLE 3 1 fixed "${OUT}/first.csv"
    COMMAND "${GAME}" -s -sim LOSAN SEATTLE 3 1 fixed "${OUT}/second.csv"
    WORKING_DIRECTORY "${folder}"
    RESULTS_VARIABLE results
    OUTPUT_QUIET
    ERROR_QUIET)

foreach(result IN LISTS results)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "a simulated run failed: ${results}")
    endif()
endforeach()

file(SHA256 "${OUT}/first.csv" first)
file(SHA256 "${OUT}/second.csv" second)

if(NOT first STREQUAL second)
    message(FATAL_ERROR "the same seed played different matches: compare ${OUT}/first.csv and ${OUT}/second.csv")
endif()

set(CSV "${OUT}/first.csv")
include("${CMAKE_CURRENT_LIST_DIR}/sim_stats.cmake")
