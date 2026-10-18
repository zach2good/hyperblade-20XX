# hyperx_link_game_data(<target>) puts the game's folders beside <target>'s image, linked rather
# than copied, and copies HYPERX.EXE there, so the build runs as if installed. Nothing from the
# game goes into the image.
function(hyperx_link_game_data target)
    if(NOT HYPERX_GAME_DATA)
        return()
    endif()

    set(out "$<TARGET_FILE_DIR:${target}>")
    set(commands)

    foreach(folder Data DIRECTX AVI DOS ACRODOS)
        if(NOT EXISTS "${HYPERX_GAME_DATA}/${folder}")
            continue()
        endif()

        if(WIN32)
            # A junction needs no privilege, where a symbolic link does.
            file(TO_NATIVE_PATH "${HYPERX_GAME_DATA}/${folder}" source)
            list(APPEND commands COMMAND cmd /c "if not exist \"${folder}\" mklink /J \"${folder}\" \"${source}\" > nul")
        else()
            list(APPEND commands COMMAND ln -sfn "${HYPERX_GAME_DATA}/${folder}" "${folder}")
        endif()
    endforeach()

    if(EXISTS "${HYPERX_GAME_DATA}/HYPERX.EXE")
        list(APPEND commands COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${HYPERX_GAME_DATA}/HYPERX.EXE" HYPERX.EXE)
    endif()

    # No VERBATIM: cmd /c must receive the inner quotes unescaped.
    add_custom_target(game-data ${commands}
        WORKING_DIRECTORY "${HYPERX_OUT}"
        COMMENT "Linking the game data beside the image")
    add_dependencies(${target} game-data)
endfunction()
