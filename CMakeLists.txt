cmake_minimum_required(VERSION 3.25)

project(hyperblade_20xx VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "HyperBlade 20XX builds for 64-bit targets only.")
endif()

# Floats round alike in every build and on every compiler, the dependencies' included: no fast
# maths, no fused multiply-adds. This comes before them so it reaches their targets too.
if(MSVC)
    add_compile_options(/fp:precise)
else()
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

# For running from the build: the game's folders are linked in beside the image and HYPERX.EXE
# copied there, and the data checks read them. Nothing from them goes into the image.
set(HYPERX_GAME_DATA "" CACHE PATH
    "Directory holding the game's HYPERX.EXE and its Data, DIRECTX, AVI, DOS and ACRODOS folders")

include(cmake/dependencies.cmake)
include(cmake/shaders.cmake)
include(cmake/pak.cmake)
include(cmake/targets.cmake)
include(cmake/gamedata.cmake)

set(HYPERX_OUT "${CMAKE_BINARY_DIR}/out")
set(HYPERX_PAK_NAME 20XX_data.pak)

set(HYPERX_SOURCES
    src/anim/anim.cpp
    src/anim/animclip.cpp
    src/anim/animclipentry.cpp
    src/anim/animclipset.cpp
    src/anim/animdb.cpp
    src/anim/animevent.cpp
    src/anim/animfile.cpp
    src/anim/animnamedref.cpp
    src/anim/animnode.cpp
    src/anim/animplayer.cpp
    src/anim/animregistry.cpp
    src/anim/animscripttable.cpp
    src/anim/animset.cpp
    src/app/config.cpp
    src/app/frametimer.cpp
    src/app/gameapp.cpp
    src/app/window.cpp
    src/app/winmain.cpp
    src/app/wndproc.cpp
    src/common/agendaitemowner.cpp
    src/common/behaviorowner.cpp
    src/common/behobjavoidance.cpp
    src/common/behobjavoidancepool.cpp
    src/common/behobjcandytracking.cpp
    src/common/behobjcandytrackingpool.cpp
    src/common/behobjcoachteam.cpp
    src/common/behobjcoachteampool.cpp
    src/common/behobjdonothing.cpp
    src/common/behobjdonothingpool.cpp
    src/common/behobjfly.cpp
    src/common/behobjflylcl.cpp
    src/common/behobjflylclpool.cpp
    src/common/behobjflypool.cpp
    src/common/behobjflytoward.cpp
    src/common/behobjflytowardpool.cpp
    src/common/behobjglide.cpp
    src/common/behobjglidebetweenfacing.cpp
    src/common/behobjglidebetweenfacingpool.cpp
    src/common/behobjglidelcl.cpp
    src/common/behobjglidelclpool.cpp
    src/common/behobjglidepool.cpp
    src/common/behobjglidetoobjectpool.cpp
    src/common/behobjglidetoposition.cpp
    src/common/behobjglidetopositionpool.cpp
    src/common/behobjheadtracking.cpp
    src/common/behobjheadtrackingpool.cpp
    src/common/behobjmonitorkeyboard.cpp
    src/common/behobjmonitorkeyboardpool.cpp
    src/common/behobjopenandclose.cpp
    src/common/behobjopenandclosepool.cpp
    src/common/behobjrailslide.cpp
    src/common/behobjrailslidepool.cpp
    src/common/behobjsetflatinplace.cpp
    src/common/behobjsetflatinplacepool.cpp
    src/common/behobjsetinplace.cpp
    src/common/behobjsetinplacepool.cpp
    src/common/behobjsetmovinginplace.cpp
    src/common/behobjsetmovinginplacepool.cpp
    src/common/behobjtracking.cpp
    src/common/behobjtrackingpool.cpp
    src/common/behobjtrackpassprep.cpp
    src/common/behobjtrackpasspreppool.cpp
    src/common/behobjtracktrajectory.cpp
    src/common/behobjtracktrajectorypool.cpp
    src/common/behobjupdaterelativepos.cpp
    src/common/behobjupdaterelativepospool.cpp
    src/common/behobjupdatewhileattached.cpp
    src/common/behobjupdatewhileattachedpool.cpp
    src/common/behobjwatching.cpp
    src/common/behobjwatchingpool.cpp
    src/common/cadjustaccelerationpool.cpp
    src/common/cavoidobjectspool.cpp
    src/common/cbeattachedpool.cpp
    src/common/cbungeetowardpool.cpp
    src/common/ccheckaipool.cpp
    src/common/ccircletrackpool.cpp
    src/common/cdisplaypassbarpool.cpp
    src/common/cdisplayshotpowerpool.cpp
    src/common/cdopplersndpool.cpp
    src/common/cexplosionanimationpool.cpp
    src/common/cglideonsurfacepool.cpp
    src/common/cheadtrackpool.cpp
    src/common/cmonitorheldkeyspool.cpp
    src/common/cmonitorkeyspool.cpp
    src/common/cpaletteeffectpool.cpp
    src/common/cpansoundpool.cpp
    src/common/cprojectandbouncepool.cpp
    src/common/crelaxtopositionpool.cpp
    src/common/crelaxyawpool.cpp
    src/common/csetattitudepool.cpp
    src/common/csetpositionrelativepool.cpp
    src/common/cslideonrailpool.cpp
    src/common/csteerbetweenfacingpool.cpp
    src/common/csteertopositionpool.cpp
    src/common/csteertotargetpool.cpp
    src/common/csteertowardpool.cpp
    src/common/cstepanimationpool.cpp
    src/common/ctimeractivitypool.cpp
    src/common/ctrackpasserreceiverpool.cpp
    src/common/ctrackpool.cpp
    src/common/ctracktrajpool.cpp
    src/common/cupdateshotpowerpool.cpp
    src/common/cviewitpool.cpp
    src/common/cwatchpool.cpp
    src/common/fileobj.cpp
    src/common/inifile.cpp
    src/common/flatbehaviorobject.cpp
    src/common/flightbehaviorobject.cpp
    src/common/gametime.cpp
    src/common/nodeowner.cpp
    src/common/parseutil.cpp
    src/common/pak.cpp
    src/common/path.cpp
    src/common/poolcountsdef.cpp
    src/common/pooledownerroot.cpp
    src/common/staticinitglobals.cpp
    src/common/string.cpp
    src/common/stringtable.cpp
    src/common/stringpool.cpp
    src/common/stub.cpp
    src/common/surfacebehaviorobject.cpp
    src/common/util.cpp
    src/engine/arenaellipsoidstatic.cpp
    src/engine/cadjustacceleration.cpp
    src/engine/cameraobject.cpp
    src/engine/cameraprojectionstate.cpp
    src/engine/cbeattached.cpp
    src/engine/ccheckai.cpp
    src/engine/ccircletrack.cpp
    src/engine/cdisplaypassbar.cpp
    src/engine/cdisplayshotpower.cpp
    src/engine/cexplosionanimation.cpp
    src/engine/cmonitorheldkeys.cpp
    src/engine/coachai.cpp
    src/engine/coachobject.cpp
    src/engine/cpansound.cpp
    src/engine/crelaxyaw.cpp
    src/engine/csetattitude.cpp
    src/engine/csetpositionrelative.cpp
    src/engine/cspeedburst.cpp
    src/engine/csteertoposition.cpp
    src/engine/ctrack.cpp
    src/engine/ctrackpasserreceiver.cpp
    src/engine/cwatch.cpp
    src/engine/dromeobject.cpp
    src/engine/engineobject.cpp
    src/engine/framepacing.cpp
    src/engine/game.cpp
    src/engine/gameengine.cpp
    src/engine/gameoptions.cpp
    src/engine/gamepaddeviceobj.cpp
    src/engine/goalobject.cpp
    src/engine/inplacebehaviorobject.cpp
    src/engine/inputdeviceobj.cpp
    src/engine/joystickdeviceobj.cpp
    src/engine/matchstats.cpp
    src/engine/mousedeviceobj.cpp
    src/engine/msgevent.cpp
    src/engine/object.cpp
    src/engine/objectbase.cpp
    src/engine/objectdef.cpp
    src/engine/objectstats.cpp
    src/engine/particle.cpp
    src/engine/particlesystem.cpp
    src/engine/player.cpp
    src/engine/playerobject.cpp
    src/engine/rampobject.cpp
    src/engine/rokobject.cpp
    src/engine/spectator.cpp
    src/engine/teamblock.cpp
    src/engine/teamsblock.cpp
    src/engine/trackingellipsoidstatics.cpp
    src/engine/weaponobject.cpp
    src/globals.cpp
    src/graphics/cd3dtexturemanager.cpp
    src/graphics/cddtext.cpp
    src/graphics/cgpalette.cpp
    src/graphics/clipsphere.cpp
    src/graphics/collisioncage.cpp
    src/graphics/colorpalette.cpp
    src/graphics/display.cpp
    src/graphics/face.cpp
    src/graphics/gdi.cpp
    src/graphics/graphics.cpp
    src/graphics/infobarradar.cpp
    src/graphics/graphicssettings.cpp
    src/graphics/hudlayer.cpp
    src/graphics/hwmesh.cpp
    src/graphics/hwmodels.cpp
    src/graphics/hwpipeline.cpp
    src/graphics/hwrenderer.cpp
    src/graphics/hwtexture.cpp
    src/graphics/infobarslot.cpp
    src/graphics/lightattenuation.cpp
    src/graphics/lodnode.cpp
    src/graphics/mdm1.cpp
    src/graphics/meshbone.cpp
    src/graphics/meshboundingbox.cpp
    src/graphics/meshmaterial.cpp
    src/graphics/modelcomponent.cpp
    src/graphics/modeldatabase.cpp
    src/graphics/modelfile.cpp
    src/graphics/palettefade.cpp
    src/graphics/palettescalarfade.cpp
    src/graphics/prompticons.cpp
    src/graphics/presenter.cpp
    src/graphics/renderer.cpp
    src/graphics/rlespritesurface.cpp
    src/graphics/softrenderer.cpp
    src/graphics/softtex.cpp
    src/graphics/stbimpl.cpp
    src/graphics/swrend.cpp
    src/graphics/text.cpp
    src/graphics/texcacheentry.cpp
    src/graphics/visualeffectfade.cpp
    src/input/actionbindings.cpp
    src/input/cheat.cpp
    src/input/deviceobject.cpp
    src/input/input.cpp
    src/input/inputlog.cpp
    src/input/keymap.cpp
    src/input/gamepad.cpp
    src/input/keyboarddeviceobj.cpp
    src/math/math.cpp
    src/math/point.cpp
    src/math/quat.cpp
    src/math/slerpseg.cpp
    src/math/spline.cpp
    src/math/splinecurve.cpp
    src/math/transformobject.cpp
    src/math/vec3.cpp
    src/math/viewextent.cpp
    src/net/movecodec.cpp
    src/net/netstub.cpp
    src/physics/boundbubble.cpp
    src/physics/bubblecmd.cpp
    src/physics/capsule.cpp
    src/physics/collinteractorpool.cpp
    src/physics/collisioncheck.cpp
    src/physics/collisionellipsoid.cpp
    src/physics/collisionobj.cpp
    src/physics/collisionplane.cpp
    src/physics/collisionshape.cpp
    src/physics/msginteractorpool.cpp
    src/physics/physicsbody.cpp
    src/planvm/agendaitem.cpp
    src/planvm/baseresetagendaitem.cpp
    src/planvm/behaviorregistry.cpp
    src/planvm/behobjglidetoobject.cpp
    src/planvm/behparam.cpp
    src/planvm/bubblenode.cpp
    src/planvm/cameraliteagendaitem.cpp
    src/planvm/cavoidobjects.cpp
    src/planvm/cbungeetoward.cpp
    src/planvm/cdopplersnd.cpp
    src/planvm/cglideonsurface.cpp
    src/planvm/cheadtrack.cpp
    src/planvm/cmonitorkeys.cpp
    src/planvm/collinteractor.cpp
    src/planvm/cpaletteeffect.cpp
    src/planvm/cprojectandbounce.cpp
    src/planvm/crelaxtoposition.cpp
    src/planvm/cslideonrail.cpp
    src/planvm/cspeedburstpool.cpp
    src/planvm/csteerbetweenfacing.cpp
    src/planvm/csteertotarget.cpp
    src/planvm/csteertoward.cpp
    src/planvm/cstepanimation.cpp
    src/planvm/ctimeractivity.cpp
    src/planvm/ctracktraj.cpp
    src/planvm/cupdateshotpower.cpp
    src/planvm/cviewit.cpp
    src/planvm/gamebehaviorobject.cpp
    src/planvm/interactor.cpp
    src/planvm/kindagendaitem.cpp
    src/planvm/msginteractor.cpp
    src/planvm/planaction.cpp
    src/planvm/planactionanim.cpp
    src/planvm/planactionlaunch.cpp
    src/planvm/planactionrecoil.cpp
    src/planvm/planactiontracking.cpp
    src/planvm/planarg.cpp
    src/planvm/plancond.cpp
    src/planvm/planfile.cpp
    src/planvm/planfocusarg.cpp
    src/planvm/planinteractorfactories.cpp
    src/planvm/planstate.cpp
    src/planvm/planutil.cpp
    src/planvm/planvm.cpp
    src/planvm/releasingagendaitem.cpp
    src/sound/audiomgr.cpp
    src/sound/soundmanager.cpp
    src/sound/soundmixer.cpp
    src/sound/wave.cpp
    src/ui/buttonwidget.cpp
    src/ui/cddsurfaceowner.cpp
    src/ui/debughud.cpp
    src/ui/debugmenu.cpp
    src/ui/displaywidget.cpp
    src/ui/flipbookwidget.cpp
    src/ui/holdingwidget.cpp
    src/ui/inertwidget.cpp
    src/ui/infobar.cpp
    src/ui/infobarbase.cpp
    src/ui/jumbotextlist.cpp
    src/ui/loopflipbookwidget.cpp
    src/ui/mainwindow.cpp
    src/ui/messagequeue.cpp
    src/ui/pausemenu.cpp
    src/ui/saveobject.cpp
    src/ui/screen.cpp
    src/ui/screens/awardsuiscreen.cpp
    src/ui/screens/championshipsetupuiscreen.cpp
    src/ui/screens/dromeexteriorscreen.cpp
    src/ui/screens/finalceremonyscreen.cpp
    src/ui/screens/gameconfiguiscreen.cpp
    src/ui/screens/gameintroscreen.cpp
    src/ui/screens/headtoheadsetupuiscreen.cpp
    src/ui/screens/jumbotronscreen.cpp
    src/ui/screens/keyconfigscreen.cpp
    src/ui/screens/leaguesetupuiscreen.cpp
    src/ui/screens/lobbyscreen.cpp
    src/ui/screens/mainmenuscreen.cpp
    src/ui/screens/matchinfouiscreen.cpp
    src/ui/screens/multisetupoptionsscreen.cpp
    src/ui/screens/pausemenufullscreen.cpp
    src/ui/screens/pausemenuminimalscreen.cpp
    src/ui/screens/pausemenunosavescreen.cpp
    src/ui/screens/saveloaduiscreen.cpp
    src/ui/screens/teamsetupuiscreen.cpp
    src/ui/screens/tournamentsetupuiscreen.cpp
    src/ui/screens/trainingendscreen.cpp
    src/ui/signflipbookwidget.cpp
    src/ui/textinputwidget.cpp
    src/ui/uiscreen.cpp
    src/ui/widget.cpp)

