# Plays the same seeded matches on v1.0.0.0 and v1.0.0.1 and fails unless both write the same CSV,
# and fails unless the demo is turned away with a message.
# Run by ctest as: cmake -DGAME=<the built game> -DREFERENCE=<hyperblade-reference> -DOUT=<a scratch folder> -P sim_builds.cmake

get_filename_component(folder "${GAME}" DIRECTORY)
get_filename_component(name "${GAME}" NAME)
file(GLOB beside "${folder}/*.dll" "${folder}/*.so*" "${folder}/*.pak")

# An install of one build: the port beside that build's HYPERX.EXE, and its DATA linked, not copied.
function(make_install build)
    set(install "${OUT}/${build}")

    # Unlink the last run's DATA first, so removing the install can't reach the reference.
    if(IS_SYMLINK "${install}/DATA" OR IS_DIRECTORY "${install}/DATA")
        if(WIN32)
            file(TO_NATIVE_PATH "${install}/DATA" link)
            execute_process(COMMAND cmd /c rmdir "${link}")
        else()
            file(REMOVE "${install}/DATA")
        endif()
    endif()

    file(REMOVE_RECURSE "${install}")
    file(MAKE_DIRECTORY "${install}")
    file(COPY "${GAME}" ${beside} "${REFERENCE}/${build}/HYPERX.EXE" DESTINATION "${install}")

    if(WIN32)
        file(TO_NATIVE_PATH "${REFERENCE}/${build}/DATA" source)
        file(TO_NATIVE_PATH "${install}/DATA" link)
        execute_process(COMMAND cmd /c mklink /J "${link}" "${source}" OUTPUT_QUIET RESULT_VARIABLE result)
    else()
        file(CREATE_LINK "${REFERENCE}/${build}/DATA" "${install}/DATA" SYMBOLIC RESULT result)
    endif()

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "could not link ${build}'s DATA: ${result}")
    endif()
endfunction()

foreach(build v1.0.0.0 v1.0.0.1)
    make_install(${build})

    execute_process(
        COMMAND "${OUT}/${build}/${name}" -s -sim LOSAN SEATTLE 2 1 fixed "${OUT}/${build}.csv"
        WORKING_DIRECTORY "${OUT}/${build}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        TIMEOUT 120)

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${build} failed to play: ${result}\n${output}")
    endif()
endforeach()

file(SHA256 "${OUT}/v1.0.0.0.csv" first)
file(SHA256 "${OUT}/v1.0.0.1.csv" second)

if(NOT first STREQUAL second)
    message(FATAL_ERROR "v1.0.0.0 and v1.0.0.1 played different matches: compare ${OUT}/v1.0.0.0.csv and ${OUT}/v1.0.0.1.csv")
endif()

make_install(demo-1996-08)

execute_process(
    COMMAND "${OUT}/demo-1996-08/${name}" -s -sim LOSAN SEATTLE 1
    WORKING_DIRECTORY "${OUT}/demo-1996-08"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    TIMEOUT 20)

string(FIND "${output}" "August 1996 demo" at)

if(result EQUAL 0 OR at EQUAL -1)
    message(FATAL_ERROR "the demo was not turned away (${result}):\n${output}")
endif()
