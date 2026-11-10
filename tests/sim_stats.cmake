# Checks a simulated run's CSV against rules the port's pass, assist, one-timer and check stats keep.
# Included by sim_replay.cmake with CSV set to the file.

file(STRINGS "${CSV}" lines)
list(POP_FRONT lines header)
string(REPLACE "," ";" header "${header}")

foreach(column IN ITEMS match team player id_STA_rGoals id_STA_rAssists id_STA_rOneTimers id_STA_rPassesAttempted id_STA_rPassesCompleted id_STA_rChecksGiven)
    list(FIND header ${column} index)

    if(index LESS 0)
        message(FATAL_ERROR "${CSV} has no ${column} column")
    endif()

    set(at_${column} ${index})
endforeach()

# Goals per team per match, to compare each player's assists with their teammates' goals.
foreach(line IN LISTS lines)
    string(REPLACE "," ";" row "${line}")
    list(GET row ${at_match} match)
    list(GET row ${at_team} team)
    list(GET row ${at_id_STA_rGoals} goals)
    string(MAKE_C_IDENTIFIER "goals_${match}_${team}" key)

    if(NOT DEFINED ${key})
        set(${key} 0)
    endif()

    math(EXPR ${key} "${${key}} + ${goals}")
endforeach()

set(completed_total 0)
set(checks_total 0)

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
    string(MAKE_C_IDENTIFIER "goals_${match}_${team}" key)
    math(EXPR teammate_goals "${${key}} - ${goals}")

    if(completed GREATER attempted)
        message(FATAL_ERROR "match ${match}, ${player}: ${completed} passes completed of ${attempted} attempted")
    endif()

    if(assists GREATER teammate_goals)
        message(FATAL_ERROR "match ${match}, ${player}: ${assists} assists but teammates scored ${teammate_goals}")
    endif()

    if(onetimers GREATER goals)
        message(FATAL_ERROR "match ${match}, ${player}: ${onetimers} one-timers but ${goals} goals")
    endif()

    math(EXPR completed_total "${completed_total} + ${completed}")
    math(EXPR checks_total "${checks_total} + ${checks}")
endforeach()

if(completed_total EQUAL 0)
    message(FATAL_ERROR "no pass was completed in ${CSV}")
endif()

if(checks_total EQUAL 0)
    message(FATAL_ERROR "no check was given in ${CSV}")
endif()