if(WIN32)
    list(APPEND HYPERX_SOURCES src/app/cddrive_win32.cpp)
else()
    list(APPEND HYPERX_SOURCES src/app/cddrive_stub.cpp)
endif()

add_library(hyperx_objects OBJECT ${HYPERX_SOURCES})
target_link_libraries(hyperx_objects PRIVATE hyperx_options hyperx_permissive SDL3::SDL3 miniaudio imgui stb)

set(HYPERX_SHADERS shaders/mesh.vert.hlsl shaders/mesh.frag.hlsl shaders/shadow.vert.hlsl shaders/shadow.frag.hlsl)
hyperx_add_shaders(hyperx_objects ${HYPERX_SHADERS})

# Named apart from HYPERX.EXE so it can sit beside it in a HyperBlade folder.
add_executable(hyperx $<TARGET_OBJECTS:hyperx_objects>)
set_target_properties(hyperx PROPERTIES
    OUTPUT_NAME hyperblade_20XX
    RUNTIME_OUTPUT_DIRECTORY "${HYPERX_OUT}")
target_link_libraries(hyperx PRIVATE SDL3::SDL3 miniaudio imgui)
hyperx_copy_runtime_dlls(hyperx "${HYPERX_OUT}")

if(WIN32)
    set_target_properties(hyperx PROPERTIES WIN32_EXECUTABLE ON)
    target_sources(hyperx PRIVATE res/hyperblade_20XX.rc)
