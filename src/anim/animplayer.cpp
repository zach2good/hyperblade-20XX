#include "common/compat.h"
#include "animbinding.h"
#include "engine/objectclasses.h"
#include "animclipset.h"
#include "math/quat.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "animdb.h"
#include "animclipentry.h"
#include "graphics/meshbone.h"
#include "planvm/planvm.h"
#include "animclip.h"
#include "math/transformobject.h"
#include "physics/physicsbody.h"
#include "ui/hudglobals.h"
#include <string.h>
#include "animplayer.h"
#include "engine/playerobject.h"
#include "animnode.h"
#include "engine/engineobject.h"
#include "math/slerpseg.h"
#include <math.h>
#include "anim/animglobals.h"
#include <stdlib.h>
#include "engine/messageids.h"

#define BLEND_NEXT (-1.0f)

#define CLIP_NEXT (-1)

// GLOBAL: HYPERX 0x004bc4fc
extern float g_flBlendDurationCap;

// GLOBAL: HYPERX 0x004b296c
extern const float g_flAnimBlendZero;

// FUNCTION: HYPERX 0x00417510
void AnimPlayer::SetObject(EngineObject* pObject)
{
    m_pObject = pObject;

    if (pObject != 0)
    {
        m_wBoneCount = pObject->GetBoneCount();
    }
}

// The base's table is stored before the blend slot array is built and the derived's after.
// FUNCTION: HYPERX 0x00424490
AnimPlayer::AnimPlayer()
{
    memset(&m_state, 0, sizeof(m_state));

    m_state.flPlaybackRate = g_flOneAnimPlayer;

    memset(&m_saved, 0, sizeof(m_saved));

    m_flDuration = g_flAnimBlendZero;
    m_flScale    = g_flAnimBlendZero;
    m_flAccum    = g_flAnimBlendZero;

    memset(&m_vPos, 0, sizeof(m_vPos));

    memset(&m_vDelta, 0, sizeof(m_vDelta));

    memset(&m_pending, 0, sizeof(m_pending));

    m_bPending = 0;
}

// Loop mode zero loops forever, above zero counts down, below zero (including -2) plays once.

