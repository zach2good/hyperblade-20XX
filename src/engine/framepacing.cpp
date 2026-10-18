#include "common/compat.h"
#include "common/clock.h"
#include "engine/framepacing.h"
#include "engine/cameraobject.h"
#include "engine/cameraprojectionstate.h"
#include "engine/engineobject.h"
#include "engine/game.h"
#include "engine/gameengine.h"
#include "graphics/meshbone.h"
#include "graphics/transparentqueue.h"
#include "globals.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace
{

// ponytail: a jump this far between ticks is a teleport or a camera cut, so it is not blended.
// Tune it against the arena's scale if a fast skater ever snaps.
constexpr auto kSnapDistanceSq = 64.0f * 64.0f;

// A slow frame owes this many ticks at most; the rest of the time is dropped.
constexpr auto kMaxCatchUp = 4;

using Rows = std::array<float, 12>;

struct ObjectPose
{
    std::vector<Rows> prev;
    std::vector<Rows> curr;
    unsigned int      nTick = 0;
};

struct CameraPose
{
    std::array<float, 16> prev    = {};
    std::array<float, 16> curr    = {};
    Vec3                  prevEye = {};
    Vec3                  currEye = {};
    unsigned int          nTick   = 0;
};

// What ViewIt pushed and found visible, so the frame draws what the tick saw at that moment.
struct View
{
    CameraObject*              pCamera = nullptr;
    std::vector<MeshBone*>     components;
    std::vector<EngineObject*> visible;
};

std::unordered_map<const EngineObject*, ObjectPose> s_objects;
std::unordered_map<CameraObject*, CameraPose>       s_cameras;
std::vector<View>                                   s_views;
unsigned int                                        s_nTick = 0;

Rows Read(const TransformObject& transform)
{
    auto rows = Rows{};

    std::memcpy(rows.data(), transform.m_a, sizeof(rows));

    return rows;
}

void Write(TransformObject& transform, const Rows& rows)
{
    std::memcpy(transform.m_a, rows.data(), sizeof(rows));
}

float DistanceSq(float flX, float flY, float flZ)
{
    return flX * flX + flY * flY + flZ * flZ;
}

// Each axis is lerped and set back to its lerped length, so the bone neither shrinks nor shears
// much over the small turn one tick makes.
Rows Blend(const Rows& a, const Rows& b, float flAlpha)
{
    auto out = Rows{};

    for (int nColumn = 0; nColumn < 3; nColumn++)
    {
        const auto flLengthA = std::sqrt(DistanceSq(a[nColumn], a[4 + nColumn], a[8 + nColumn]));
        const auto flLengthB = std::sqrt(DistanceSq(b[nColumn], b[4 + nColumn], b[8 + nColumn]));

        for (int nRow = 0; nRow < 3; nRow++)
        {
            const auto nIndex = nRow * 4 + nColumn;

            out[nIndex] = a[nIndex] + (b[nIndex] - a[nIndex]) * flAlpha;
        }

        const auto flLength = std::sqrt(DistanceSq(out[nColumn], out[4 + nColumn], out[8 + nColumn]));

        if (flLength > 0.0f)
        {
            const auto flScale = (flLengthA + (flLengthB - flLengthA) * flAlpha) / flLength;

            for (int nRow = 0; nRow < 3; nRow++)
            {
                out[nRow * 4 + nColumn] *= flScale;
            }
        }
    }

    for (int nRow = 0; nRow < 3; nRow++)
    {
        const auto nIndex = nRow * 4 + 3;

        out[nIndex] = a[nIndex] + (b[nIndex] - a[nIndex]) * flAlpha;
    }

    return out;
}

void SnapshotObjects()
{
    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        const EngineObject* const pObject = g_pGameEngine->m_ppObjects[i];

        auto& pose = s_objects[pObject];

        const auto nBones = static_cast<size_t>(pObject->m_wBoneCount);

        pose.prev.swap(pose.curr);
        pose.curr.resize(nBones);

        for (size_t nBone = 0; nBone < nBones; nBone++)
        {
            pose.curr[nBone] = Read(pObject->m_pBones[nBone].m_world);
        }

        const auto bJumped = [&]
        {
            if (pose.prev.size() != nBones || pose.nTick + 1 != s_nTick || nBones == 0)
            {
                return true;
            }

            const auto& a = pose.prev[0];
            const auto& b = pose.curr[0];

            return DistanceSq(b[3] - a[3], b[7] - a[7], b[11] - a[11]) > kSnapDistanceSq;
        }();

        if (bJumped)
        {
            pose.prev = pose.curr;
        }

        pose.nTick = s_nTick;
    }

    std::erase_if(
        s_objects,
        [](const auto& entry)
        {
            return entry.second.nTick != s_nTick;
        });
}

