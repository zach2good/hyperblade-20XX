// Asserts every class size the original's own `new` calls give, as tools/object_sizes.py reports them.

#include "common/compat.h"
#include "anim/animclipentry.h"
#include "engine/objectdef.h"
#include "anim/animclipset.h"
#include "anim/animnode.h"
#include "common/animnamedref.h"
#include "common/pooledclasses.h"
#include "common/pathstring.h"
#include "engine/arenastate.h"
#include "engine/rokobject.h"
#include "engine/cameraobject.h"
#include "engine/cameraprojectionstate.h"
#include "engine/devicector.h"
#include "engine/engineobject.h"
#include "engine/coachobject.h"
#include "engine/cheadtrack.h"
#include "engine/messagequeue.h"
#include "engine/objectclasses.h"
#include "engine/dromeobject.h"
#include "engine/goalobject.h"
#include "engine/weaponobject.h"
#include "engine/playerobject.h"
#include "engine/rampobject.h"
#include "engine/particlesystem.h"
#include "graphics/framedbarbitmap.h"
#include "graphics/cddtextsurface.h"
#include "graphics/colorpalette.h"
#include "graphics/modeldatabase.h"
#include "graphics/meshmaterial.h"
#include "graphics/palettefade.h"
#include "graphics/palettescalarfade.h"
#include "graphics/texcacheentry.h"
#include "graphics/visualeffectfade.h"
#include "input/keyboarddeviceobj.h"
#include "math/transformobject.h"
#include "math/splinecurve.h"
#include "net/cplayer.h"
#include "net/gamesession.h"
#include "physics/physicsbody.h"
#include "planvm/collinteractor.h"
#include "planvm/msginteractor.h"
#include "planvm/cprojectandbounce.h"
#include "engine/ccircletrack.h"
#include "planvm/csteertotarget.h"
#include "engine/csteertoposition.h"
#include "planvm/csteerbetweenfacing.h"
#include "planvm/cavoidobjects.h"
#include "planvm/cglideonsurface.h"
#include "engine/ctrack.h"
#include "engine/ctrackpasserreceiver.h"
#include "engine/cadjustacceleration.h"
#include "planvm/cbungeetoward.h"
#include "engine/cbeattached.h"
#include "engine/csetpositionrelative.h"
#include "planvm/csteertoward.h"
#include "engine/csetattitude.h"
#include "engine/crelaxyaw.h"
#include "planvm/cstepanimation.h"
#include "planvm/cmonitorkeys.h"
#include "engine/cmonitorheldkeys.h"
#include "planvm/crelaxtoposition.h"
#include "planvm/ctimeractivity.h"
#include "engine/cwatch.h"
#include "planvm/cviewit.h"
#include "engine/cpansound.h"
#include "planvm/cdopplersnd.h"
#include "planvm/cpaletteeffect.h"
#include "engine/cdisplayshotpower.h"
#include "planvm/cupdateshotpower.h"
#include "engine/cexplosionanimation.h"
#include "engine/ccheckai.h"
#include "engine/cdisplaypassbar.h"
#include "planvm/ctracktraj.h"
#include "planvm/cslideonrail.h"
#include "planvm/planvm.h"
#include "ui/buttonwidget.h"
#include "ui/displaywidget.h"
#include "ui/flipbookwidget.h"
#include "ui/keyconfigstate.h"
#include "ui/loopflipbookwidget.h"
#include "ui/signflipbookwidget.h"
#include "ui/textinputwidget.h"
#include "input/joystickdevice.h"

