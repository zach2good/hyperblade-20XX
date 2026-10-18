// The cheat handlers and their acknowledgement sound. RESTORE undoes the bone scale cheats below.

#include "common/compat.h"
#include "input/keycodes.h"
#include "engine/objectclasses.h"
#include "math/transformobject.h"
#include "globals.h"
#include "engine/statids.h"
#include "engine/gameengine.h"
#include "engine/devicector.h"
#include "graphics/meshbone.h"
#include "math/mathglobals.h"
#include "cheattable.h"
#include "ui/torchbitmaps.h"
#include "engine/messagequeue.h"
#include "sound/audiomgr.h"
#include "ui/saveobject.h"
#include "engine/engineobject.h"
#include "engine/playerobject.h"
#include "graphics/graphicsglobals.h"
#include "input/inputglobals.h"
#include "cheat.h"
#include "engine/object.h"
#include "sound/soundids.h"
#include "engine/messageids.h"
#include "cheatstate.h"
#include "input.h"
#include "app/gameapp.h"
#include "app/menuids.h"
#include "app/wndproc.h"
#include "graphics/hwrenderer.h"
#include "graphics/renderer.h"
#include "input/keyboarddeviceobj.h"

constexpr auto kCheatMessageMs = 1500;

// FUNCTION: HYPERX 0x004497f0
void Cheat::BroadcastFreakout()
{
    g_pGameEngine->BroadcastEvent(kMsgPlayCD, 0, 0);
}

// FUNCTION: HYPERX 0x00449810
void Cheat::EnableHiddenTeams()
{
    g_pGameEngine->m_pSaveObject->SetAllTeamsAvailable(1);

    // STRING: HYPERX 0x004c2c5c
    g_pMessageQueue->Push("Hidden teams enabled", 0, kCheatMessageMs);
}

// FUNCTION: HYPERX 0x00449840
void Cheat::PlayActivateSound()
{
    g_pAudioMgr->PlaySpecific(kSoundSxstmo1);
}

// Changes nothing in the game; the queued row appears whenever something next drains the queue.
// FUNCTION: HYPERX 0x00449860
void Cheat::GeorgeActivate()
{
    g_pAudioMgr->PlaySpecific(kSoundSasrles2);

    // STRING: HYPERX 0x004c2c74
    g_pMessageQueue->Push("George, it's good to see you", 0, kCheatMessageMs);
}

// FUNCTION: HYPERX 0x00449890
void Cheat::MaxViewedPlayerRatings()
{
    PlayerObject* pObject = g_pGameEngine->GetPlayer(g_pGameEngine->m_nViewedSlot);

    // Only the six player slots carry stats.
    if (pObject != nullptr && pObject->GetSlot() < kPlayerObjectCount)
    {
        pObject->m_stats.m_aStats[RATING_SLOT(kRatingDamage)]      = 100.0f;
        pObject->m_stats.m_aStats[RATING_SLOT(kRatingImmunity)]    = 100.0f;
        pObject->m_stats.m_aStats[RATING_SLOT(kRatingChecking)]    = 100.0f;
        pObject->m_stats.m_aStats[RATING_SLOT(kRatingWeightClass)] = 100.0f;
    }
}

// FUNCTION: HYPERX 0x004498d0
void Cheat::EnableAllAwards()
{
    if (g_pGameEngine != nullptr)
    {
        for (int nSlot = 0; nSlot < 6; nSlot++)
        {
            g_pGameEngine->SetAward(nSlot, 1);
        }
    }
}

// The walk starts at one, so bone zero is never touched.
// FUNCTION: HYPERX 0x00449900
void Cheat::RestoreAllPlayers()
{
    for (int nTeam = 0; nTeam < 4; nTeam++)
    {
        EngineObject* pTeam = g_pGameEngine->m_ppSlots[static_cast<short>(g_teamScaleSlots.anTeamOrder[nTeam])];

        if (pTeam != nullptr)
        {
            for (int nBone = 1; nBone < 15; nBone++)
            {
                MeshBone* pBone = &pTeam->m_pBones[static_cast<short>(nBone)];

                if (pBone != nullptr && pBone->IsBound() != 0)
                {
                    TransformObject* const pTransform = &pBone->m_world;

                    pTransform->m_vScale.x = 1.0f;
                    pTransform->m_vScale.y = 1.0f;
                    pTransform->m_vScale.z = 1.0f;

                    pTransform->ApplyScale();

                    TransformObject* const pTarget = &pBone->m_local;

                    pBone->m_bHasScale = 0;

                    pTarget->SetTranslation(1.0f, 1.0f, 1.0f);
                }
            }
        }
    }
}

