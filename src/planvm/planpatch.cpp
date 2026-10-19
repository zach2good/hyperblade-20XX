// Fixes for the shipped plans, as splices on the compiled words. docs/plan_bugs.md describes each one.

#include "planvm/planpatch.h"
#include "engine/messageids.h"
#include "planvm/agendaitemids.h"
#include "planvm/plannode.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdint>

namespace
{

constexpr auto kNone = static_cast<size_t>(-1);

constexpr auto kAddAgenda    = static_cast<int>(CATEGORY_AGENDA) | MEMBER_ADD_AGENDA_ITEM;
constexpr auto kTimeout      = static_cast<int>(CATEGORY_AGENDA) | MEMBER_SET_TIMEOUTMSG;
constexpr auto kRemoveAgenda = static_cast<int>(CATEGORY_AGENDA) | MEMBER_REMOVE_AGENDA_ITEM;
constexpr auto kChange       = static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_CHANGE_TO_BEHAVIOR;
constexpr auto kCollision    = static_cast<int>(CATEGORY_COLLISION_INTERACTOR) | MEMBER_SET_COLLISION_INTERACTOR;
constexpr auto kDecide       = static_cast<int>(CATEGORY_DECIDE) | MEMBER_DECIDE_BY_AMONG;
constexpr auto kAction       = static_cast<int>(CATEGORY_ACTION) | MEMBER_DO_ACTION;
constexpr auto kBlock        = static_cast<int>(CATEGORY_BLOCK);
constexpr auto kEnable       = static_cast<int>(CATEGORY_ENABLE);
constexpr auto kNodeMarker   = static_cast<int>(kPlanMarkerNode);

constexpr int PronounWord(Pronoun prn)
{
    return static_cast<int>((kPlanArgPronoun << 16) | prn);
}

// The word after the node, leaf or string at nAt, or kNone when the words run out first.
size_t End(std::span<const int> words, size_t nAt)
{
    auto nPending = 1;

    while (nPending > 0 && nAt < words.size())
    {
        const auto nWord = static_cast<unsigned int>(words[nAt]);

        if (nWord == kPlanMarkerNode)
        {
            if (nAt + 1 < words.size() && static_cast<unsigned int>(words[nAt + 1]) == CATEGORY_END_PLAN)
            {
                nAt += 2;
                nPending--;

                continue;
            }

            if (nAt + 2 >= words.size())
            {
                return kNone;
            }

            nPending += (words[nAt + 2] & 0x7fff) - 1;
            nAt += 3;
        }
        else if (nWord == kPlanMarkerString)
        {
            nAt++;

            while (nAt < words.size() && words[nAt] != 0)
            {
                nAt++;
            }

            nAt++;
            nPending--;
        }
        else
        {
            nAt++;
            nPending--;
        }
    }

    if (nPending != 0 || nAt > words.size())
    {
        return kNone;
    }

    return nAt;
}

// The top-level nodes run to END_PLAN and fill the words exactly.
bool Parses(std::span<const int> words)
{
    auto nAt = size_t{ 0 };

    while (nAt != kNone && nAt + 1 < words.size())
    {
        if (words[nAt] == kNodeMarker && static_cast<unsigned int>(words[nAt + 1]) == CATEGORY_END_PLAN)
        {
            return nAt + 2 == words.size();
        }

        nAt = End(words, nAt);
    }

    return false;
}

struct Splice
{
    size_t           nAt;
    size_t           nRemove;
    std::vector<int> words;
};

// Every position is a word offset in the shipped file. Any check that fails spoils the whole plan.
class Patcher
{
public:
    explicit Patcher(std::span<const int> words)
    : m_words(words)
    {
    }

    std::vector<int> Copy(size_t nNode, int nOpcode)
    {
        if (!Expect(nNode, nOpcode))
        {
            return {};
        }

        return Span(nNode);
    }

    std::vector<int> CopyChild(size_t nNode, int nOpcode, int nChild)
    {
        const auto nAt = Child(nNode, nOpcode, nChild);

        if (nAt == kNone)
        {
            return {};
        }

        return Span(nAt);
    }

    void SetLeaf(size_t nNode, int nOpcode, int nChild, int nWas, int nNow)
    {
        const auto nAt = Child(nNode, nOpcode, nChild);

        if (nAt == kNone || m_words[nAt] != nWas)
        {
            m_bOK = false;

            return;
        }

        m_splices.push_back({ nAt, 1, { nNow } });
    }

