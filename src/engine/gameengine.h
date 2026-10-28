// The game engine object; the size is a floor.

#ifndef HYPERBLADE_GAMEENGINE_H
#define HYPERBLADE_GAMEENGINE_H

#include "common/compat.h"
#include "common/clock.h"
#include "engine/engineobject.h"
#include "engine/coachobject.h"
#include "engine/collisionobjectids.h"
#include "common/commonglobals.h"
#include <array>
#include <memory>

// The length of both slot tables, m_ppSlots and m_ppObjects.
constexpr auto kEngineSlotCount = 0x66;

// The length of each of the scene message queue's five arrays.
constexpr auto kPendingQueueLength = 0x400;

struct ArenaState;
class EngineObject;
class PlayerObject;
class CameraObject;
class RokObject;
class KeyboardDeviceObj;
class FileObj;
class ObjectDef;
class ParticleSystem;
class SaveObject;
struct TeamBlock;
struct TeamsBlock;
enum class DifficultyLevel;

#include "teamblock.h"
#include "devicector.h"
#include "engine/engineclock.h"
#include "engineglobals.h"
#include "graphics/graphics.h"

class AgendaItem;
class CollInteractor;
class BoundBubble;
class GameBehaviorObject;

class SaveObject;

// What the layout builder returns: 64 bytes shared by a widget's layout and a camera's viewport.
struct LayoutState
{
    void* pBits; // 0x10
    // Nought in a widget's, the camera's own transform in the camera's.
    class CameraProjectionState* pCamera;      // 0x14
    int                          nPrevWidth;   // 0x18
    int                          nPrevHeight;  // 0x1c
    int                          nWidth;       // 0x20
    int                          nHeight;      // 0x24
    int                          nPitch;       // 0x28
    int                          nByteSize;    // 0x2c
    float                        flPrevRight;  // 0x30
    float                        flPrevBottom; // 0x34
    int                          nHalfWidth;   // 0x38
    int                          nHalfHeight;  // 0x3c
};

// The six end-of-match awards.
constexpr auto kAwardCount = 6;

// GameEngine::m_wRunState: no match, playing, paused, or a restart the next frame runs.
constexpr auto kEngineRunStopped = 0;
constexpr auto kEngineRunPlaying = 1;
constexpr auto kEngineRunPaused  = 2;
constexpr auto kEngineRunRestart = 3;

// The cameras and input devices the factory builds at most.
constexpr auto kCameraLimit = 4;
constexpr auto kDeviceLimit = 2;

struct GameEngine
{
    // Reached through g_pGameEngine rather than this.
    static int CountEntitiesInRange(short nWhich);
    static int CoachConditionNever();

    // The linker folded this empty body onto KeyConfigDialog::SaveKeysAs, which carries the marker.
    static int CoachConditionUnused()
    {
        return 0;
    }

    static int            IsAnyPlayerInjured();
    static void           ShowRokValue(short nRokValue);
    static int            CheckOpposedCarriersRace();
    static int            CheckCarrierBreakaway();
    static int            CheckGoalieOutOfPosition();
    static void           DestroyAgendaItemPools();
    static void           CreateAgendaItemPools();
    static int            IsComputerTeam(int nSlot);
    static int            CheckWeaponOutWhileRokCarried();
    static void           SubstituteRecoveredReserve(unsigned int nTeam);
    static int            FindNearestLiveMultiplier(EngineObject* pFrom);
    static int            CheckHandicapCondition();
    static void           InitAgendaItemPools();
    static int            CheckAnyPlayerNeedsSub();
    static unsigned short TogglePause(int bRestyle);
    static int            SelectSubstitute(EngineObject* pActor);
    static int            IsTeamWinning(unsigned char nTeam);

    void Reset();

    void ResetPeriodToFirst();
    void PerFrameUpdate();

    void Init(const char* pszConfig);
    void StartMatch();

    char ReadSlotLabel(int nSlot, char* pszLabel, int nMax);
    char PostSceneMessage(int nSlot, int nMessage, int nArg, int nSender, int bFromInput);

    class ObjectDef*                            GetArenaRokDef();
    class ObjectDef*                            GetTeamRokDef(int nSlot);
    [[nodiscard]] std::unique_ptr<EngineObject> UnregisterObject(int nSlot);

    void RegisterObject(int nSlot, std::unique_ptr<EngineObject> pObject);
    char SyncRenderQueueSize();
    void ReplaceEntity(int nKind, class ObjectDef* pDef);

    void SetAttackMode(int nOn);
    void SetTeammateMode(int nOn);

    void SetHomeScore(int nScoreHome);
    void SetAwayScore(int nScoreAway);

    char HasAward(int nSlot) const;
    void SetAward(int nSlot, char cValue);
    GameEngine();

    void ClearAwards();
    char EvalArcherAward();
    char EvalAnvilAward();

    std::unique_ptr<TeamBlock> ParseTeamHTMFile(class FileObj* pFile);
    static int                 FindTeamDefinition(class FileObj* pFile, char** ppLine, char* pszToken);