// FUNCTION: HYPERX 0x004499a0
void Cheat::ShrinkPlayers()
{
    Object::ScaleBones(g_anShrinkBones, 15, g_teamScaleSlots.flShrinkScaleX, g_teamScaleSlots.flShrinkScaleY, g_teamScaleSlots.flShrinkScaleZ);
}

// FUNCTION: HYPERX 0x00449b50
void Cheat::GrowPlayers()
{
    Object::ScaleBones(g_anGrowBones, 15, g_flGrowScaleX, g_flGrowScaleY, g_flGrowScaleZ);
}

// FUNCTION: HYPERX 0x00449b80
void Cheat::MirrorPlayers()
{
    Object::ScaleBones(g_anMirrorBones, 15, g_flMirrorScaleX, g_flMirrorScaleY, g_flMirrorScaleZ);
}

// FUNCTION: HYPERX 0x00449bb0
void Cheat::StretchPlayers()
{
    Object::ScaleBones(g_anStretchBones, 15, g_flStretchScaleX, g_flStretchScaleY, g_flStretchScaleZ);
}

// FUNCTION: HYPERX 0x00449be0
void Cheat::FlipPlayers()
{
    Object::ScaleBones(g_anFlipBones, 15, g_flFlipScaleX, g_flFlipScaleY, g_flFlipScaleZ);
}

// Bones 4, 7, 9, 12 and 15 are the feet, the head and the hands; the body bones are the rest.
// This and the next two overwrite their globals, so a second use scales the first list differently.
// FUNCTION: HYPERX 0x00449c10
void Cheat::BulkUpPlayers()
{
    Object::ScaleBones(g_anBulkUpBodyBones, 10, g_flBulkUpScaleX, g_flBulkUpScaleY, g_flBulkUpScaleZ);

    g_flBulkUpScaleX = 2.5f;
    g_flBulkUpScaleY = 1.2f;
    g_flBulkUpScaleZ = 2.5f;

    Object::ScaleBones(g_anBulkUpHandBones, 2, g_flBulkUpScaleX, g_flBulkUpScaleY, g_flBulkUpScaleZ);

    g_flBulkUpScaleX = 3.5f;
    g_flBulkUpScaleY = 1.2f;
    g_flBulkUpScaleZ = 3.5f;

    Object::ScaleBones(g_anBulkUpHeadFootBones, 3, g_flBulkUpScaleX, g_flBulkUpScaleY, g_flBulkUpScaleZ);
}

// FUNCTION: HYPERX 0x00449cb0
void Cheat::SquashPlayers()
{
    Object::ScaleBones(g_anSquashBodyBones, 10, g_flSquashScaleX, g_flSquashScaleY, g_flSquashScaleZ);

    g_flSquashScaleX = 3.5f;
    g_flSquashScaleY = 3.5f;
    g_flSquashScaleZ = 3.5f;

    Object::ScaleBones(g_anSquashExtremityBones, 5, g_flSquashScaleX, g_flSquashScaleY, g_flSquashScaleZ);
}

// The hip and the spine flip with the legs; the arms keep their scale.
// FUNCTION: HYPERX 0x00449d10
void Cheat::FlipPlayerLegs()
{
    Object::ScaleBones(g_anFlipLegsBodyBones, 6, g_flFlipLegsScaleX, g_flFlipLegsScaleY, g_flFlipLegsScaleZ);

    g_flFlipLegsScaleX = 4.5f;
    g_flFlipLegsScaleY = 4.5f;
    g_flFlipLegsScaleZ = 4.5f;

    Object::ScaleBones(g_anFlipLegsExtremityBones, 5, g_flFlipLegsScaleX, g_flFlipLegsScaleY, g_flFlipLegsScaleZ);
}

