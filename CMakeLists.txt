cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "toolchain-vc6sp3\\.cmake$")
    message(FATAL_ERROR "This project builds only with Visual C++ 6.0 SP3. Configure with a preset "
                        "from CMakePresets.json, which loads cmake/toolchain-vc6sp3.cmake.")
endif()

project(hyperblade_decomp C CXX)

# Windows-MSVC.cmake appends its own defaults to the toolchain's *_INIT seeds, so the exact strings
# are forced here and the per-configuration flags emptied.
set(CMAKE_C_FLAGS "${VC6_COMPILE_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${VC6_COMPILE_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${VC6_LINKER_FLAGS}" CACHE STRING "" FORCE)

foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_C_FLAGS_${config} "" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_${config} "" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_${config} "" CACHE STRING "" FORCE)
endforeach()

option(HYPERX_BRINGUP
    "Build the bring-up images, which run on a modern machine. Never measure this build." OFF)

option(HYPERX_COMPILE_ONLY
    "Compile every source without your copy of the game. Links no image and runs no check." OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT HYPERX_COMPILE_ONLY)
    include(cmake/resources.cmake)
endif()

set(HYPERX_OUT "${CMAKE_BINARY_DIR}/out")
set(JPEG6A_DIR "${CMAKE_SOURCE_DIR}/third_party/jpeg-6a")

set(HYPERX_LIBS
    kernel32.lib
    user32.lib
    gdi32.lib
    winmm.lib
    version.lib
    ddraw.lib
    dxguid.lib
    dsound.lib
    dplayx.lib)

if(HYPERX_BRINGUP)
    add_compile_definitions(HYPERX_BRINGUP)
endif()