    void AddLeaf(size_t nNode, int nOpcode, int nLeaf)
    {
        InsertChildren(nNode, nOpcode, Count(nNode), { { nLeaf } });
    }

    void InsertChildren(size_t nNode, int nOpcode, int nChild, const std::vector<std::vector<int> >& children)
    {
        const auto nAt = Child(nNode, nOpcode, nChild);

        if (nAt == kNone)
        {
            return;
        }

        auto words = std::vector<int>();

        for (const auto& child : children)
        {
            if (child.empty())
            {
                m_bOK = false;

                return;
            }

            words.insert(words.end(), child.begin(), child.end());
        }

        SetCount(nNode, static_cast<int>(children.size()));
        m_splices.push_back({ nAt, 0, std::move(words) });
    }

    void AppendChildren(size_t nNode, int nOpcode, const std::vector<std::vector<int> >& children)
    {
        InsertChildren(nNode, nOpcode, Count(nNode), children);
    }

    void RemoveChild(size_t nNode, int nOpcode, int nChild, int nChildOpcode)
    {
        const auto nAt = Child(nNode, nOpcode, nChild);

        if (nAt == kNone || !Expect(nAt, nChildOpcode))
        {
            m_bOK = false;

            return;
        }

        SetCount(nNode, -1);
        m_splices.push_back({ nAt, End(m_words, nAt) - nAt, {} });
    }

    void SwapChildren(size_t nNode, int nOpcode, int nFirst, int nSecond)
    {
        auto first  = CopyChild(nNode, nOpcode, nFirst);
        auto second = CopyChild(nNode, nOpcode, nSecond);

        if (first.empty() || second.empty())
        {
            m_bOK = false;

            return;
        }

        const auto nFirstAt  = Child(nNode, nOpcode, nFirst);
        const auto nSecondAt = Child(nNode, nOpcode, nSecond);

        m_splices.push_back({ nFirstAt, first.size(), std::move(second) });
        m_splices.push_back({ nSecondAt, End(m_words, nSecondAt) - nSecondAt, std::move(first) });
    }

    // The node becomes a Block of itself followed by tail.
    void WrapInBlock(size_t nNode, int nOpcode, std::vector<int> tail)
    {
        if (!Expect(nNode, nOpcode) || tail.empty())
        {
            m_bOK = false;

            return;
        }

        m_splices.push_back({ nNode, 0, { kNodeMarker, kBlock, 2 } });
        m_splices.push_back({ End(m_words, nNode), 0, std::move(tail) });
    }

    // Each node of the exact six words is rewritten into the five-leaf form.
    void RewriteAll(std::span<const int> pattern, const std::vector<int>& tail, int nExpected)
    {
        auto nFound = 0;

        for (size_t nAt = 0; nAt + pattern.size() <= m_words.size(); nAt++)
        {
            if (std::ranges::equal(pattern, m_words.subspan(nAt, pattern.size())))
            {
                SetCount(nAt, static_cast<int>(tail.size()) - 1);
                m_splices.push_back({ nAt + pattern.size() - 1, 1, tail });
                nFound++;
            }
        }

        if (nFound != nExpected)
        {
            m_bOK = false;
        }
    }

    std::optional<std::vector<int> > Result()
    {
        if (!m_bOK)
        {
            return std::nullopt;
        }

        // Last word first, so no splice moves one still to come. Inserts at one word apply latest
        // first, which leaves them in the order they were made.
        std::ranges::reverse(m_splices);
        std::ranges::stable_sort(
            m_splices,
            [](const Splice& a, const Splice& b)
            {
                return a.nAt > b.nAt;
            });

        auto words  = std::vector<int>(m_words.begin(), m_words.end());
        auto nFloor = words.size();

        for (const auto& splice : m_splices)
        {
            if (splice.nAt + splice.nRemove > nFloor)
            {
                return std::nullopt;
            }

            words.erase(words.begin() + splice.nAt, words.begin() + splice.nAt + splice.nRemove);
            words.insert(words.begin() + splice.nAt, splice.words.begin(), splice.words.end());

            nFloor = splice.nAt;
        }

        if (!Parses(words))
        {
            return std::nullopt;
        }

        return words;
    }

private:
    std::vector<int> Span(size_t nAt)
    {
        const auto nEnd = End(m_words, nAt);

        if (nEnd == kNone)
        {
            m_bOK = false;

            return {};
        }

        return std::vector<int>(m_words.begin() + nAt, m_words.begin() + nEnd);
    }