    ReserveRows* GetReserveRows(const TeamBlock* pTeam);

    float GetGravityDt() const
    {
        return m_flGravityDt;
    }

    short GetViewedSlot() const
    {
        return m_nViewedSlot;
    }

    EngineObject* GetSlot(int nSlot) const
    {
        return m_ppSlots[nSlot];
    }

    int   m_nLoadCost;    // 0x0000
    short m_nLoadedCount; // 0x0004
    short m_nObjectCount; // 0x0006, bounds the walk over m_ppObjects
    short m_nSlotCount;   // 0x0008
    // The slot count the render queues were last sized for.
    short m_nQueuedSlotCount; // 0x000a

    // Indexed by model id: 0 to 5 the dromers, 6 the Rok, 7 and 8 the goals, 23 God, 0x36 Mine.
    std::unique_ptr<EngineObject*[]> m_ppSlots; // 0x000c

    // Walked from 0 to m_nObjectCount.
    std::unique_ptr<EngineObject*[]> m_ppObjects; // 0x0010

    // The objects m_ppSlots names, owned slot for slot.
    std::array<std::unique_ptr<EngineObject>, kEngineSlotCount> m_apSlotOwners;

    // The typed views of m_ppSlots; each is defined beside its class.
    PlayerObject*      GetPlayer(int nSlot) const;
    CameraObject*      GetCamera(int nSlot) const;
    RokObject*         GetRok() const;
    KeyboardDeviceObj* GetInputDevice(int nSlot) const;
    ParticleSystem*    GetParticleSystem(int nSlot) const;
    void               SetColObject(short nSlot, class BoundBubble* pBubble);
    class BoundBubble* GetColObjPtr(short nSlot) const;

    // The cameras the factory builds, counted by m_wCameraCount.
    CameraObject* m_apCameras[kCameraLimit]; // 0x0014
    // The devices the factory builds, counted by m_wDeviceCount.
    EngineObject* m_apDevices[kDeviceLimit]; // 0x0024
    short         m_wDeviceCount;            // 0x002c
    short         m_wCameraCount;            // 0x002e
    // One per type range, holding the highest id handed out in it.
    unsigned char m_abTypeHighWater[kGOBNumOfTypes]; // 0x0030

    // The slot being watched, followed or viewed.
    short m_nViewedSlot; // 0x0050
    short m_wUnusedA;    // 0x0052
    // One bubble per CollisionObjectID before id_COB_NOCOB.
    class BoundBubble* m_ppColObjects[kCOBNone]; // 0x0054
    // Read as one dword and as a word pair.
    unsigned short m_wRunState;     // 0x0228
    short          m_nPendingCount; // 0x022a
    // Five parallel queues of kPendingQueueLength words; kMsgNomsg terminates the first.
    int m_aQueueMessage[kPendingQueueLength];   // 0x022c
    int m_aQueueSlot[kPendingQueueLength];      // 0x122c
    int m_aQueueArg[kPendingQueueLength];       // 0x222c
    int m_aQueueSender[kPendingQueueLength];    // 0x322c
    int m_aQueueFromInput[kPendingQueueLength]; // 0x422c

    // The clock block; every m_flPrev* is written from the field above it.
    EngineClock m_clock;       // 0x522c
    float       m_flGravityDt; // 0x5278
    // Written by the constructor and read by nothing.
    float m_flPrevGravityDt; // 0x527c
    short m_wUnusedB;        // 0x5280
    short m_wUnusedC;        // 0x5282
    short m_wUnusedD;        // 0x5284
    // The two tracked objects.
    EngineObject* m_apTracked[2]; // 0x5288
    // The home team's definition, and the score block the end of a match clears.
    std::unique_ptr<TeamBlock> m_pTeamA; // 0x5290
    std::unique_ptr<TeamBlock> m_pTeamB; // 0x5294
    // src/engine/arenastate.h holds the record.
    std::unique_ptr<ArenaState> m_pArena;         // 0x5298
    unsigned short              m_nTeamHomeIndex; // 0x529c
    unsigned short              m_nTeamAwayIndex; // 0x529e
    unsigned short              m_nArenaIndex;    // 0x52a0
    int                         m_nPending;       // 0x52a4
    // Minus one means no score yet.
    int             m_nScoreHome;  // 0x52a8
    int             m_nScoreAway;  // 0x52ac
    DifficultyLevel m_nDifficulty; // 0x52b0
    int             m_nBrutality;  // 0x52b4
    // The screen size step, 0 to 6.
    int                 m_nScreenSize;  // 0x52b8
    Clock::Milliseconds m_periodLength; // 0x52bc
    int                 m_nAttackMode;  // 0x52c0
    // The teammate mode, read from the options under g_pszTeammateModeKey.
    int m_nTeammateMode; // 0x52c4
    // One flag per award, which the awards screen shows.
    char m_abAward[kAwardCount]; // 0x52c8
    char m_bFollowingOther;      // 0x52ce
    char m_bEndPeriodRequested;  // 0x52cf
    // Tested before ending a period and set when one ends.
    char                        m_bPeriodEnded; // 0x52d0
    std::unique_ptr<TeamsBlock> m_pTeamTable;   // 0x52d4
    std::unique_ptr<SaveObject> m_pSaveObject;  // 0x52d8
    int                         m_bMatchOver;   // 0x52dc