# Link order is part of the match. Append new sources; never sort this list.
# tools/check_link_order.py holds the order the $E thunk markers need.
set(HYPERX_SOURCES
    src/planvm/planactionanim.cpp
    src/planvm/planactiontracking.cpp
    src/planvm/planactionlaunch.cpp
    src/planvm/planactionrecoil.cpp
    src/engine/trackingellipsoidstatics.cpp
    src/engine/cameraobject.cpp
    src/net/cplayer.cpp
    src/planvm/plancond.cpp
    src/ui/infobar.cpp
    src/graphics/lodnode.cpp
    src/planvm/cupdateshotpower.cpp
    src/math/vec3.cpp
    src/ui/keyconfigdialog.cpp
    src/ui/ratingsdialog.cpp
    src/graphics/infobarslot.cpp
    src/anim/animregistry.cpp
    src/net/netsetup.cpp
    src/net/network.cpp
    src/planvm/planaction.cpp
    src/graphics/renderer.cpp
    src/graphics/modeldatabase.cpp
    src/ui/screens/leaguesetupuiscreen.cpp
    src/graphics/cd3dtexturemanager.cpp
    src/engine/cdisplayshotpower.cpp
    src/anim/animevent.cpp
    src/anim/animfile.cpp
    src/graphics/texcacheentry.cpp
    src/anim/animplayer.cpp
    src/engine/cameraprojectionstate.cpp
    src/physics/collisionplane.cpp
    src/ui/screens/trainingendscreen.cpp
    src/graphics/ddraw.cpp
    src/graphics/texture.cpp
    src/graphics/d3d.cpp
    src/planvm/planvm.cpp
    src/common/mem.cpp
    src/ui/screens/mainmenuscreen.cpp
    src/ui/screens/headtoheadsetupuiscreen.cpp
    src/ui/screens/championshipsetupuiscreen.cpp
    src/ui/screens/pausemenufullscreen.cpp
    src/ui/screens/pausemenunosavescreen.cpp
    src/ui/screens/pausemenuminimalscreen.cpp
    src/ui/signflipbookwidget.cpp
    src/math/viewextent.cpp
    src/ui/screens/finalceremonyscreen.cpp
    src/ui/screens/dromeexteriorscreen.cpp
    src/sound/audiomgr.cpp
    src/ui/uiscreen.cpp
    src/ui/screens/lobbyscreen.cpp
    src/ui/screens/multisetupoptionsscreen.cpp
    src/planvm/gamebehaviorobject.cpp
    src/planvm/planstate.cpp
    src/input/actionbindings.cpp
    src/ui/screens/gameconfiguiscreen.cpp
    src/engine/particle.cpp
    src/ui/screens/teamsetupuiscreen.cpp
    src/ui/screens/saveloaduiscreen.cpp
    src/graphics/scrollbitmap.cpp
    src/planvm/csteertoward.cpp
    src/planvm/behparam.cpp
    src/math/slerpseg.cpp
    src/net/gamesession.cpp
    src/input/joystickmgr.cpp
    src/input/keyboarddeviceobj.cpp
    src/planvm/planeditor.cpp
    src/engine/gameoptions.cpp
    src/input/deviceobject.cpp
    src/engine/objectbase.cpp
    src/engine/mousedeviceobj.cpp
    src/engine/joystickdeviceobj.cpp
    src/engine/gamepaddeviceobj.cpp
    src/engine/coachobject.cpp
    src/app/bringup.cpp
    src/net/godsinventory.cpp
    src/common/string.cpp
    src/graphics/softrenderer.cpp
    src/planvm/planeditordialog.cpp
    src/ui/gamesetup.cpp
    src/graphics/steppedscrollbitmap.cpp
    src/graphics/passbar.cpp
    src/graphics/framedbarbitmap.cpp
    src/graphics/cddbitmap.cpp
    src/anim/animclip.cpp
    src/planvm/collinteractor.cpp
    src/engine/particlesystem.cpp
    src/ui/pausemenu.cpp
    src/ui/saveobject.cpp
    src/common/flatbehaviorobject.cpp
    src/graphics/colorpalette.cpp
    src/graphics/gdi.cpp
    src/common/fileobj.cpp
    src/anim/animscripttable.cpp
    src/anim/animclipentry.cpp
    src/anim/animdb.cpp
    src/anim/animnamedref.cpp
    src/anim/animnode.cpp
    src/engine/weaponobject.cpp
    src/engine/dromeobject.cpp
    src/input/cheat.cpp
    src/graphics/d3dviewport.cpp
    src/ui/jumbotextlist.cpp
    src/common/surfacebehaviorobject.cpp
    src/planvm/cpaletteeffect.cpp
    src/planvm/cspeedburstpool.cpp
    src/math/transformobject.cpp
    src/sound/mixer.cpp
    src/engine/msgevent.cpp
    src/graphics/modelcomponent.cpp
    src/planvm/agendaitem.cpp
    src/engine/objectstats.cpp
    src/graphics/infobarradar.cpp
    src/engine/cpansound.cpp
    src/engine/cadjustacceleration.cpp
    src/net/movecodec.cpp
    src/anim/anim.cpp
    src/graphics/palettefade.cpp
    src/graphics/cddtext.cpp
    src/physics/msginteractorpool.cpp
    src/ui/screens/gameintroscreen.cpp
    src/ui/buttonwidget.cpp
    src/net/netsetupdialog.cpp
    src/app/win32.cpp
    src/app/wndproc.cpp
    src/common/stub.cpp
    src/physics/collisionshape.cpp
    src/graphics/cddtextsurface.cpp
    src/engine/goalobject.cpp
    src/graphics/d3ddevice.cpp
    src/graphics/display.cpp
    src/net/directplay.cpp
    src/app/frametimer.cpp
    src/physics/boundbubble.cpp
    src/physics/bubblecmd.cpp
    src/engine/teamblock.cpp
    src/graphics/cd3dtexture.cpp
    src/globals.cpp
    src/app/config.cpp
    src/sound/soundmanager.cpp
    src/ui/screen.cpp
    src/planvm/planfile.cpp
    src/planvm/behobjglidetoobject.cpp
    src/common/behobjopenandclose.cpp
    src/common/behobjupdatewhileattached.cpp
    src/common/behobjmonitorkeyboard.cpp
    src/common/behobjfly.cpp
    src/common/behobjcandytracking.cpp
    src/common/behobjtrackpassprep.cpp
    src/common/behobjtracktrajectory.cpp
    src/common/behobjwatching.cpp
    src/common/behobjheadtracking.cpp
    src/common/behobjtracking.cpp
    src/common/behobjdonothing.cpp
    src/common/behobjsetmovinginplace.cpp
    src/common/behobjsetflatinplace.cpp
    src/common/behobjcoachteam.cpp
    src/common/behobjsetinplace.cpp
    src/common/behobjflytoward.cpp
    src/common/behobjflylcl.cpp
    src/common/behobjrailslide.cpp
    src/common/behobjglidelcl.cpp
    src/common/behobjglide.cpp
    src/common/behobjupdaterelativepos.cpp
    src/common/behobjavoidance.cpp
    src/common/behobjglidebetweenfacing.cpp
    src/common/behobjglidetoposition.cpp
    src/common/crelaxtopositionpool.cpp
    src/common/ccircletrackpool.cpp
    src/common/ccheckaipool.cpp
    src/common/cexplosionanimationpool.cpp
    src/common/cdisplaypassbarpool.cpp
    src/common/cupdateshotpowerpool.cpp
    src/common/cdisplayshotpowerpool.cpp
    src/common/cpaletteeffectpool.cpp
    src/common/cheadtrackpool.cpp
    src/common/cdopplersndpool.cpp
    src/common/cpansoundpool.cpp
    src/common/cviewitpool.cpp
    src/common/ctrackpasserreceiverpool.cpp
    src/common/cwatchpool.cpp
    src/common/ctracktrajpool.cpp
    src/common/ctrackpool.cpp
    src/common/ctimeractivitypool.cpp
    src/common/cmonitorheldkeyspool.cpp
    src/common/cmonitorkeyspool.cpp
    src/common/cstepanimationpool.cpp
    src/common/csetattitudepool.cpp
    src/common/cbungeetowardpool.cpp
    src/common/cavoidobjectspool.cpp
    src/common/cadjustaccelerationpool.cpp
    src/common/csteertowardpool.cpp
    src/common/csteerbetweenfacingpool.cpp
    src/common/csteertopositionpool.cpp
    src/common/csetpositionrelativepool.cpp
    src/common/csteertotargetpool.cpp
    src/common/cbeattachedpool.cpp
    src/common/cprojectandbouncepool.cpp
    src/common/cslideonrailpool.cpp
    src/common/cglideonsurfacepool.cpp
    src/common/crelaxyawpool.cpp
    src/common/gametime.cpp
    src/physics/collisioncheck.cpp
    src/graphics/meshbone.cpp
    src/engine/rokobject.cpp
    src/engine/playerobject.cpp
    src/sound/dsoundmgr.cpp
    src/sound/soundchannel.cpp
    src/engine/engineobject.cpp
    src/physics/physicsbody.cpp
    src/engine/gameengine.cpp
    src/app/gameapp.cpp
    src/ui/widget.cpp
    src/engine/coachai.cpp
    src/graphics/d3drend.cpp
    src/ui/hud.cpp
    src/ui/radar.cpp
    src/graphics/swrend.cpp
    src/physics/collisionobj.cpp
    src/planvm/planarg.cpp
    src/engine/game.cpp
    src/graphics/face.cpp
    src/ui/screens/tournamentsetupuiscreen.cpp
    src/graphics/modelfile.cpp
    src/graphics/mdm1.cpp
    src/planvm/ctimeractivity.cpp
    src/net/networkslot.cpp
    src/ui/screens/jumbotronscreen.cpp
    src/engine/csteertoposition.cpp
    src/planvm/behaviorregistry.cpp
    src/ui/screens/matchinfouiscreen.cpp
    src/ui/screens/awardsuiscreen.cpp
    src/ui/flipbookwidget.cpp
    src/ui/displaywidget.cpp
    src/ui/loopflipbookwidget.cpp
    src/ui/textinputwidget.cpp
    src/ui/cddsurfaceowner.cpp
    src/graphics/softtex.cpp
    src/net/playerbase.cpp
    src/engine/rampobject.cpp
    src/ui/messagequeue.cpp
    src/ui/holdingwidget.cpp
    src/planvm/planinteractorfactories.cpp
    src/planvm/ctracktraj.cpp
    src/planvm/cslideonrail.cpp
    src/engine/csetattitude.cpp
    src/engine/cbeattached.cpp
    src/common/behobjcandytrackingpool.cpp
    src/common/behobjcoachteampool.cpp
    src/common/behobjtrackpasspreppool.cpp
    src/common/behobjtracktrajectorypool.cpp
    src/common/behobjwatchingpool.cpp
    src/common/behobjheadtrackingpool.cpp
    src/common/behobjtrackingpool.cpp
    src/common/behobjopenandclosepool.cpp
    src/common/behobjdonothingpool.cpp
    src/common/behobjsetmovinginplacepool.cpp
    src/common/behobjsetflatinplacepool.cpp
    src/common/behobjsetinplacepool.cpp
    src/common/behobjupdatewhileattachedpool.cpp
    src/common/behobjmonitorkeyboardpool.cpp
    src/common/behobjflytowardpool.cpp
    src/common/behobjflylclpool.cpp
    src/common/behobjflypool.cpp
    src/common/behobjrailslidepool.cpp
    src/common/behobjglidelclpool.cpp
    src/common/behobjglidepool.cpp
    src/common/behobjupdaterelativepospool.cpp
    src/common/behobjavoidancepool.cpp
    src/common/behobjglidebetweenfacingpool.cpp
    src/common/behobjglidetopositionpool.cpp
    src/common/poolcountsdef.cpp
    src/common/staticinitglobals.cpp
    src/common/behobjglidetoobjectpool.cpp
    src/planvm/kindagendaitem.cpp
    src/planvm/cdopplersnd.cpp
    src/planvm/cstepanimation.cpp
    src/planvm/cviewit.cpp
    src/planvm/baseresetagendaitem.cpp
    src/planvm/releasingagendaitem.cpp
    src/planvm/cameraliteagendaitem.cpp
    src/graphics/graphics.cpp
    src/common/parseutil.cpp
    src/physics/collisionellipsoid.cpp
    src/graphics/visualeffectfade.cpp
    src/engine/level.cpp
    src/graphics/cgpalette.cpp
    src/planvm/crelaxtoposition.cpp
    src/ui/infobarbase.cpp
    src/planvm/interactor.cpp
    src/planvm/msginteractor.cpp
    src/sound/wave.cpp
    src/engine/cwatch.cpp
    src/anim/animclipset.cpp
    src/engine/objectdef.cpp
    src/planvm/bubblenode.cpp
    src/math/point.cpp
    src/engine/inputdeviceobj.cpp
    src/graphics/damagebar.cpp
    src/graphics/renderstatedef.cpp
    src/common/stringpool.cpp
    src/graphics/rlespritesurface.cpp
    src/common/util.cpp
    src/math/math.cpp
    src/graphics/collisioncage.cpp
    src/app/window.cpp
    src/app/winmain.cpp
    src/ui/debughud.cpp
    src/engine/matchstats.cpp
    src/common/pooledownerroot.cpp
    src/engine/teamsblock.cpp
    src/math/spline.cpp
    src/planvm/cbungeetoward.cpp
    src/planvm/csteerbetweenfacing.cpp
    src/ui/screens/keyconfigscreen.cpp
    src/ui/dialog.cpp
    src/ui/animationpeekdialog.cpp
    src/math/quat.cpp
    src/planvm/cheadtrack.cpp
    src/planvm/cprojectandbounce.cpp
    src/planvm/cglideonsurface.cpp
    src/planvm/csteertotarget.cpp
    src/graphics/palettescalarfade.cpp
    src/common/purevirtualcall.cpp
    src/engine/object.cpp
    src/planvm/planutil.cpp
    src/engine/player.cpp
    src/graphics/meshmaterial.cpp
    src/engine/csetpositionrelative.cpp
    src/engine/cmonitorheldkeys.cpp
    src/engine/cexplosionanimation.cpp
    src/engine/ccheckai.cpp
    src/engine/crelaxyaw.cpp
    src/planvm/cavoidobjects.cpp
    src/graphics/clipsphere.cpp
    src/engine/cspeedburst.cpp
    src/planvm/cmonitorkeys.cpp
    src/engine/cdisplaypassbar.cpp
    src/engine/ctrackpasserreceiver.cpp
    src/engine/ctrack.cpp
    src/engine/ccircletrack.cpp
    src/input/joystickdevice.cpp
    src/ui/mainwindow.cpp
    src/ui/splashdialog.cpp
    src/physics/capsule.cpp
    src/graphics/meshboundingbox.cpp
    src/graphics/lightattenuation.cpp
    src/math/splinecurve.cpp
    src/planvm/planfocusarg.cpp
    src/physics/collinteractorpool.cpp
    src/engine/arenaellipsoidstatic.cpp
    src/common/agendaitemowner.cpp
    src/common/nodeowner.cpp
    src/common/behaviorowner.cpp
    src/engine/inplacebehaviorobject.cpp
    src/common/flightbehaviorobject.cpp
    src/common/classsizes.cpp
    src/ui/inertwidget.cpp
    src/input/input.cpp
    src/anim/animset.cpp)