    bool Expect(size_t nNode, int nOpcode)
    {
        if (nNode + 2 >= m_words.size() || m_words[nNode] != kNodeMarker || m_words[nNode + 1] != nOpcode)
        {
            m_bOK = false;

            return false;
        }

        return true;
    }

    int Count(size_t nNode) const
    {
        if (nNode + 2 >= m_words.size())
        {
            return 0;
        }

        return m_words[nNode + 2] & 0x7fff;
    }

    // Where child nChild starts; a child one past the last is where a new one would go.
    size_t Child(size_t nNode, int nOpcode, int nChild)
    {
        if (!Expect(nNode, nOpcode) || nChild < 0 || nChild > Count(nNode))
        {
            m_bOK = false;

            return kNone;
        }

        auto nAt = nNode + 3;

        for (auto i = 0; i < nChild && nAt != kNone; i++)
        {
            nAt = End(m_words, nAt);
        }

        if (nAt == kNone)
        {
            m_bOK = false;
        }

        return nAt;
    }

    // One count change per node: a second would splice the same word twice.
    void SetCount(size_t nNode, int nDelta)
    {
        m_splices.push_back({ nNode + 2, 1, { m_words[nNode + 2] + nDelta } });
    }

    std::span<const int> m_words;
    std::vector<Splice>  m_splices;
    bool                 m_bOK = true;
};

// The receiver checks read the message data as the receiver's slot, and these timers carry none.
void FixCarryRok(Patcher& p)
{
    p.AddLeaf(1504, kTimeout, PronounWord(PRN_MY_TEAMMATE));
    p.AddLeaf(2950, kTimeout, PronounWord(PRN_MY_TEAMMATE));

    // A network replay reads a second ShootSituation branch of 1 as 4.
    for (const auto nDecide : { 1634, 1683 })
    {
        const auto arm = p.CopyChild(nDecide, kDecide, 2);

        p.AppendChildren(nDecide, kDecide, { arm, arm, arm });
    }
}

// Three-leaf avoidance has no range, and its second target is slot 9, the camera.
void AvoidTeammateOnly(Patcher& p, int nExpected)
{
    constexpr auto pattern = std::array{ kNodeMarker, kAddAgenda, 3, static_cast<int>(Agenda_CAvoidObjects), PronounWord(PRN_MY_TEAMMATE), 9 };

    p.RewriteAll(pattern, { 0, std::bit_cast<int>(80.0f), std::bit_cast<int>(-6.5f) }, nExpected);
}

void FixDefense(Patcher& p)
{
    // Attack behaviours name opponents by slot where a collision object belongs.
    for (const auto nNode : { 2293, 2992, 5379 })
    {
        p.SetLeaf(nNode, kCollision, 0, PronounWord(PRN_MY_LEAD_OPPONENT), PronounWord(PRN_LEAD_OPPN_BODY_BUB));
    }

    for (const auto nNode : { 2332, 3012, 5399 })
    {
        p.SetLeaf(nNode, kCollision, 0, PronounWord(PRN_MY_OTHER_OPPONENT), PronounWord(PRN_OTHER_OPPN_BODY_BUB));
    }

    // Substitution only ran with the clock going, and only a face-off with the clock stopped asks.
    p.SwapChildren(191, kDecide, 1, 2);

    AvoidTeammateOnly(p, 8);
}

void FixOffWing(Patcher& p)
{
    AvoidTeammateOnly(p, 9);
}

void FixMultiplier(Patcher& p)
{
    p.AddLeaf(687, kTimeout, PLAN_TOKEN_MESSAGE_DATA);
    p.AddLeaf(792, kTimeout, PLAN_TOKEN_MESSAGE_DATA);

    // ScoreArming's spin goes from YerHit, which carries the hitter, not Initiate, which carries 0.
    p.InsertChildren(831, kEnable, 0, { p.Copy(746, kAction) });
    p.RemoveChild(743, kEnable, 0, kAction);
}

void FixTurnstile(Patcher& p)
{
    p.RemoveChild(421, kEnable, 0, kAction);
    p.AddLeaf(611, kTimeout, PLAN_TOKEN_MESSAGE_DATA);
}

// Flying has no TimeOutMessage handler; Extra1 kicks the goalie down.
void FixGoalie(Patcher& p)
{
    p.SetLeaf(1032, kTimeout, 1, kMsgTimeOut, kMsgExtra1);
}

// Removing every timer also removed the 45 second RedIsUp.
void FixRedRandomizer(Patcher& p)
{
    p.RemoveChild(854, kEnable, 0, kRemoveAgenda);
}

// A refused throw's _NO_BEH_CHANGE_ sticks unless the throw arm names Flying itself.
void FixRok(Patcher& p)
{
    p.AppendChildren(1051, kBlock, { p.Copy(1036, kChange) });
}

// An own goal's _NO_BEH_CHANGE_ sticks and skips every later post-goal sequence.
void FixGoal(Patcher& p)
{
    for (const auto nNode : { 73, 140, 213, 298 })
    {
        p.SetLeaf(nNode, kChange, 0, PLAN_TOKEN_NO_BEH_CHANGE, 1);
    }
}

void FixArmory(Patcher& p)
{
    const auto open = p.Copy(45, kChange);

    p.WrapInBlock(33, kAction, open);
    p.WrapInBlock(39, kAction, open);
}

void FixSwarf(Patcher& p)
{
    p.SetLeaf(202, kTimeout, 2, 52, 53);
}

void FixCamera(Patcher& p)
{
    for (const auto nNode : { 743, 791 })
    {
        p.SetLeaf(nNode, kChange, 0, PLAN_TOKEN_NO_BEH_CHANGE, 2);
    }
}

struct PlanFixes
{
    std::string_view name;
    size_t           nWords;
    uint32_t         nHash;
    void (*pfnFix)(Patcher&);
};

constexpr auto kPlans = std::array{
    PlanFixes{ "CARRYROK", 4431, 0x197e11b2, FixCarryRok },
    PlanFixes{ "DEFENSE", 6163, 0xae90237d, FixDefense },
    PlanFixes{ "OFFWING", 4969, 0xc81e30f0, FixOffWing },
    PlanFixes{ "MULTPLR", 1081, 0xf7871b28, FixMultiplier },
    PlanFixes{ "TURNSTIL", 890, 0xda58956b, FixTurnstile },
    PlanFixes{ "GOALIE", 2816, 0x5838f35b, FixGoalie },
    PlanFixes{ "RRANDOM", 867, 0x25131520, FixRedRandomizer },
    PlanFixes{ "ROK", 3124, 0x8c450018, FixRok },
    PlanFixes{ "NGOAL", 626, 0x9c7ec799, FixGoal },
    PlanFixes{ "SGOAL", 626, 0x3002ecb3, FixGoal },
    PlanFixes{ "ARMORY", 243, 0xd05b8c07, FixArmory },
    PlanFixes{ "SWARF", 638, 0xf7099f32, FixSwarf },
    PlanFixes{ "CAMERA", 1124, 0x8c638342, FixCamera },
};

// FNV-1a over whole words.
uint32_t Hash(std::span<const int> words)
{
    auto nHash = uint32_t{ 0x811c9dc5 };

    for (const auto nWord : words)
    {
        nHash = (nHash ^ static_cast<uint32_t>(nWord)) * 16777619u;
    }

    return nHash;
}

bool SameName(std::string_view a, std::string_view b)
{
    return std::ranges::equal(
        a,
        b,
        [](char x, char y)
        {
            return std::toupper(static_cast<unsigned char>(x)) == std::toupper(static_cast<unsigned char>(y));
        });
}

} // namespace

std::optional<std::vector<int> > PlanPatch::Fixed(std::string_view name, std::span<const int> words)
{
    for (const auto& plan : kPlans)
    {
        if (!SameName(plan.name, name))
        {
            continue;
        }

        if (words.size() != plan.nWords || Hash(words) != plan.nHash)
        {
            return std::nullopt;
        }

        auto patcher = Patcher(words);

        plan.pfnFix(patcher);

        return patcher.Result();
    }

    return std::nullopt;
}

std::vector<std::string_view> PlanPatch::Names()
{
    auto names = std::vector<std::string_view>();

    for (const auto& plan : kPlans)
    {
        names.push_back(plan.name);
    }

    return names;
}