    // Match time leaves the pauses out, so plan timers, input and the period clock stop with play.
    Clock::Milliseconds m_paused{};
    Clock::Milliseconds m_pausedSince{};

    // The match clock, or the wall clock while there is no engine.
    static Clock::Milliseconds MatchNow();

    void DispatchCollisionToRule(class CollInteractor* pChain);
    void SetRunState(short bRunning, int nReason);
    void BroadcastEvent(int nEvent, int nParam, int nSender);

    // Answers null at the end of the file; the model id is written back through the second argument.
    std::unique_ptr<ObjectDef> ParseObjectDefinition(class FileObj* pFile, unsigned int* pnModelId);
    EngineObject*              CreateObject(class ObjectDef* pDef, unsigned int nModelId);
    char                       LoadConfigFile(const char* pszFileName);
    char                       LoadGameConfig(const char* pszFileName);
    char                       LoadObjectsFromConfig(const char* pszFileName);

    // The five commands a game configuration file is made of.
    std::unique_ptr<ObjectDef> Cmd_DefineObject(char* pszLine, unsigned int* pnModelId);
    char                       Cmd_SetMotionParams(char* pszLine, class ObjectDef* pDef);
    int                        Cmd_SetRatings(char* pszLine, class ObjectDef* pDef, int nMode, int nLevel);
    char                       Cmd_SetCollisionObject(char* pszLine, class ObjectDef* pDef, unsigned int nModelId);
    void                       MapModifierToId(short nModelId, class BubbleCmd* pCmd);
    char                       Cmd_SetPlans(char* pszLine, class ObjectDef* pDef);
    // Reads only g_nPlayerMode.
    unsigned char IsExhibitionMode();

    void TickClock();

    char                       LoadHomeTeam(const char* pszTeamPath);
    char                       LoadAwayTeam(const char* pszTeamPath);
    std::unique_ptr<TeamBlock> LoadTeamFromFile(const char* pszTeamPath, int nTeam);

    char LoadArena(const char* pszArenaPath);

    char SetTeams(short nHome, short nAway);

    // Always answers one, and its caller discards it.
    char SetArena(short nArena);

    unsigned char IsTournamentMode();

    // Every caller drops the answer.
    char          SubstitutePlayer(int nSlot, int nWith);
    static int    SubstituteSlot(int nTeam, int nSeat);
    unsigned char IsFitSubstitute(unsigned int nSlot, int nWhich);

    void SetScreenSize(int nStep);
    void ProcessInputCommandQueue();
    void SyncPlayerDataToTeam();
    void AdvanceTournamentBracket();

    // Answers whether the slot was written; only tournament player mode writes one.
    char WriteSaveFile(int nSlot, const char* pszLabel);
    char ReadSaveFile(int nSlot);

    char SetDifficultyLevel(unsigned int nLevel);
    void UnloadArena();
    ~GameEngine();

    GameEngine(const GameEngine&)            = delete;
    GameEngine(GameEngine&&)                 = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine& operator=(GameEngine&&)      = delete;

    char RemoveFromSortedList(short nId);

    // Each writes the setting back to the ini file as well, and only when it changed.
    void SetBrutality(int nValue);
    void SetPeriodLengthMs(int nValue);

    int GetEntityCountOfType(int nType);
    // Each is m_abTypeHighWater[N] less the baseline for a fixed N.
    int  GetArmoryCount() const;
    int  GetSwarfCount() const;
    int  GetMineCount() const;
    void DeactivateParticleSystems();

    // Answers the first particle system with nothing alive, or nothing.
    class ParticleSystem* FindFreeParticleSystem();

    void UnloadEntitySet(struct TeamBlock* pSet);
    char UnloadHomeTeam();
    char UnloadAwayTeam();
    char InsertObjectSorted(short nId);

    int             GetHomeScore() const;
    int             GetAwayScore() const;
    DifficultyLevel GetDifficultyLevel() const;
    char            AreAllPlayersInjured();
    void            BeginMatchPlay();
    unsigned char   IsHomeTeamDead();
    unsigned char   IsAwayTeamDead();
    void            RestartMatch();
    void            BroadcastCollisionEvents();
    void            SyncNetworkStateToObjects();
    void            ClearPendingEvents();
    void            InitPlanSystems();
    void            InitMatchTimers();
    int             IncrementHomeScore();
    int             IncrementAwayScore();
    unsigned char   IsHomeTeamInjured();
    unsigned char   IsAwayTeamInjured();
};

// VC6 has no static_assert; a negative array bound makes a wrong offset fail to compile.

#endif
