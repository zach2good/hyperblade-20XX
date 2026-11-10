# Checks a simulated run's CSV against rules the port's recorded stats keep.
# Included by sim_replay.cmake with CSV set to the file.

file(STRINGS "${CSV}" lines)
list(POP_FRONT lines header)
string(REPLACE "," ";" header "${header}")

foreach(column IN ITEMS match team player id_STA_rGoals id_STA_rAssists id_STA_rOneTimers id_STA_rPassesAttempted id_STA_rPassesCompleted id_STA_rChecksGiven id_STA_rHangTime id_STA_rInterceptions id_STA_rDragsWon period_ms)
    list(FIND header ${column} index)

    if(index LESS 0)
        message(FATAL_ERROR "${CSV} has no ${column} column")
    endif()

    set(at_${column} ${index})
endforeach()

function(add_to name value)
    if(NOT DEFINED ${name})
        set(${name} 0)
    endif()

    math(EXPR sum "${${name}} + ${value}")
    set(${name} ${sum} PARENT_SCOPE)
endfunction()

# Per team per match: goals, to compare each player's assists with their teammates' goals, and pass
# attempts and interceptions, since a team can't intercept more passes than the other side tried.
set(sides "")

foreach(line IN LISTS lines)
    string(REPLACE "," ";" row "${line}")
    list(GET row ${at_match} match)
    list(GET row ${at_team} team)
    list(GET row ${at_id_STA_rGoals} goals)
    list(GET row ${at_id_STA_rPassesAttempted} attempted)
    list(GET row ${at_id_STA_rInterceptions} intercepted)
    list(GET row ${at_id_STA_rDragsWon} drags)
    string(MAKE_C_IDENTIFIER "${match}_${team}" side)
    string(MAKE_C_IDENTIFIER "${match}" whole)

    add_to(goals_${side} ${goals})
    add_to(attempts_${side} ${attempted})
    add_to(attempts_${whole} ${attempted})
    add_to(intercepts_${side} ${intercepted})
    add_to(drags_${whole} ${drags})
    add_to(goals_${whole} ${goals})
    list(APPEND sides "${side}|${whole}|${match}|${team}")
endforeach()

list(REMOVE_DUPLICATES sides)

foreach(entry IN LISTS sides)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 side)
    list(GET parts 1 whole)
    list(GET parts 2 match)
    list(GET parts 3 team)
    math(EXPR opponent_attempts "${attempts_${whole}} - ${attempts_${side}}")

    if(intercepts_${side} GREATER opponent_attempts)
        message(FATAL_ERROR "match ${match}, ${team}: ${intercepts_${side}} interceptions of ${opponent_attempts} passes tried against them")
    endif()

    # A drag starts each of the three periods and follows each goal.
    math(EXPR most_drags "3 + ${goals_${whole}}")

    if(drags_${whole} LESS 3 OR drags_${whole} GREATER most_drags)
        message(FATAL_ERROR "match ${match}: ${drags_${whole}} drags won, outside 3 to ${most_drags}")
    endif()
endforeach()

set(completed_total 0)
set(checks_total 0)
set(hang_total 0)

foreach(line IN LISTS lines)
    string(REPLACE "," ";" row "${line}")
    list(GET row ${at_match} match)
    list(GET row ${at_team} team)
    list(GET row ${at_player} player)
    list(GET row ${at_id_STA_rGoals} goals)
    list(GET row ${at_id_STA_rAssists} assists)
    list(GET row ${at_id_STA_rOneTimers} onetimers)
    list(GET row ${at_id_STA_rPassesAttempted} attempted)
    list(GET row ${at_id_STA_rPassesCompleted} completed)
    list(GET row ${at_id_STA_rChecksGiven} checks)
    list(GET row ${at_id_STA_rHangTime} hang)
    list(GET row ${at_period_ms} period)
    string(REGEX REPLACE "\\..*" "" hang "${hang}")
    string(REGEX REPLACE "\\..*" "" period "${period}")
    math(EXPR match_ms "3 * ${period}")
    string(MAKE_C_IDENTIFIER "${match}_${team}" side)
    math(EXPR teammate_goals "${goals_${side}} - ${goals}")

    if(completed GREATER attempted)
        message(FATAL_ERROR "match ${match}, ${player}: ${completed} passes completed of ${attempted} attempted")
    endif()

    if(assists GREATER teammate_goals)
        message(FATAL_ERROR "match ${match}, ${player}: ${assists} assists but teammates scored ${teammate_goals}")
    endif()

    if(hang GREATER match_ms)
        message(FATAL_ERROR "match ${match}, ${player}: ${hang} ms of hang time in a ${match_ms} ms match")
    endif()

    if(onetimers GREATER goals)
        message(FATAL_ERROR "match ${match}, ${player}: ${onetimers} one-timers but ${goals} goals")
    endif()

    math(EXPR completed_total "${completed_total} + ${completed}")
    math(EXPR checks_total "${checks_total} + ${checks}")
    math(EXPR hang_total "${hang_total} + ${hang}")
endforeach()

if(completed_total EQUAL 0)
    message(FATAL_ERROR "no pass was completed in ${CSV}")
endif()

if(checks_total EQUAL 0)
    message(FATAL_ERROR "no check was given in ${CSV}")
endif()

if(hang_total EQUAL 0)
    message(FATAL_ERROR "no hang time was recorded in ${CSV}")
endif()