# The optimisation is set per source, so that no flag on a command line overrides another. /Ox /Gf
# is /O2 without /Gy: the original folds inline bodies only.
set_source_files_properties(${HYPERX_SOURCES} PROPERTIES COMPILE_FLAGS "/Ox /Gf")

add_library(hyperx_objects OBJECT ${HYPERX_SOURCES})
target_include_directories(hyperx_objects PRIVATE "${CMAKE_SOURCE_DIR}/src" "${JPEG6A_DIR}")

# IJG JPEG 6a with the game's own copy of djpeg's BMP writer, built /O1 /Ob2 as the original's
# was. The members link after the game's objects and before LIBCMT's, in the order the game
# reaches them.
set(JPEG6A_SOURCES
    jdapimin.c
    jdapistd.c
    jdatasrc.c
    cdjpeg.c
    jerror.c
    jdinput.c
    jdmarker.c
    jmemmgr.c
    jcomapi.c
    jdmaster.c
    jutils.c
    jmemansi.c
    jdmainct.c
    jdcoefct.c
    jdhuff.c
    jdphuff.c
    jddctmgr.c
    jdpostct.c
    jdsample.c
    jdcolor.c
    jdmerge.c
    jquant2.c
    jquant1.c
    jidctred.c
    jidctint.c
    jidctfst.c
    jidctflt.c)