void SnapshotCamera(CameraObject& camera)
{
    auto& pose = s_cameras[&camera];

    const CameraProjectionState& view = *camera.m_pView;

    const auto bFresh = pose.nTick + 1 != s_nTick;

    if (pose.nTick != s_nTick)
    {
        pose.prev    = pose.curr;
        pose.prevEye = pose.currEye;
    }

    std::memcpy(pose.curr.data(), view.m_a, sizeof(pose.curr));

    pose.currEye = view.m_vEye;

    const auto bCut = DistanceSq(pose.currEye.x - pose.prevEye.x, pose.currEye.y - pose.prevEye.y, pose.currEye.z - pose.prevEye.z) > kSnapDistanceSq;

    if (bFresh || bCut)
    {
        pose.prev    = pose.curr;
        pose.prevEye = pose.currEye;
    }

    pose.nTick = s_nTick;
}

void PlaceObjects(float flAlpha)
{
    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        auto* const pObject = g_pGameEngine->m_ppObjects[i];

        const auto it = s_objects.find(pObject);

        if (it == s_objects.end() || it->second.curr.size() != static_cast<size_t>(pObject->m_wBoneCount))
        {
            continue;
        }

        const auto& pose = it->second;

        for (size_t nBone = 0; nBone < pose.curr.size(); nBone++)
        {
            Write(pObject->m_pBones[nBone].m_world, Blend(pose.prev[nBone], pose.curr[nBone], flAlpha));
        }
    }
}

// The bones as the tick left them, which may be later than the pose ViewIt saw.
void SaveLive(std::vector<Rows>& live)
{
    live.clear();

    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        const EngineObject* const pObject = g_pGameEngine->m_ppObjects[i];

        for (short nBone = 0; nBone < pObject->m_wBoneCount; nBone++)
        {
            live.push_back(Read(pObject->m_pBones[nBone].m_world));
        }
    }
}

void RestoreLive(const std::vector<Rows>& live)
{
    size_t nNext = 0;

    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        auto* const pObject = g_pGameEngine->m_ppObjects[i];

        for (short nBone = 0; nBone < pObject->m_wBoneCount; nBone++)
        {
            Write(pObject->m_pBones[nBone].m_world, live[nNext++]);
        }
    }
}

// ponytail: the view includes the projection, so it is lerped as a whole matrix. The turn
// between ticks is small; decompose it if a fast pan ever looks squashed.
void PlaceCamera(CameraObject& camera, float flAlpha)
{
    const auto& pose = s_cameras.at(&camera);

    CameraProjectionState& view = *camera.m_pView;

    auto* const pOut = &view.m_a[0][0];

    for (size_t i = 0; i < pose.curr.size(); i++)
    {
        pOut[i] = pose.prev[i] + (pose.curr[i] - pose.prev[i]) * flAlpha;
    }

    view.m_vEye.x = pose.prevEye.x + (pose.currEye.x - pose.prevEye.x) * flAlpha;
    view.m_vEye.y = pose.prevEye.y + (pose.currEye.y - pose.prevEye.y) * flAlpha;
    view.m_vEye.z = pose.prevEye.z + (pose.currEye.z - pose.prevEye.z) * flAlpha;
}

} // namespace

static auto s_dNowMs = 0.0;

static auto s_nRateTicks  = 0;
static auto s_nRate       = 0;
static auto s_rateStart   = std::chrono::steady_clock::time_point{};
static auto s_dDriftStart = 0.0;
static auto s_flDrift     = 0.0f;

