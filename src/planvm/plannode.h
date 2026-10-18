#ifndef HYPERBLADE_PLANNODE_H
#define HYPERBLADE_PLANNODE_H

#include "common/compat.h"
#include "graphics/bitmapformat.h"

// An argument whose high half is this is a pronoun: the low half indexes g_apPlanStatFns.
constexpr auto kPlanArgPronoun = 0xfedc;

// The high halves the compiler gives a quoted local real and an interactor parameter.
constexpr auto kPlanArgLocalReal      = 0xedcb;
constexpr auto kPlanArgInteractorName = 0xdcba;

// The values of the authored tokens the plan compiler seeds, each named for its token.
constexpr auto kPlanTokenBroadcast = 0x0000ffff;
#define PLAN_TOKEN_STRAIGHTAHEAD ((int)0xabcdabcd)
#define PLAN_TOKEN_TEMPORARILY   ((int)0xabcdabcc)
#define PLAN_TOKEN_MESSAGE_DATA  ((int)0xabcdabce)
#define PLAN_TOKEN_NO_BEH_CHANGE ((int)0xabcdabcb)
#define PLAN_TOKEN_PREVIOUS_BEH  ((int)0xabcdabca)

// The stream words that open a compound node and a string literal.
constexpr auto kPlanMarkerNode   = 0xabcdefff;
constexpr auto kPlanMarkerString = 0xabcdeffe;

// One argument word: most are integers; some carry a float's bits, a colour, a child node or a
// name.
union PlanWord
{
    int              n;
    float            fl;
    PaletteEntry     pe;
    struct PlanNode* pNode;
    char*            psz;
    // The low half under a tag; a field, as the client loads it with a word read.
    unsigned short wPronoun;

    unsigned int GetTag() const
    {
        return static_cast<unsigned short>(static_cast<unsigned int>(n) >> 16);
    }

    bool IsPronoun() const
    {
        return GetTag() == kPlanArgPronoun;
    }

    bool IsLocalReal() const
    {
        return GetTag() == kPlanArgLocalReal;
    }

    bool IsInteractorName() const
    {
        return GetTag() == kPlanArgInteractorName;
    }

    static int Pack(unsigned int nTag, int nIndex)
    {
        return nIndex | (nTag << 16);
    }
};

class Interactor;

struct PlanNode
{
    // (category << 16) | sub.
    int       m_nOpcode;   // 0x00
    short     m_nArgCount; // 0x04
    PlanWord* m_pArgs;     // 0x08

    int GetArgLeaf(int nIndex) const
    {
        return m_pArgs[nIndex].pNode->m_pArgs[0].n;
    }
};

// DATA/ENUMIDS.H's Pronouns: the plan words that name an object relative to the actor.
enum Pronoun
{
    PRN_ME,
    PRN_MY_TEAMMATE,
    PRN_MY_LEAD_OPPONENT,
    PRN_MY_OTHER_OPPONENT,
    PRN_MY_GOAL,
    PRN_MY_GOAL_SIDE,
    PRN_MY_GOAL_ZONE,
    PRN_OPPONENTS_GOAL,
    PRN_OPPN_GOAL_SIDE,
    PRN_OPPN_GOAL_ZONE,
    PRN_MY_COLLISION_BUB,
    PRN_MY_LEAD_OPPN_COLL_BUB,
    PRN_MY_OTHER_OPPN_COLL_BUB,
    PRN_MY_BACK_DEF_WG_POS,
    PRN_MY_GOAL_POS,
    PRN_VIEW_OWNER,
    PRN_VIEW_OWNERS_GOAL,
    PRN_VIEW_OWNERS_TEAMMATE,
    PRN_ROK_CARRIER,
    PRN_ROK_CARRIER_COB,
    PRN_MY_DRAG_START_POS,
    PRN_LAST_ROK_CARRIER,
    PRN_PREV_ROK_CARRIER,
    PRN_MY_AWARENESS_BUB,
    PRN_NON_ROK_CARRYING_OPPN,
    PRN_VIEW_ONRS_OPPN1,
    PRN_VIEW_ONRS_OPPN2,
    PRN_VIEW_OWNER_COB,
    PRN_VIEW_ONRS_OPPN1_COB,
    PRN_VIEW_ONRS_OPPN2_COB,
    PRN_WATCHED_OBJECT,
    PRN_VW_OWNR_OPPN_GOALIE,
    PRN_MY_BODY_BUBBLE,
    PRN_LEAD_OPPN_BODY_BUB,
    PRN_OTHER_OPPN_BODY_BUB,
    PRN_MY_GOALIE,
    PRN_MY_LEAD_TEAMMATE,
    PRN_TRACKED_OBJECT,
    PRN_TRACKED_OBJS_TEAMMATE,
    PRN_VW_OWNR_AWARNS_BUB,
    PRN_VW_OWNRS_OPPN_GOAL,
    PRN_THIS_MULTIPLIER_COB,
    PRN_THIS_TURNSTILE_COB,
    PRN_THIS_LIMBO_POLE_COB,
    PRN_THIS_LASER_HURDLE_COB,
    PRN_THIS_FAST_LANE_COB,
    PRN_THIS_EXPLOSION_COB,
    PRN_THIS_HURDLE_COB,
    PRN_THIS_GREEN_RANDOMIZER_COB,
    PRN_THIS_RED_RANDOMIZER_COB,
    PRN_THIS_RAMP_COB,
    PRN_MY_TEAMMATES_COB,
    PRN_THIS_ARMORY_COB,
    PRN_THIS_SLOW_LANE_COB,
    PRN_THIS_MINE_COB,
    PRN_THIS_SWARF_COB,
    PRN_THIS_BODY_PART_COB,
    PRN_COUNT
};