// FUNCTION: HYPERX 0x00449d70
int CheatState::ProcessKeypress(char cKey)
{
    const char* pszCode;

    if (cKey == '\r')
    {
        memset(m_szHistory, 0, sizeof(m_szHistory));

        return 0;
    }

    memmove(m_szHistory, m_szHistory + 1, sizeof(m_szHistory) - 1);
    m_szHistory[sizeof(m_szHistory) - 1] = cKey;

    for (int i = 0; i < 15; i++)
    {
        const CheatEntry* const pEntry = &g_aCheatCodes[i];

        pszCode         = pEntry->pszCode;
        int       nSlot = sizeof(m_szHistory) - 1;
        int       nCode = *pszCode++;
        const int nXor  = *pszCode++;

        while (nCode >= 0 && nSlot >= 0 && (m_szHistory[nSlot] ^ nXor) == pszCode[nCode])
        {
            nCode--;
            nSlot--;
        }

        if (nCode == -1)
        {
            pEntry->pfnActivate();

            memset(m_szHistory, 0, sizeof(m_szHistory));

            return 1;
        }
    }

    return 0;
}

// Message table entries the match keys show, and how long they stay up.
constexpr auto kMsgFilterOn         = 0x54;
constexpr auto kMsgFilterOff        = 0x55;
constexpr auto kMsgDromeTexturedOn  = 0x56;
constexpr auto kMsgDromeTexturedOff = 0x57;
#define MSG_TOGGLE_MS 0x5dc

// FUNCTION: HYPERX 0x00449e20
int GameApp::HandleMatchKeypress(unsigned int nKey)
{
    GameEngine*   pEngine;
    int           nSlot;
    EngineObject* pEntity;
    TeamBlock*    pTeamDef;
    unsigned int* pBindings;
    unsigned int  nToggled;

    if (g_gameApp.m_nEngineState == kEstateMatchLive)
    {
        if (g_pGameEngine->m_wRunState == kEngineRunPlaying)
        {
            if (g_cheatState.ProcessKeypress(static_cast<char>(nKey)))
            {
                return 1;
            }

            switch (nKey)
            {
                case Key::kSubtract:
                case Key::kOEMMinus:
                    if (Input::IsShiftKeyDown())
                    {
                        g_pAudioMgr->SfxFadeVolume(g_pAudioMgr->SfxReadVolume() - 1);
                        return 1;
                    }

                    if (Input::IsCtrlKeyDown())
                    {
                        g_pAudioMgr->CDFadeVolume(g_pAudioMgr->CDReadVolume() - 1);
                        return 1;
                    }

                    pEngine = g_pGameEngine;
                    pEngine->SetScreenSize(pEngine->m_nScreenSize - 1);
                    return 1;

                case Key::kAdd:
                case Key::kOEMPlus:
                    if (Input::IsShiftKeyDown())
                    {
                        g_pAudioMgr->SfxFadeVolume(g_pAudioMgr->SfxReadVolume() + 1);
                        return 1;
                    }

                    if (Input::IsCtrlKeyDown())
                    {
                        g_pAudioMgr->CDFadeVolume(g_pAudioMgr->CDReadVolume() + 1);
                        return 1;
                    }

                    pEngine = g_pGameEngine;
                    pEngine->SetScreenSize(pEngine->m_nScreenSize + 1);
                    return 1;

                case Key::kF8:
                    // The hardware renderer textures the drome without the texture DLL.
                    if (g_cMdmTexLoaded == 0 && !HwRenderer::Capturing())
                    {
                        return 1;
                    }

                    if (g_bShadedDromeModels != 0)
                    {
                        return 1;
                    }

                    nToggled = Renderer::ToggleDromeTextured();
                    g_pMessageQueue->ProcessAndClear();

                    if (nToggled != 0)
                    {
                        g_pMessageQueue->PushFromTable(kMsgDromeTexturedOn, 0, MSG_TOGGLE_MS);
                    }
                    else
                    {
                        g_pMessageQueue->PushFromTable(kMsgDromeTexturedOff, 0, MSG_TOGGLE_MS);
                    }

                    return 1;

                case Key::kF7:
                    if (g_cMdmTexLoaded != 0)
                    {
                        nToggled = WndProc::HandleRenderMenuCommand(kIDRenderBilinearFilter);
                        g_pMessageQueue->ProcessAndClear();

                        if (nToggled != 0)
                        {
                            g_pMessageQueue->PushFromTable(kMsgFilterOn, 0, MSG_TOGGLE_MS);
                        }
                        else
                        {
                            g_pMessageQueue->PushFromTable(kMsgFilterOff, 0, MSG_TOGGLE_MS);
                        }
                    }

                    return 1;

                case '1':
                case '2':
                    pEngine = g_pGameEngine;
                    nSlot   = pEngine->m_nViewedSlot;
                    pEntity = pEngine->m_ppSlots[static_cast<short>(nSlot)];

                    if (pEntity != nullptr)
                    {
                        if ((nSlot & 1) != 0)
                        {
                            pTeamDef = pEngine->m_pTeamB;
                        }
                        else
                        {
                            pTeamDef = pEngine->m_pTeamA;
                        }

                        ReserveRows* const pReserves = pEngine->GetReserveRows(pTeamDef);
                        pEntity->PostSceneEvent(kMsgSubstituteDromer, pReserves->anRow[nKey - '1']);
                    }

                    return 1;

                default:
                    pBindings = g_pInputDevice->GetDefaultKeyBindings();

                    if (nKey == pBindings[kBindingJump])
                    {
                        pEngine = g_pGameEngine;
                        nSlot   = pEngine->m_nViewedSlot;
                        pEngine->m_ppSlots[nSlot]->PostSceneEvent(kMsgJump, 0);
                        return 1;
                    }

                    if (nKey != pBindings[kBindingThrow])
                    {
                        return 0;
                    }

                    pEngine = g_pGameEngine;
                    nSlot   = pEngine->m_nViewedSlot;
                    pEngine->m_ppSlots[nSlot]->PostSceneEvent(kMsgThrowWindUp, 0);
                    return 1;
            }
        }
    }

    return 0;
}