list(TRANSFORM JPEG6A_SOURCES PREPEND "${JPEG6A_DIR}/")

# A separate object target: VC6's NMAKE does not see an object it rebuilt in the same run as newer
# than the archive.
add_library(jpeg6a_objects OBJECT ${JPEG6A_SOURCES} src/engine/bmpwriter.cpp)
target_include_directories(jpeg6a_objects PRIVATE "${JPEG6A_DIR}" "${CMAKE_SOURCE_DIR}/src")
target_compile_options(jpeg6a_objects PRIVATE /O1 /Ob2)

add_library(jpeg6a STATIC $<TARGET_OBJECTS:jpeg6a_objects>)

# Everything below needs the original: its resources, its timestamp, or its bytes to compare.
if(HYPERX_COMPILE_ONLY)
    message(STATUS "Compile only: no image is linked")
    return()
endif()

# hyperx_image(<target> <output name> [STAMP] [LINK_OPTIONS <options>...])
# STAMP patches in the original's COFF TimeDateStamp, which LINK.EXE 6.00.8447 cannot set.
function(hyperx_image target output)
    cmake_parse_arguments(PARSE_ARGV 2 arg "STAMP" "" "LINK_OPTIONS")

    add_executable(${target} WIN32 $<TARGET_OBJECTS:hyperx_objects> "${HYPERX_RES}")
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${output}"
        RUNTIME_OUTPUT_DIRECTORY "${HYPERX_OUT}")
    target_link_libraries(${target} PRIVATE jpeg6a ${HYPERX_LIBS})
    target_link_options(${target} PRIVATE ${arg_LINK_OPTIONS} "/MAP:${HYPERX_OUT}/${output}.map")

    if(arg_STAMP)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/stamp.py"
                    "$<TARGET_FILE:${target}>" --reference "${HYPERX_REFERENCE}"
            COMMENT "Patching the COFF timestamp to the original's link time"
            VERBATIM)
    endif()