constexpr auto kOpCategory = 0xffff0000;
constexpr auto kOpMember   = 0xffff;

// An opcode's category, each named for the authored keywords it holds.
enum OpcodeCategory
{
    CATEGORY_COLLISION_INTERACTOR = 0x00030000,
    CATEGORY_MESSAGE_INTERACTOR   = 0x00050000,
    CATEGORY_DEBUG                = 0x00060000,
    CATEGORY_BEHAVIOUR            = 0x00070000,
    CATEGORY_PLAN_CHANGE          = 0x00160000,
    CATEGORY_DECIDE               = 0x00170000,
    CATEGORY_ACTION               = 0x00180000,
    CATEGORY_AGENDA               = 0x001a0000,
    CATEGORY_GLOBAL_INTERACTORS   = 0x001b0000,
    CATEGORY_DATA_BLOCK           = 0x20000000,
    CATEGORY_BLOCK                = 0x40000000,
    CATEGORY_ENABLE               = 0x80000000,
    CATEGORY_END_PLAN             = 0xffff0000
};

// An opcode's member within its category, each named for the authored keyword it compiles from.
// GLOBAL_INTERACTORS, Enable, Block, DataBlock and END_PLAN are their category's whole word.

// SET_NET_COLLISION_INTERACTOR compiles to SET_NET_MESSAGE_INTERACTOR's word.
enum CollisionInteractorMember
{
    MEMBER_SET_COLLISION_INTERACTOR,
    MEMBER_SET_LOC_COLLISION_INTERACTOR
};

enum MessageInteractorMember
{
    MEMBER_SET_MESSAGE_INTERACTOR,
    MEMBER_SET_LOC_MESSAGE_INTERACTOR,
    MEMBER_SET_NET_MESSAGE_INTERACTOR
};

enum DebugMember
{
    MEMBER_SET_DEBUG = 1,
    MEMBER_SET_TRACE
};

// The behaviour category, id_BDC_ in DATA/ENUMIDS.H.
enum BehaviourMember
{
    MEMBER_DECLARE_BEHAVIOR,
    MEMBER_BEHAVIOR_NAME,
    MEMBER_CHANGE_TO_BEHAVIOR,
    MEMBER_SET_BEHAVIOR_PARAM
};

enum PlanChangeMember
{
    MEMBER_CHANGE_OF_PLAN,
    MEMBER_DECLARE_LOCAL_REALS
};

enum DecideMember
{
    MEMBER_DECIDE_BY_AMONG,
    MEMBER_DECIDE_BY_WITH_AMONG
};

enum ActionMember
{
    MEMBER_DO_ACTION,
    MEMBER_SET_INTERACTOR_PARAM,
    MEMBER_DO_LOCAL_ACTION,
    MEMBER_DO_NET_ACTION
};

enum AgendaMember
{
    MEMBER_ADD_AGENDA_ITEM = 1,
    MEMBER_SET_TIMEOUTMSG,
    MEMBER_REMOVE_AGENDA_ITEM
};

// How the plan machine calls an action or a decision.
using PlanHandlerFn = void (*)(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);

class EngineObject;
class GameBehaviorObject;

// How the plan machine calls a behaviour parameter: the actor, the node and the behaviour it sets.
using BehParamHandlerFn = void (*)(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pTarget);

// How SET_INTERACTOR_PARAM calls an id_SET_ interactor parameter: the actor, the node, the behaviour
// and the interactor it sets, which is null in the behaviour's own setup pass.
using InteractorParamFn = void (*)(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour, Interactor* pInteractor);

// The opcode table: SET_INTERACTOR_PARAM reads its first two slots and a decision the rest.
union PlanOpcodeSlot
{
    InteractorParamFn pfnSetParam;
    PlanHandlerFn     pfnDecide;
};

#endif