endif()

if(MSVC)
    target_link_options(hyperx PRIVATE "/MAP:${HYPERX_OUT}/hyperblade_20XX.map")
endif()

# Our own assets, which the game reads from the pak beside it.
set(HYPERX_PAK_FILES
    fonts/PixelOperator.ttf
    fonts/PixelOperator8.ttf
    fonts/PixelOperator-LICENSE.txt
    prompts/License.txt
    prompts/xbox_button_back.png
    prompts/xbox_button_color_a.png
    prompts/xbox_button_color_b.png
    prompts/xbox_button_color_x.png
    prompts/xbox_button_color_y.png
    prompts/xbox_button_start.png
    prompts/xbox_dpad_down.png
    prompts/xbox_dpad_left.png
    prompts/xbox_dpad_right.png
    prompts/xbox_dpad_up.png
    prompts/xbox_lb.png
    prompts/xbox_lt.png
    prompts/xbox_rb.png
    prompts/xbox_rt.png
    prompts/xbox_stick_l_down.png
    prompts/xbox_stick_l_left.png
    prompts/xbox_stick_l_right.png
    prompts/xbox_stick_l_up.png
)
hyperx_add_pak(hyperx "${HYPERX_OUT}/${HYPERX_PAK_NAME}" ${HYPERX_PAK_FILES})

