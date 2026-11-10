# Fails unless -sim, run away from a HyperBlade folder, says so and exits instead of waiting on a
# dialog nobody will close.
# Run by ctest as: cmake -DGAME=<the built game> -DSDL=<its SDL3 library> -DPAK=<its pak> -DOUT=<a scratch folder> -P sim_nodata.cmake

file(REMOVE_RECURSE "${OUT}")
file(MAKE_DIRECTORY "${OUT}")
file(COPY "${GAME}" "${SDL}" "${PAK}" DESTINATION "${OUT}")

get_filename_component(name "${GAME}" NAME)

execute_process(
    COMMAND "${OUT}/${name}" -s -sim LOSAN SEATTLE 1
    WORKING_DIRECTORY "${OUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    TIMEOUT 20)

if(result EQUAL 0 OR NOT result MATCHES "^[0-9]+$")
    message(FATAL_ERROR "-sim with no game data ended with \"${result}\", wanted an exit code other than 0:\n${output}")
endif()

string(FIND "${output}" "HYPERX.EXE" at)

if(at EQUAL -1)
    message(FATAL_ERROR "-sim with no game data did not say where to run it:\n${output}")
endif()