// Each body is XORed with the key in its second byte; the first byte is the last index.
static const char g_szCheatFreakout[] = { 7, 0x1d, 0x5b, 0x4f, 0x58, 0x5c, 0x56, 0x52, 0x48, 0x49, 0 };
static const char g_szCheatShiun[]    = { 4, 0x1e, 0x4d, 0x56, 0x57, 0x4b, 0x50, 0 };
static const char g_szCheatSmorris[]  = { 6, 0x1c, 0x4f, 0x51, 0x53, 0x4e, 0x4e, 0x55, 0x4f, 0 };
static const char g_szCheatMdmksb[]   = { 5, 0x1f, 0x52, 0x5b, 0x52, 0x54, 0x4c, 0x5d, 0 };

static const char g_szCheatCoolio[] = { 13, 0x17, 0x44, 0x42, 0x47, 0x52, 0x45, 0x37, 0x54, 0x58, 0x58, 0x5b, 0x5e, 0x58, 0x2f, 0x23, 0 };

static const char g_szCheatGeorge[]     = { 10, 0x15, 0x52, 0x50, 0x5a, 0x47, 0x52, 0x50, 0x46, 0x40, 0x41, 0x41, 0x4c, 0 };
static const char g_szCheatRestore[]    = { 6, 0x1c, 0x4e, 0x59, 0x4f, 0x48, 0x53, 0x4e, 0x59, 0 };
static const char g_szCheatGorilla[]    = { 6, 0x1c, 0x5b, 0x53, 0x4e, 0x55, 0x50, 0x50, 0x5d, 0 };
static const char g_szCheatSkuzz[]      = { 10, 0x10, 0x43, 0x5b, 0x45, 0x4a, 0x4a, 0x52, 0x5f, 0x40, 0x40, 0x55, 0x42, 0 };
static const char g_szCheatDragon[]     = { 5, 0x1f, 0x5b, 0x4d, 0x5e, 0x58, 0x50, 0x51, 0 };
static const char g_szCheatGark[]       = { 3, 0x19, 0x5e, 0x58, 0x4b, 0x52, 0 };
static const char g_szCheatSpicy[]      = { 10, 0x10, 0x43, 0x40, 0x59, 0x53, 0x49, 0x52, 0x42, 0x51, 0x59, 0x5e, 0x43, 0 };
static const char g_szCheatWatusi[]     = { 5, 0x1f, 0x48, 0x5e, 0x4b, 0x4a, 0x4c, 0x56, 0 };
static const char g_szCheatHeartlight[] = { 9, 0x13, 0x5b, 0x56, 0x52, 0x41, 0x47, 0x5f, 0x5a, 0x54, 0x5b, 0x47, 0 };
static const char g_szCheatPotatoe[]    = { 6, 0x1c, 0x4c, 0x53, 0x48, 0x5d, 0x48, 0x53, 0x59, 0 };