void FramePacing::BeginTick()
{
    s_views.clear();
    s_nTick++;

    s_dNowMs += kTickMs;

    const auto now = std::chrono::steady_clock::now();

    if (s_rateStart == std::chrono::steady_clock::time_point{} || now - s_rateStart > std::chrono::seconds{ 5 })
    {
        s_rateStart   = now;
        s_dDriftStart = s_dNowMs;
        s_nRateTicks  = 0;
    }

    s_nRateTicks++;

    if (now - s_rateStart >= std::chrono::seconds{ 1 })
    {
        s_flDrift     = static_cast<float>((s_dNowMs - s_dDriftStart) - Clock::FractionalMilliseconds(now - s_rateStart).count());
        s_nRate       = s_nRateTicks;
        s_rateStart   = now;
        s_dDriftStart = s_dNowMs;
        s_nRateTicks  = 0;
    }
}

int FramePacing::TicksPerSecond()
{
    return s_nRate;
}

float FramePacing::DriftMs()
{
    return s_flDrift;
}

void FramePacing::RecordView(CameraObject& camera)
{
    // The first view of a tick keeps the pose it drew, as the original drew it there.
    if (s_views.empty())
    {
        SnapshotObjects();
    }

    SnapshotCamera(camera);

    auto it = std::find_if(
        s_views.begin(),
        s_views.end(),
        [&](const View& view)
        {
            return view.pCamera == &camera;
        });

    if (it == s_views.end())
    {
        it = s_views.insert(s_views.end(), View{ &camera });
    }

    it->components.clear();
    it->visible.clear();

    for (short i = 1; i <= g_nTransparentCount; i++)
    {
        it->components.push_back(g_pTransparentQueue[i].pComponent);
    }

    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        auto* const pObject = g_pGameEngine->m_ppObjects[i];

        if (pObject->IsVisible() != 0)
        {
            it->visible.push_back(pObject);
        }
    }
}

float FramePacing::RunTicks()
{
    constexpr auto kTick = std::chrono::nanoseconds{ std::chrono::seconds{ 1 } } / 30;

    static auto s_last = std::chrono::steady_clock::time_point{};
    static auto s_owed = std::chrono::nanoseconds{};

    const auto now = std::chrono::steady_clock::now();

    // After a gap, such as the menus or the capped loop, the next frame starts on a fresh tick.
    if (s_last == std::chrono::steady_clock::time_point{} || now - s_last > kTick * kMaxCatchUp)
    {
        s_owed = kTick;
    }
    else
    {
        s_owed += now - s_last;
    }

    s_last = now;

    for (int nTick = 0; nTick < kMaxCatchUp && s_owed >= kTick; nTick++)
    {
        Game::Tick();

        s_owed -= kTick;
    }

    s_owed = (std::min)(s_owed, kTick);

    return std::chrono::duration<float>(s_owed) / std::chrono::duration<float>(kTick);
}

void FramePacing::DrawViews(float flAlpha)
{
    static auto s_live = std::vector<Rows>();

    SaveLive(s_live);
    PlaceObjects(flAlpha);

    for (const View& view : s_views)
    {
        for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
        {
            g_pGameEngine->m_ppObjects[i]->SetVisible(0);
        }

        for (EngineObject* const pObject : view.visible)
        {
            pObject->SetVisible(1);
        }

        CameraProjectionState& camera = *view.pCamera->m_pView;

        const auto liveView = Read(camera);
        const auto liveRow3 = std::array<float, 4>{ camera.m_a[3][0], camera.m_a[3][1], camera.m_a[3][2], camera.m_a[3][3] };
        const auto liveEye  = camera.m_vEye;

        PlaceCamera(*view.pCamera, flAlpha);

        g_nTransparentCount = 0;

        for (MeshBone* const pComponent : view.components)
        {
            pComponent->PushToRenderStack(view.pCamera);
        }

        view.pCamera->DrawView();

        Write(camera, liveView);
        std::memcpy(camera.m_a[3], liveRow3.data(), sizeof(liveRow3));
        camera.m_vEye = liveEye;
    }

    RestoreLive(s_live);
}