hyperx_link_game_data(hyperx)

enable_testing()

# The unit checks need nothing from the game.
hyperx_add_check(ini LABELS unit SOURCES tests/ini_check.cpp src/common/inifile.cpp src/common/path.cpp)
hyperx_add_check(strings LABELS unit SOURCES tests/strings_check.cpp src/common/stringtable.cpp src/common/path.cpp)
hyperx_add_check(path LABELS unit SOURCES tests/path_check.cpp src/common/path.cpp)
hyperx_add_check(hwmesh LABELS unit PERMISSIVE SOURCES tests/hwmesh_check.cpp src/graphics/hwmesh.cpp)
hyperx_add_check(keymap LABELS unit SOURCES tests/keymap_check.cpp src/input/keymap.cpp LIBRARIES SDL3::SDL3)
hyperx_add_check(pak LABELS unit
    SOURCES tests/pak_check.cpp src/common/pak.cpp src/graphics/stbimpl.cpp
    LIBRARIES stb
    DEPENDS hyperx-pak
    ARGS "${HYPERX_OUT}/${HYPERX_PAK_NAME}" "${PROJECT_SOURCE_DIR}/res" ${HYPERX_PAK_FILES})

# Draws a lit triangle through the mesh pipeline and reads it back; skipped without a GPU.
hyperx_add_check(hwpipeline LABELS gpu
    SOURCES tests/hwpipeline_check.cpp src/graphics/hwpipeline.cpp
    LIBRARIES SDL3::SDL3
    SHADERS ${HYPERX_SHADERS})

# Every MDM1 bitmap and every wave in the game's Data, read back.
hyperx_add_check(mdm1 LABELS data DATA PERMISSIVE
    SOURCES tests/mdm1_check.cpp src/graphics/mdm1.cpp
    LIBRARIES SDL3::SDL3
    ARGS "${HYPERX_GAME_DATA}/Data")
hyperx_add_check(wave LABELS data DATA
    SOURCES tests/wave_check.cpp src/sound/wave.cpp src/common/path.cpp
    ARGS "${HYPERX_GAME_DATA}/Data")

include(cmake/package.cmake)