// FUNCTION: HYPERX 0x00424590
char AnimPlayer::UpdateTick(void)
{
    float flDelta;

    if (m_pSet != 0)
    {
        switch (m_state.nPlayState)
        {
            case AA_PLAY:
            case AA_SINGLESTEP:
                if (m_state.nHold == -1)
                {
                    flDelta = g_pGameEngine->m_clock.flDelta;

                    if (Advance(flDelta) == 0)
                    {
                        m_state.nHold = m_pClip->m_nLoopCount;
                    }

                    if (m_state.nPlayState == AA_SINGLESTEP)
                    {
                        m_state.nPlayState = AA_PAUSE;

                        return 1;
                    }
                }
                else if (CheckLoopPoint() != 0)
                {
                    UpdateFrame(0);

                    AnimNode* const pClip = m_pClip;

                    if (pClip->m_wLoopCount > 0)
                    {
                        m_state.wLoops--;

                        if (m_state.wLoops <= 0)
                        {
                            if (OnClipEnd() != 0)
                            {
                                return 1;
                            }

                            return ConditionalDispatch(0);
                        }
                    }

                    if (pClip->m_wLoopCount >= 0)
                    {
                        m_state.nLoopFrame = pClip->m_wStartFrame;
                        m_state.nFrame     = m_state.nLoopFrame;
                        m_nFrameInt        = m_state.nLoopFrame;
                        m_state.flPosition = (float)m_state.nLoopFrame;
                        m_state.nEndFrame  = pClip->m_wEndFrame;

                        if (pClip->m_wLoopCount == 0)
                        {
                            AnimDB* const pSet          = m_pSet;
                            const short   nChannelCount = pSet->GetEntryCount();

                            if (nChannelCount != 0)
                            {
                                for (int iChannel = 0; iChannel < nChannelCount; iChannel++)
                                {
                                    AnimClipEntry* const pRecord = pSet->m_apEntries[(short)iChannel];

                                    if (pRecord->m_wSegment == m_state.wClipIndex)
                                    {
                                        m_state.wEventCursor    = (short)iChannel;
                                        m_state.pEventRecord    = pRecord;
                                        m_state.bChannelPending = 1;
                                        m_state.wEventTrigger   = pRecord->m_wTriggerFrame;
                                        break;
                                    }
                                }
                            }
                        }

                        if (pClip->m_wLoopCount == 0 && m_pObject->GetSlot() < PLAYER_OBJECT_COUNT && ((PlayerObject*)m_pObject)->m_wAnimationSemaphore != 0)
                        {
                            g_pGameEngine->m_ppSlots[(short)m_state.nSlot]
                                ->PostSceneEvent(m_state.nArgA, m_state.nArgB);

                            return 1;
                        }
                    }
                    else
                    {
                        if (OnClipEnd() != 0)
                        {
                            return 1;
                        }

                        return ConditionalDispatch(0);
                    }
                }
                else
                {
                    UpdateFrame(1);
                }
                break;
        }

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00424760
int __fastcall AnimPlayer::SetPlayDirection(int nDirection)
{
    const int nWas = m_state.nDirection;

    m_state.nDirection = nDirection;

    return nWas;
}

// Clamps to the end frame, or forward to a pending trigger frame; other directions answer no.
// FUNCTION: HYPERX 0x00424770
unsigned char AnimPlayer::CheckLoopPoint()
{
    int           nEnd;
    int           nCurrent;
    unsigned char bReached;

    switch (m_state.nDirection)
    {
        case A_REVERSE:
            nEnd     = m_state.nEndFrame;
            nCurrent = m_state.nFrame;

            bReached = nCurrent <= nEnd;

            if (bReached != 0)
            {
                m_state.nFrame     = nEnd;
                m_nFrameInt        = nEnd;
                m_state.flPosition = (float)nEnd;
            }

            return bReached;

        case A_FORWARD:
            if (m_state.bChannelPending != 0)
            {
                const int nLoop = m_state.wEventTrigger;
                nCurrent        = m_state.nFrame;

                if (nCurrent >= nLoop)
                {
                    nEnd = m_state.nEndFrame;

                    m_state.nFrame     = nLoop;
                    m_nFrameInt        = nLoop;
                    m_state.flPosition = (float)nLoop;

                    bReached = nLoop == nEnd;

                    return bReached;
                }
            }

            nEnd     = m_state.nEndFrame;
            nCurrent = m_state.nFrame;

            bReached = nCurrent >= nEnd;
            break;

        default:
            return 0;
    }

    if (bReached != 0)
    {
        m_state.nFrame     = nEnd;
        m_nFrameInt        = nEnd;
        m_state.flPosition = (float)nEnd;
    }

    return bReached;
}

// FUNCTION: HYPERX 0x00424810
void AnimPlayer::SetPlaybackRate(float flRate)
{
    if (m_pSet != 0 && m_state.nPlayState != AA_DONE && m_pClip != 0)
    {
        m_state.flPlaybackRate = flRate;

        const float flPerSecond = flRate * m_pClip->m_flStandardFps;

        if (m_state.nDirection == A_FORWARD)
        {
            m_state.flStep = flPerSecond * g_pGameEngine->m_clock.GetDelta();
        }
        else if (m_state.nDirection == A_REVERSE)
        {
            m_state.flStep = -(flPerSecond * g_pGameEngine->m_clock.GetDelta());
        }
    }
}

#define SLOT_FLAG_MOVED 4

// MSG_NOMSG marks an event consumed; a zero argument still poses the skeleton.
// FUNCTION: HYPERX 0x00424880
void AnimPlayer::UpdateFrame(char bAdvance)
{
    AnimClip*     pClip;
    MeshBone*     pSlot;
    EngineObject* pTarget;
    short         nBone;
    int           nDirection;
    int           nFrame;

    m_state.flStep = g_pGameEngine->m_clock.flDelta * g_pGameEngine->m_clock.flPrevInvDelta * m_state.flStep;

    if (m_state.bFullDecode != 0)
    {
        for (nBone = 0; nBone < m_pBoneSet->m_nClipCount; nBone++)
        {
            pSlot = &m_pObject->m_pBones[nBone];
            pClip = &m_pBoneSet->m_pClips[nBone];

            if (pClip->GetBoneTransformFull(
                    pSlot->m_local.GetMatrixPtr(),
                    (short)m_state.nFrame) != 0)
            {
                pSlot->SetFlags(SLOT_FLAG_MOVED);
            }
        }

        m_state.bFullDecode = 0;
    }
    else
    {
        for (nBone = 0; nBone < m_pBoneSet->m_nClipCount; nBone++)
        {
            pSlot = &m_pObject->m_pBones[nBone];
            pClip = &m_pBoneSet->m_pClips[nBone];

            if (pClip->GetBoneTransform(pSlot->m_local.GetMatrixPtr(), (short)m_state.nFrame) != 0)
            {
                pSlot->SetFlags(SLOT_FLAG_MOVED);
            }
        }
    }

    // The step's sign picks whether the frame must reach or fall to the trigger.
    if (m_state.bChannelPending != 0 && ((m_state.flStep >= g_flAnimBlendZero && m_state.nFrame >= m_state.wEventTrigger) || (m_state.flStep < g_flAnimBlendZero && m_state.nFrame <= m_state.wEventTrigger)))
    {
        const short nCursor = m_state.wEventCursor;

        const int nSlot    = m_state.anEventTarget[nCursor];
        int       nMessage = m_state.anEventMessage[nCursor];
        const int nParam   = m_state.anEventParam[nCursor];

        if (nMessage == MSG_SYNC_FRAME)
        {
            if (m_bPending != 0)
            {
                AnimNamedRef* const pRef = m_state.pEventRecord->m_pRef;

                if (m_pending.pSet == pRef->m_pDb)
                {
                    ConditionalDispatch(pRef);
                    return;
                }
            }

            nMessage = MSG_NOMSG;
        }

        const short nNext    = nCursor + 1;
        m_state.wEventCursor = nNext;

        if (nNext != m_pSet->GetEntryCount())
        {
            AnimClipEntry* const pRecord = m_pSet->m_apEntries[nNext];

            if (pRecord->m_wSegment == m_state.wClipIndex)
            {
                m_state.pEventRecord    = pRecord;
                m_state.bChannelPending = 1;

                // Stored twice: through the record and from the table slot again.
                m_state.wEventTrigger = pRecord->m_wTriggerFrame;
                m_state.wEventTrigger = m_pSet->m_apEntries[nNext]->m_wTriggerFrame;
            }
            else
            {
                m_state.bChannelPending = 0;
            }
        }
        else
        {
            m_state.bChannelPending = 0;
        }

        if (nMessage != MSG_NOMSG && m_pClip->m_wLoopCount != 0)
        {
            pTarget = g_pGameEngine->m_ppSlots[(short)nSlot];

            pTarget->PostEvent(nMessage, nParam, (int)m_pObject->GetSlot());
        }
    }

    if (bAdvance != 0)
    {
        if (m_state.nPlayState != AA_SINGLESTEP)
        {
            m_state.flPosition = m_state.flStep + m_state.flPosition;
        }
        else
        {
            nDirection = m_state.nDirection;

            m_state.nPlayState = AA_PAUSE;

            if (nDirection == A_FORWARD)
            {
                m_state.flPosition = g_flOneAnimPlayer + m_state.flPosition;
            }
            else
            {
                m_state.flPosition -= g_flOneAnimPlayer;
            }
        }

        nFrame = (int)m_state.flPosition;

        m_state.nFrame = nFrame;
        m_nFrameInt    = nFrame;
    }
}

// The blend's duration is capped at 0.35 and never shorter than the root's own travel.
// FUNCTION: HYPERX 0x00424b40
char AnimPlayer::ComputeScale(float flScale)
{
    Vec3  vPos;
    float flApplied;
    Vec3  vProd;

    // Both refusals jump to the one zero return at the end.
    if (flScale != 0.0f)
    {
        if (!(flScale < 0.0f) || m_pClip->m_flFastFps != 0.0f)
        {
            static TransformObject s_blendTransform;

            float flSum     = g_flAnimBlendZero;
            short nSegments = 0;

            short iBone = m_pBoneSet->m_nClipCount;

            while (iBone-- > 0)
            {
                MeshBone*       pSlot = &m_pObject->m_pBones[iBone];
                AnimClip* const pBone = &m_pBoneSet->m_pClips[iBone];

                if (pBone->m_nFormat != ANIM_BONE_MODE_SKIP)
                {
                    pBone->GetBoneTransformFull(s_blendTransform.m_aMatrixRows, (short)m_state.nFrame);

                    m_aSlots[iBone].Init(&pSlot->m_local, &s_blendTransform);

                    flSum = m_aSlots[iBone].GetTheta() + flSum;
                    nSegments++;
                }
            }

            if (!(flScale < 0.0f))
            {
                flApplied = flScale;
            }
            else
            {
                flApplied = m_pClip->m_flFastFps;
            }

            m_flAccum    = 0.0f;
            m_flDuration = flSum / (float)nSegments * flApplied;

            MeshBone* pRoot = m_pObject->m_pBones;

            if (pRoot->IsAnimationDirty() != 0)
            {
                TransformObject* const pMatrix = &pRoot->m_local;

                pMatrix->SetTranslation(pMatrix->GetElement(0, 3) * pRoot->m_pDesc->m_vScale.x,
                                        pMatrix->GetElement(1, 3) * pRoot->m_pDesc->m_vScale.y,
                                        pMatrix->GetElement(2, 3) * pRoot->m_pDesc->m_vScale.z);

                pRoot->ClearFlags(MESHBONE_ANIMATION_DIRTY);
            }

            vPos.x = pRoot->m_local.m_a[0][3];
            vPos.y = pRoot->m_local.m_a[1][3];
            vPos.z = pRoot->m_local.m_a[2][3];

            // The client reads the position back through EAX (0x00424d82 to 0x00424d9b).
            Vec3* const pPos = &m_vPos;
            *pPos            = vPos;

            const Vec3* const pRow = m_pBoneSet->m_pClips[0].m_pTranslation;

            const Vec3* pScale = &pRoot->m_pDesc->m_vScale;
            vProd.x            = pScale->x * pRow->x;
            vProd.y            = pScale->y * pRow->y;
            vProd.z            = pScale->z * pRow->z;

            // A second name keeps the delta's stores where the client has them; without it 82.41.
            Vec3* const pDelta = &m_vDelta;
            pDelta->x          = vProd.x - pPos->x;
            pDelta->y          = vProd.y - pPos->y;
            pDelta->z          = vProd.z - pPos->z;

            double flMax = fabs(m_vDelta.x);

            if (flMax < fabs(m_vDelta.y))
            {
                flMax = fabs(m_vDelta.y);
            }

            if (fabs(flMax) < fabs(m_vDelta.z))
            {
                flMax = fabs(m_vDelta.z);
            }

            double dNeed;

            if ((dNeed = flMax * flApplied) > m_flDuration)
            {
                m_flDuration = (float)dNeed;
            }

            if (m_flDuration > g_flBlendDurationCap)
            {
                m_flDuration = g_flBlendDurationCap;
            }

            m_flScale = 1.0f / m_flDuration;

            return 1;
        }
    }

    return 0;
}

// Returns zero once the blend has run out; a bone in mode one is skipped.
// FUNCTION: HYPERX 0x00424e60
char AnimPlayer::Advance(float flDelta)
{
    char cRunning;

    float flAt = flDelta + m_flAccum;
    m_flAccum  = flAt;

    if (flAt > m_flDuration)
    {
        flAt     = 1.0f;
        cRunning = 0;
    }
    else
    {
        flAt *= m_flScale;
        cRunning = 1;
    }

    for (short iBone = 0; iBone < m_pBoneSet->m_nClipCount; iBone++)
    {
        if (m_pBoneSet->m_pClips[iBone].m_nFormat != ANIM_BONE_MODE_SKIP)
        {
            SlerpSeg* const pSeg = &m_aSlots[iBone];

            pSeg->Eval(flAt);

            pSeg->m_q0.ToMatrix(&m_pObject->m_pBones[iBone].m_local);
        }
    }

    m_pObject->m_pBones->m_local.SetTranslation(flAt * m_vDelta.x + m_vPos.x,
                                                flAt * m_vDelta.y + m_vPos.y,
                                                flAt * m_vDelta.z + m_vPos.z);

    return cRunning;
}

// FUNCTION: HYPERX 0x00424f70
char __fastcall AnimPlayer::AssignAnimEventSlot(int nChannel, int nTarget, int nMessage, int nParam)
{
    if (m_pSet != 0)
    {
        m_state.anEventTarget[nChannel]  = nTarget;
        m_state.anEventMessage[nChannel] = nMessage;
        m_state.anEventParam[nChannel]   = nParam;
    }

    return 1;
}

// FUNCTION: HYPERX 0x00424fa0
char __fastcall AnimPlayer::SetPlayState(int nState)
{
    if (m_state.nPlayState == AA_DONE)
    {
        const char bOk = StartAnimSetDefaultRate(m_pSet);

        if (bOk == 0)
        {
            return bOk;
        }
    }

    m_state.nPlayState = nState;

    return 1;
}

// Answers yes whether or not there was a set to drop.
// FUNCTION: HYPERX 0x00424fd0
char AnimPlayer::ClearAnimationSet()
{
    if (m_pSet != 0)
    {
        m_state.nPlayState = AA_DONE;
        m_pSet             = 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x00424ff0
void __fastcall AnimPlayer::SetCallbackTarget(int nSlot, int nArgA, int nArgB)
{
    m_state.nSlot = nSlot;
    m_state.nArgA = nArgA;
    m_state.nArgB = nArgB;
}

// Starts the parked set, if there is one, and answers what SelectAnimationSet answered, or nought.
// FUNCTION: HYPERX 0x00425010
char AnimPlayer::ConditionalDispatch(AnimNamedRef* pTransition)
{
    char bResult = 0;

    if (m_bPending != 0)
    {
        m_bPending = 0;

        bResult = SelectAnimationSet(m_pending.pSet, m_pending.flRate, 0, pTransition);
    }

    return bResult;
}

// Both forward to slot five with a deferred flag of minus one, which is off.
// FUNCTION: HYPERX 0x00425040
char AnimPlayer::StartAnimSetAtRate(AnimDB* pAnimSet, float flRate)
{
    return StartClipPlayback(pAnimSet, flRate, -1);
}

// FUNCTION: HYPERX 0x00425060
char AnimPlayer::StartAnimSetDefaultRate(AnimDB* pAnimSet)
{
    return StartClipPlayback(pAnimSet, -1.0f, -1);
}

// With no transition the set starts at its first clip, from that clip's start frame.
inline short AnimPlayer::TransitionClip(const AnimNamedRef* pTransition)
{
    if (pTransition == 0)
    {
        return 0;
    }

    return pTransition->m_nSegment;
}

inline short AnimPlayer::TransitionLoopStart(const AnimNamedRef* pTransition)
{
    if (pTransition == 0)
    {
        return -1;
    }

    return pTransition->m_nFrame;
}

// A deferred request parks in m_pending.pSet; asking for the playing set again does not restart it.
// FUNCTION: HYPERX 0x00425080
char AnimPlayer::SelectAnimationSet(AnimDB* pAnimSet, float flRate, int nDeferred, AnimNamedRef* pTransition)
{
    if (nDeferred == 1)
    {
        m_bPending       = 1;
        m_pending.pSet   = pAnimSet;
        m_pending.flRate = flRate;

        if (m_state.nPlayState != AA_PLAY)
        {
            return ConditionalDispatch(0);
        }
        else
        {
            return 1;
        }
    }
    else
    {
        m_bPending = 0;

        if (pAnimSet != 0)
        {
            if (m_state.nPlayState == AA_PLAY && pAnimSet != m_pSet)
            {
                m_pObject->GetPhysicsBody()->ApplyAnimDrivenForce();
            }

            if (pAnimSet != m_pSet || m_state.nPlayState == AA_DONE)
            {
                m_pPrevSet             = m_pSet;
                m_pSet                 = pAnimSet;
                m_pBoneSet             = 0;
                m_pClip                = 0;
                m_state.wClipIndex     = -1;
                m_state.flPlaybackRate = g_flOneAnimPlayer;
                m_state.wEventCursor   = 0;

                const short nChannelCount = pAnimSet->GetEntryCount();
                for (short iChannel = 0; iChannel < nChannelCount; iChannel++)
                {
                    m_state.anEventTarget[iChannel]  = m_pObject->GetSlot();
                    m_state.anEventMessage[iChannel] = m_pSet->GetEntry(iChannel)->m_nMessage;
                    m_state.anEventParam[iChannel]   = 0;
                }

                SetClip(TransitionClip(pTransition), flRate, TransitionLoopStart(pTransition));
            }

            m_state.nPlayState = AA_PLAY;
            SetCallbackTarget(m_pObject->GetSlot(), MSG_CELL_ANIMATION_DONE, 0);
        }

        return pAnimSet != 0;
    }
}

// A duration of zero divides by zero.

// FUNCTION: HYPERX 0x004251d0
void __fastcall AnimPlayer::SetClipDuration(short nIndex, float flDuration)
{
    m_state.aflClipDuration[nIndex] = flDuration;

    if (nIndex == m_state.wClipIndex)
    {
        AnimNode* const pRange = m_pClip;

        const int nFrames = pRange->m_wEndFrame - pRange->m_wStartFrame;

        m_state.flStep = (float)nFrames * g_pGameEngine->m_clock.flDelta / flDuration;
    }
}

#define CLIP_DURATION_TIMED (-2)

// A clip whose last frame precedes its first plays backwards with a negated step.
// FUNCTION: HYPERX 0x00425220
char __fastcall AnimPlayer::SetClip(short nClipIndex, float flBlend, short nLoopStart)
{
    int nDirection;

    char cResult = 0;

    AnimDB* pSet = m_pSet;

    m_state.wClipIndex = nClipIndex;

    if (nClipIndex < pSet->GetNodeCount())
    {
        m_pClip = pSet->m_apNodes[nClipIndex];

        if (m_pClip != 0)
        {
            m_state.bFullDecode = 1;
            m_pBoneSet          = m_pClip->m_pSet;

            if (m_pClip->m_nLoopCount != CLIP_DURATION_TIMED)
            {
                m_state.flStep = g_pGameEngine->m_clock.flDelta * m_pClip->m_flStandardFps * m_state.flPlaybackRate;
            }
            else
            {
                m_state.flStep =
                    (float)(m_pClip->m_wEndFrame - m_pClip->m_wStartFrame) * g_pGameEngine->m_clock.flDelta / m_pClip->m_flSlowFps;

                m_state.aflClipDuration[nClipIndex] = m_pClip->m_flSlowFps;
            }

            if (m_pClip->m_wEndFrame - m_pClip->m_wStartFrame < 0)
            {
                nDirection = A_REVERSE;
            }
            else
            {
                nDirection = A_FORWARD;
            }

            m_state.nDirection = nDirection;

            if (nDirection == A_REVERSE)
            {
                m_state.flStep = -m_state.flStep;
            }

            if (nLoopStart < 0)
            {
                nLoopStart = m_pClip->m_wStartFrame;
            }

            const int nStart = nLoopStart;

            m_state.nLoopFrame = nStart;
            m_state.nEndFrame  = m_pClip->m_wEndFrame;

            const int nMode = m_pClip->m_nLoopCount;

            if (nMode > 0)
            {
                m_state.wLoops = (short)nMode;
            }

            pSet = m_pSet;

            m_state.flPosition = (float)nStart;
            m_state.nFrame     = nStart;
            m_nFrameInt        = nStart;

            m_saved = m_state;

            m_pPrevBoneSet          = m_pBoneSet;
            m_pPrevClip             = m_pClip;
            m_state.bChannelPending = 0;

            const short nChannels = pSet->GetEntryCount();

            if (nChannels != 0)
            {
                for (int iChannel = 0; iChannel < nChannels; iChannel++)
                {
                    AnimClipEntry* const pRecord = pSet->m_apEntries[(short)iChannel];

                    if (pRecord->m_wSegment == m_state.wClipIndex)
                    {
                        m_state.wEventCursor    = (short)iChannel;
                        m_state.pEventRecord    = pRecord;
                        m_state.bChannelPending = 1;
                        m_state.wEventTrigger   = pRecord->m_wTriggerFrame;

                        break;
                    }
                }
            }

            if (ComputeScale(flBlend) != 0)
            {
                m_state.nHold = -1;

                return 1;
            }

            m_state.nHold = m_pClip->m_nLoopCount;
            cResult       = 1;
        }
    }

    return cResult;
}

// FUNCTION: HYPERX 0x004253e0
char AnimPlayer::OnClipEnd()
{
    char cResult = 0;

    m_pObject->GetPhysicsBody()->ApplyAnimDrivenForce();

    m_state.wClipIndex++;

    if (SetClip(m_state.wClipIndex, BLEND_NEXT, CLIP_NEXT) != 0)
    {
        cResult = 1;
    }
    else
    {
        m_state.nPlayState = AA_DONE;

        g_pGameEngine->m_ppSlots[(short)m_state.nSlot]->PostSceneEvent(m_state.nArgA, m_state.nArgB);
    }

    return cResult;
}

// FUNCTION: HYPERX 0x00425540
int AnimPlayer::GetPlayState()
{
    return m_state.nPlayState;
}

// FUNCTION: HYPERX 0x00425550
float AnimPlayer::GetPlaybackRate()
{
    return m_state.flPlaybackRate;
}

// The blend slots' element destructor is Object::Trace in the original, folded by its linker.
// FUNCTION: HYPERX 0x00425580
AnimPlayer::~AnimPlayer()
{
}

// 0.35, the cap on a blend's duration.
float g_flBlendDurationCap = 0.35f;
