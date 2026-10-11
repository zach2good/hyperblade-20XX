// Asserts every class size the original's own `new` calls give, as tools/object_sizes.py reports them.

#include "common/compat.h"
#include "common/staticassert.h"
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

STATIC_ASSERT(sizeof(AnimClipEntry) == 0x24);
STATIC_ASSERT(sizeof(AnimClipSet) == 0x20);
STATIC_ASSERT(sizeof(AnimNamedRef) == 0x8);
STATIC_ASSERT(sizeof(AnimNode) == 0x34);
STATIC_ASSERT(sizeof(CSteerBetweenFacing) == 0x28);
STATIC_ASSERT(sizeof(ArenaState) == 0x20);
STATIC_ASSERT(sizeof(CBeAttached) == 0x24);
STATIC_ASSERT(sizeof(BehObj_Avoidance) == 0xa4);
STATIC_ASSERT(sizeof(BehObj_CandyTracking) == 0x98);
STATIC_ASSERT(sizeof(BehObj_CoachTeam) == 0x98);
STATIC_ASSERT(sizeof(BehObj_DoNothing) == 0x98);
STATIC_ASSERT(sizeof(BehObj_Fly) == 0x98);
STATIC_ASSERT(sizeof(BehObj_FlyLCL) == 0x98);
STATIC_ASSERT(sizeof(BehObj_FlyToward) == 0x98);
STATIC_ASSERT(sizeof(BehObj_Glide) == 0x98);
STATIC_ASSERT(sizeof(BehObj_GlideBetweenFacing) == 0xa8);
STATIC_ASSERT(sizeof(BehObj_GlideLCL) == 0x98);
STATIC_ASSERT(sizeof(BehObj_GlideToObject) == 0x98);
STATIC_ASSERT(sizeof(BehObj_GlideToPosition) == 0xa4);
STATIC_ASSERT(sizeof(BehObj_HeadTracking) == 0x98);
STATIC_ASSERT(sizeof(BehObj_MonitorKeyboard) == 0x98);
STATIC_ASSERT(sizeof(BehObj_OpenAndClose) == 0x98);
STATIC_ASSERT(sizeof(BehObj_RailSlide) == 0x98);
STATIC_ASSERT(sizeof(BehObj_SetFlatInPlace) == 0x98);
STATIC_ASSERT(sizeof(BehObj_SetInPlace) == 0x98);
STATIC_ASSERT(sizeof(BehObj_SetMovingInPlace) == 0x98);
STATIC_ASSERT(sizeof(BehObj_TrackPassPrep) == 0x98);
STATIC_ASSERT(sizeof(BehObj_TrackTrajectory) == 0x98);
STATIC_ASSERT(sizeof(BehObj_Tracking) == 0x98);
STATIC_ASSERT(sizeof(BehObj_UpdateRelativePos) == 0x98);
STATIC_ASSERT(sizeof(BehObj_UpdateWhileAttached) == 0x98);
STATIC_ASSERT(sizeof(BehObj_Watching) == 0x98);
STATIC_ASSERT(sizeof(RokObject) == 0xd30);
STATIC_ASSERT(sizeof(CProjectAndBounce) == 0x4c);
STATIC_ASSERT(sizeof(ButtonWidget) == 0x68);
STATIC_ASSERT(sizeof(FramedBarBitmap) == 0x110);
STATIC_ASSERT(sizeof(CDDTextSurface) == 0xd0);
STATIC_ASSERT(sizeof(CPlayer) == 0x2518);
STATIC_ASSERT(sizeof(CameraObject) == 0x1030);
STATIC_ASSERT(sizeof(CameraProjectionState) == 0x118);
STATIC_ASSERT(sizeof(CWatch) == 0x10);
STATIC_ASSERT(sizeof(CCircleTrack) == 0x1c);
STATIC_ASSERT(sizeof(CollInteractor) == 0x34);
STATIC_ASSERT(sizeof(CCheckAI) == 0x10);
STATIC_ASSERT(sizeof(ColorPalette) == 0x474);
STATIC_ASSERT(sizeof(MsgInteractor) == 0x34);
STATIC_ASSERT(sizeof(CDisplayPassBar) == 0x10);
STATIC_ASSERT(sizeof(CDisplayShotPower) == 0x10);
STATIC_ASSERT(sizeof(DisplayWidget) == 0x68);
STATIC_ASSERT(sizeof(CDopplerSnd) == 0x1c);
STATIC_ASSERT(sizeof(EngineObject) == 0xd14);
STATIC_ASSERT(sizeof(FlipbookWidget) == 0x68);
STATIC_ASSERT(sizeof(GameSession) == 0xfc);
STATIC_ASSERT(sizeof(CoachObject) == 0xd14);
STATIC_ASSERT(sizeof(CHeadTrack) == 0x1c);
STATIC_ASSERT(sizeof(CSteerToward) == 0x44);
STATIC_ASSERT(sizeof(CTrackTraj) == 0x28);
STATIC_ASSERT(sizeof(CSlideOnRail) == 0x40);
STATIC_ASSERT(sizeof(KeyboardDeviceObj) == 0xe78);
STATIC_ASSERT(sizeof(KeyConfigState) == 0x10);
STATIC_ASSERT(sizeof(GamepadDeviceObj) == 0xd28);
STATIC_ASSERT(sizeof(JoystickDeviceObj) == 0xd28);
STATIC_ASSERT(sizeof(MouseDeviceObj) == 0xd28);
STATIC_ASSERT(sizeof(LoopFlipbookWidget) == 0x6c);
STATIC_ASSERT(sizeof(MaterialListNode) == 0x14);
STATIC_ASSERT(sizeof(MessageNode) == 0x10);
STATIC_ASSERT(sizeof(DromeObject) == 0xd14);
STATIC_ASSERT(sizeof(CStepAnimation) == 0x10);
STATIC_ASSERT(sizeof(CMonitorHeldKeys) == 0x10);
STATIC_ASSERT(sizeof(GoalObject) == 0xd18);
STATIC_ASSERT(sizeof(CSetAttitude) == 0x24);
STATIC_ASSERT(sizeof(PaletteFade) == 0x30);
STATIC_ASSERT(sizeof(PaletteScalarFade) == 0x38);
STATIC_ASSERT(sizeof(CPanSound) == 0x20);
STATIC_ASSERT(sizeof(ObjectDef) == 0x60);
STATIC_ASSERT(sizeof(CExplosionAnimation) == 0x10);
STATIC_ASSERT(sizeof(PhysicsBody) == 0x150);
STATIC_ASSERT(sizeof(CGlideOnSurface) == 0x68);
STATIC_ASSERT(sizeof(PlanVM) == 0x2289c);
STATIC_ASSERT(sizeof(PlayerObject) == 0xff4);
STATIC_ASSERT(sizeof(RampObject) == 0xe88);
STATIC_ASSERT(sizeof(CSteerToTarget) == 0x1c);
STATIC_ASSERT(sizeof(CRelaxToPosition) == 0x24);
STATIC_ASSERT(sizeof(ParticleSystem) == 0x2c5c);
STATIC_ASSERT(sizeof(CSteerToPosition) == 0x34);
STATIC_ASSERT(sizeof(CSetPositionRelative) == 0x2c);
STATIC_ASSERT(sizeof(SignFlipbookWidget) == 0x68);
STATIC_ASSERT(sizeof(WeaponObject) == 0xd1c);
STATIC_ASSERT(sizeof(SplineCurve) == 0x30);
STATIC_ASSERT(sizeof(CRelaxYaw) == 0x18);
STATIC_ASSERT(sizeof(CBungeeToward) == 0x34);
STATIC_ASSERT(sizeof(String) == 0x4);
STATIC_ASSERT(sizeof(CAvoidObjects) == 0x30);
STATIC_ASSERT(sizeof(TexCacheEntry) == 0x18);
STATIC_ASSERT(sizeof(TextInputWidget) == 0x74);
STATIC_ASSERT(sizeof(CTimerActivity) == 0x20);
STATIC_ASSERT(sizeof(CTrack) == 0x34);
STATIC_ASSERT(sizeof(CTrackPasser_Receiver) == 0x28);
STATIC_ASSERT(sizeof(TransformObject) == 0x7c);
STATIC_ASSERT(sizeof(CMonitorKeys) == 0x20);
STATIC_ASSERT(sizeof(CUpdateShotPower) == 0x20);
STATIC_ASSERT(sizeof(CAdjustAcceleration) == 0x20);
STATIC_ASSERT(sizeof(CViewIt) == 0x10);
STATIC_ASSERT(sizeof(VisualEffectFade) == 0x30);
STATIC_ASSERT(sizeof(CPaletteEffect) == 0x1c);
STATIC_ASSERT(sizeof(JoystickDevice) == 0x214);