endfunction()

# HYPERX.exe links like the original, with no debug directory. The same objects link again into
# HYPERX_sym.exe with a PDB for reccmp, /OPT:NOREF so uncalled functions keep their rows, and no
# /OPT:ICF so no function folds away its symbol.
hyperx_image(hyperx HYPERX STAMP)
hyperx_image(hyperx_sym HYPERX_sym
    LINK_OPTIONS /DEBUG /OPT:NOREF "/PDB:${HYPERX_OUT}/HYPERX_sym.pdb")

add_custom_target(verify)

# hyperx_check(<target> <tool> <comment> [DEPENDS <target>] [ARGS <args>...])
# Adds a target that runs tools/<tool>, and makes verify run it.
function(hyperx_check name tool comment)
    cmake_parse_arguments(PARSE_ARGV 3 arg "" "DEPENDS" "ARGS")

    add_custom_target(${name}
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/${tool}" ${arg_ARGS}
        DEPENDS ${arg_DEPENDS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "${comment}"
        VERBATIM)
    add_dependencies(verify ${name})
endfunction()

add_custom_target(match
    COMMAND "${Python3_EXECUTABLE}" -m reccmp.tools.asmcmp --target HYPERX --no-color
    DEPENDS hyperx_sym
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Scoring every annotated function against the original"
    USES_TERMINAL
    VERBATIM)
add_dependencies(verify match)

# No --warnfail: function_out_of_order cannot be obeyed, because a function's position in its file
# changes VC6's code.
hyperx_check(lint check_lint.py
    "Linting the reccmp annotations")

hyperx_check(verify-toolchain richcheck.py
    "Comparing Rich header stamps against the original"
    DEPENDS hyperx ARGS "$<TARGET_FILE:hyperx>" --reference "${HYPERX_REFERENCE}")
hyperx_check(verify-bytes pecompare.py
    "Comparing the built image against the original"
    DEPENDS hyperx ARGS "$<TARGET_FILE:hyperx>" "${HYPERX_REFERENCE}")
hyperx_check(verify-resources rsrccompare.py
    "Comparing the built .rsrc against the original's"
    DEPENDS hyperx ARGS "$<TARGET_FILE:hyperx>" "${HYPERX_REFERENCE}")

hyperx_check(verify-vtables check_vtables.py
    "Pairing every vtable against the original" DEPENDS hyperx_sym)
hyperx_check(verify-datavalues check_datavalues.py
    "Comparing every annotated global's value against the original" DEPENDS hyperx_sym)
hyperx_check(verify-folding check_folded.py
    "Checking that nothing in the measurement build folded" DEPENDS hyperx_sym)
hyperx_check(verify-vtable-arity check_vtable_arity_declared.py
    "Comparing each class's declared virtuals against its table's length")
hyperx_check(verify-one-declaration view_classes.py
    "Checking that no class is declared in two files" ARGS --twice)
hyperx_check(verify-self-casts check_self_casts.py
    "Checking that no member casts this to its own class")
hyperx_check(verify-field-offsets check_field_offsets.py
    "Checking that no class declares its fields twice")
hyperx_check(verify-slot-arity check_slot_arity.py
    "Checking each class's slots against the arities its table carries")
hyperx_check(verify-dtor-calls check_dtor_calls.py
    "Checking that every explicit destructor call names its class")
hyperx_check(verify-member-owner check_member_owner.py
    "Checking that every marked member sits on the class whose table holds it")
hyperx_check(verify-vptr-offsets check_vptr_offsets.py
    "Checking that no first field overlaps its vptr or its base")
hyperx_check(verify-virtual-bodies check_virtual_bodies.py
    "Checking that every declared virtual has a body")
hyperx_check(verify-field-tiling check_field_tiling.py
    "Checking that each class's fields tile without a hole or an overlap")
hyperx_check(verify-tables-named check_tables_named.py
    "Checking that every vtable in the image has a class")
hyperx_check(verify-body-reach check_body_reach.py
    "Checking that no body reads past the end of its own class")
hyperx_check(verify-shared-reserves shared_reserves.py
    "Checking that no two siblings reserve the same bytes over one base")
hyperx_check(verify-base-reads base_reads_derived.py
    "Checking that no base reads into one of its derived classes")
hyperx_check(verify-vtbl-stores check_vtbl_stores.py
    "Checking that a class stores only its own table")
hyperx_check(verify-conventions check_conventions.py
    "Checking each declared convention against the client's own RET")
hyperx_check(verify-marker-names marker_next_line.py
    "Checking that no marker takes its name from a comment above a declaration")
hyperx_check(verify-plan-names plan_names.py
    "Checking each plan function against the developers' name for its id")
hyperx_check(verify-enum-names enum_names.py
    "Checking each numbered id against the developers' own name for it")
hyperx_check(verify-kind-names kind_names.py
    "Checking each object kind against the name the client's type table gives it")
hyperx_check(verify-blob-rivals blob_rivals.py
    "Checking each blob's own offset names against the class that covers them")
hyperx_check(verify-rival-names rival_names.py
    "Checking each define against the field name at the same offset")

# Writes names, globals, vtables and types into a Ghidra database built from your own copy of the
# game. Not part of verify, because it writes.
set(HYPERX_GHIDRA_PROJECT "" CACHE FILEPATH "Your own Ghidra project (.gpr) file")
set(HYPERX_GHIDRA_FILE "/HYPERX.EXE" CACHE STRING "The program inside that Ghidra project")
set(HYPERX_GHIDRA_ROOT "" CACHE PATH "Your Ghidra installation; empty uses GHIDRA_INSTALL_DIR")

if(HYPERX_GHIDRA_PROJECT)
    set(ghidra_root_arg)
    if(HYPERX_GHIDRA_ROOT)
        set(ghidra_root_arg --ghidra "${HYPERX_GHIDRA_ROOT}")
    endif()

    add_custom_target(publish-ghidra
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/publish_ghidra.py"
                --project "${HYPERX_GHIDRA_PROJECT}"
                --file "${HYPERX_GHIDRA_FILE}"
                ${ghidra_root_arg}
        DEPENDS hyperx_sym
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Publishing names, globals, vtables and types into your Ghidra database"
        USES_TERMINAL
        VERBATIM)
endif()

if(HYPERX_BRINGUP)
    # The game data is junctioned in beside the images. mklink /J needs no privilege.
    set(HYPERX_GAME_DATA "${CMAKE_SOURCE_DIR}/build/out" CACHE PATH
        "Directory holding the game's Data, DIRECTX, AVI, DOS and ACRODOS folders")

    # No VERBATIM: cmd /c must receive the inner quotes unescaped.
    set(asset_commands)
    foreach(asset Data DIRECTX AVI DOS ACRODOS)
        list(APPEND asset_commands COMMAND cmd /c
            "if not exist \"${HYPERX_OUT}/${asset}\" \
             if exist \"${HYPERX_GAME_DATA}/${asset}\" \
             mklink /J \"${HYPERX_OUT}/${asset}\" \"${HYPERX_GAME_DATA}/${asset}\" > nul")
    endforeach()

    add_custom_target(bringup-assets ${asset_commands}
        COMMENT "Linking the game data beside the bring-up images")

    # The software renderer. BringUp::Variant reads it from the file name.
    hyperx_image(hyperx_bringup_soft hyperx_bringup_soft STAMP)
    add_dependencies(hyperx_bringup_soft bringup-assets)
endif()

message(STATUS "VC6 tree  : ${VC6_ROOT}")
message(STATUS "Reference : ${HYPERX_REFERENCE}")
message(STATUS "Bring-up  : ${HYPERX_BRINGUP}")
