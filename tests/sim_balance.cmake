# Fails unless the balanced switch changes the matches of BLAM! Nemesis and The Sturmnacht, and
# leaves one without either alone.
# Run by ctest as: cmake -DGAME=<the built game> -DOUT=<a scratch folder> -P sim_balance.cmake

file(MAKE_DIRECTORY "${OUT}")

get_filename_component(folder "${GAME}" DIRECTORY)

function(play name)
    execute_process(
        COMMAND "${GAME}" -s -sim ${ARGN} "${OUT}/${name}.csv"
        WORKING_DIRECTORY "${folder}"
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_QUIET)

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "-sim ${ARGN} exited with ${result}")
    endif()

    file(SHA256 "${OUT}/${name}.csv" hash)
    set(${name} "${hash}" PARENT_SCOPE)
endfunction()

play(other LOSAN SEATTLE 1 7 fixed)
play(other_balanced LOSAN SEATTLE 1 7 fixed balanced)
play(nemesis NEM LEEDS 1 7 fixed)
play(nemesis_balanced NEM LEEDS 1 7 fixed balanced)
play(sturmnacht MUNIC LEEDS 1 7 fixed)
play(sturmnacht_balanced MUNIC LEEDS 1 7 fixed balanced)

if(NOT other STREQUAL other_balanced)
    message(FATAL_ERROR "the balance changed a match without either team")
endif()

if(nemesis STREQUAL nemesis_balanced)
    message(FATAL_ERROR "the balance left Nemesis's match unchanged")
endif()

if(sturmnacht STREQUAL sturmnacht_balanced)
    message(FATAL_ERROR "the balance left The Sturmnacht's match unchanged")
endif()