// The shortened names decode to SUPER COOLIO84, GEORGESUTTY, SKUZZBOPPER and SPICYBRAINS.
CheatEntry g_aCheatCodes[15] = {
    { g_szCheatFreakout, Cheat::BroadcastFreakout },
    { g_szCheatShiun, Cheat::EnableHiddenTeams },
    { g_szCheatSmorris, Cheat::PlayActivateSound },
    { g_szCheatMdmksb, Cheat::MaxViewedPlayerRatings },
    { g_szCheatCoolio, Cheat::EnableAllAwards },
    { g_szCheatGeorge, Cheat::GeorgeActivate },
    { g_szCheatRestore, Cheat::RestoreAllPlayers },
    { g_szCheatGorilla, Cheat::BulkUpPlayers },
    { g_szCheatSkuzz, Cheat::SquashPlayers },
    { g_szCheatDragon, Cheat::FlipPlayerLegs },
    { g_szCheatGark, Cheat::GrowPlayers },
    { g_szCheatSpicy, Cheat::MirrorPlayers },
    { g_szCheatWatusi, Cheat::FlipPlayers },
    { g_szCheatHeartlight, Cheat::StretchPlayers },
    { g_szCheatPotatoe, Cheat::ShrinkPlayers }
};

TeamScaleSlots g_teamScaleSlots              = { { 0, 2, 1, 3 }, { 0, 2, 1, 3 }, 0.9f, 0.9f, 0.9f };
int            g_anShrinkBones[15]           = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
float          g_flGrowScaleX                = 1.1f;
float          g_flGrowScaleY                = 1.1f;
float          g_flGrowScaleZ                = 1.1f;
int            g_anGrowBones[15]             = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
float          g_flMirrorScaleX              = -1.1f;
float          g_flMirrorScaleY              = -1.1f;
float          g_flMirrorScaleZ              = -1.1f;
int            g_anMirrorBones[15]           = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
float          g_flStretchScaleX             = 1.1f;
float          g_flStretchScaleY             = 2.0f;
float          g_flStretchScaleZ             = 1.0f;
int            g_anStretchBones[15]          = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
float          g_flFlipScaleX                = 1.0f;
float          g_flFlipScaleY                = -1.5f;
float          g_flFlipScaleZ                = 1.0f;
int            g_anFlipBones[15]             = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
float          g_flBulkUpScaleX              = 2.0f;
float          g_flBulkUpScaleY              = 0.9f;
float          g_flBulkUpScaleZ              = 2.0f;
int            g_anBulkUpBodyBones[10]       = { 1, 2, 3, 5, 6, 8, 10, 11, 13, 14 };
int            g_anBulkUpHandBones[2]        = { 12, 15 };
int            g_anBulkUpHeadFootBones[3]    = { 4, 7, 9 };
float          g_flSquashScaleX              = 1.5f;
float          g_flSquashScaleY              = 0.6f;
float          g_flSquashScaleZ              = 1.5f;
int            g_anSquashBodyBones[10]       = { 1, 2, 3, 5, 6, 8, 10, 11, 13, 14 };
int            g_anSquashExtremityBones[5]   = { 4, 7, 9, 12, 15 };
float          g_flFlipLegsScaleX            = 3.5f;
float          g_flFlipLegsScaleY            = -4.0f;
float          g_flFlipLegsScaleZ            = 3.5f;
int            g_anFlipLegsBodyBones[6]      = { 1, 2, 3, 5, 6, 8 };
int            g_anFlipLegsExtremityBones[5] = { 4, 7, 9, 12, 15 };