static_assert(sizeof(AnimClipEntry) == 0x24);
static_assert(sizeof(AnimClipSet) == 0x20);
static_assert(sizeof(AnimNamedRef) == 0x8);
static_assert(sizeof(AnimNode) == 0x34);
static_assert(sizeof(CSteerBetweenFacing) == 0x28);
static_assert(sizeof(ArenaState) == 0x20);
static_assert(sizeof(CBeAttached) == 0x24);
static_assert(sizeof(BehObj_Avoidance) == 0xa4);
static_assert(sizeof(BehObj_CandyTracking) == 0x98);
static_assert(sizeof(BehObj_CoachTeam) == 0x98);
static_assert(sizeof(BehObj_DoNothing) == 0x98);
static_assert(sizeof(BehObj_Fly) == 0x98);
static_assert(sizeof(BehObj_FlyLCL) == 0x98);
static_assert(sizeof(BehObj_FlyToward) == 0x98);
static_assert(sizeof(BehObj_Glide) == 0x98);
static_assert(sizeof(BehObj_GlideBetweenFacing) == 0xa8);
static_assert(sizeof(BehObj_GlideLCL) == 0x98);
static_assert(sizeof(BehObj_GlideToObject) == 0x98);
static_assert(sizeof(BehObj_GlideToPosition) == 0xa4);
static_assert(sizeof(BehObj_HeadTracking) == 0x98);
static_assert(sizeof(BehObj_MonitorKeyboard) == 0x98);
static_assert(sizeof(BehObj_OpenAndClose) == 0x98);
static_assert(sizeof(BehObj_RailSlide) == 0x98);
static_assert(sizeof(BehObj_SetFlatInPlace) == 0x98);
static_assert(sizeof(BehObj_SetInPlace) == 0x98);
static_assert(sizeof(BehObj_SetMovingInPlace) == 0x98);
static_assert(sizeof(BehObj_TrackPassPrep) == 0x98);
static_assert(sizeof(BehObj_TrackTrajectory) == 0x98);
static_assert(sizeof(BehObj_Tracking) == 0x98);
static_assert(sizeof(BehObj_UpdateRelativePos) == 0x98);
static_assert(sizeof(BehObj_UpdateWhileAttached) == 0x98);
static_assert(sizeof(BehObj_Watching) == 0x98);
static_assert(sizeof(RokObject) == 0xd30);
static_assert(sizeof(CProjectAndBounce) == 0x4c);
static_assert(sizeof(ButtonWidget) == 0x68);
static_assert(sizeof(FramedBarBitmap) == 0x110);
static_assert(sizeof(CDDTextSurface) == 0xd0);
static_assert(sizeof(CPlayer) == 0x2518);
static_assert(sizeof(CameraObject) == 0x1030);
static_assert(sizeof(CameraProjectionState) == 0x118);
static_assert(sizeof(CWatch) == 0x10);
static_assert(sizeof(CCircleTrack) == 0x1c);
static_assert(sizeof(CollInteractor) == 0x34);
static_assert(sizeof(CCheckAI) == 0x10);
static_assert(sizeof(ColorPalette) == 0x474);
static_assert(sizeof(MsgInteractor) == 0x34);
static_assert(sizeof(CDisplayPassBar) == 0x10);
static_assert(sizeof(CDisplayShotPower) == 0x10);
static_assert(sizeof(DisplayWidget) == 0x68);
static_assert(sizeof(CDopplerSnd) == 0x1c);
static_assert(sizeof(EngineObject) == 0xd14);
static_assert(sizeof(FlipbookWidget) == 0x68);
static_assert(sizeof(GameSession) == 0xfc);
static_assert(sizeof(CoachObject) == 0xd14);
static_assert(sizeof(CHeadTrack) == 0x1c);
static_assert(sizeof(CSteerToward) == 0x44);
static_assert(sizeof(CTrackTraj) == 0x28);
static_assert(sizeof(CSlideOnRail) == 0x40);
static_assert(sizeof(KeyboardDeviceObj) == 0xe78);
static_assert(sizeof(KeyConfigState) == 0x10);
static_assert(sizeof(GamepadDeviceObj) == 0xd28);
static_assert(sizeof(JoystickDeviceObj) == 0xd28);
static_assert(sizeof(MouseDeviceObj) == 0xd28);
static_assert(sizeof(LoopFlipbookWidget) == 0x6c);
static_assert(sizeof(MaterialListNode) == 0x14);
static_assert(sizeof(MessageNode) == 0x10);
static_assert(sizeof(DromeObject) == 0xd14);
static_assert(sizeof(CStepAnimation) == 0x10);
static_assert(sizeof(CMonitorHeldKeys) == 0x10);
static_assert(sizeof(GoalObject) == 0xd18);
static_assert(sizeof(CSetAttitude) == 0x24);
static_assert(sizeof(PaletteFade) == 0x30);
static_assert(sizeof(PaletteScalarFade) == 0x38);
static_assert(sizeof(CPanSound) == 0x20);
static_assert(sizeof(ObjectDef) == 0x60);
static_assert(sizeof(CExplosionAnimation) == 0x10);
static_assert(sizeof(PhysicsBody) == 0x150);
static_assert(sizeof(CGlideOnSurface) == 0x68);
static_assert(sizeof(PlanVM) == 0x2289c);
static_assert(sizeof(PlayerObject) == 0xff4);
static_assert(sizeof(RampObject) == 0xe88);
static_assert(sizeof(CSteerToTarget) == 0x1c);
static_assert(sizeof(CRelaxToPosition) == 0x24);
static_assert(sizeof(ParticleSystem) == 0x2c5c);
static_assert(sizeof(CSteerToPosition) == 0x34);
static_assert(sizeof(CSetPositionRelative) == 0x2c);
static_assert(sizeof(SignFlipbookWidget) == 0x68);
static_assert(sizeof(WeaponObject) == 0xd1c);
static_assert(sizeof(SplineCurve) == 0x30);
static_assert(sizeof(CRelaxYaw) == 0x18);
static_assert(sizeof(CBungeeToward) == 0x34);
static_assert(sizeof(String) == 0x4);
static_assert(sizeof(CAvoidObjects) == 0x30);
static_assert(sizeof(TexCacheEntry) == 0x18);
static_assert(sizeof(TextInputWidget) == 0x74);
static_assert(sizeof(CTimerActivity) == 0x20);
static_assert(sizeof(CTrack) == 0x34);
static_assert(sizeof(CTrackPasser_Receiver) == 0x28);
static_assert(sizeof(TransformObject) == 0x7c);
static_assert(sizeof(CMonitorKeys) == 0x20);
static_assert(sizeof(CUpdateShotPower) == 0x20);
static_assert(sizeof(CAdjustAcceleration) == 0x20);
static_assert(sizeof(CViewIt) == 0x10);
static_assert(sizeof(VisualEffectFade) == 0x30);
static_assert(sizeof(CPaletteEffect) == 0x1c);
static_assert(sizeof(JoystickDevice) == 0x214);
