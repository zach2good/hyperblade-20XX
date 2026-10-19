#ifndef HYPERBLADE_PLANSTATE_H
#define HYPERBLADE_PLANSTATE_H

#include <memory>
#include <vector>

#include "planvm/plannode.h"

class EngineObject;

// Ticks after any interactor runs before every collision interactor is set active again.
constexpr auto kCollisionRearmTicksAfterRun = 0x0c;

// Everything one compile allocates; the tree inside it only points.
struct CompiledPlan
{
    std::vector<std::unique_ptr<PlanNode> >   apNodes;
    std::vector<std::unique_ptr<PlanWord[]> > apArgs;
    std::vector<std::unique_ptr<char[]> >     apStrings;
    std::unique_ptr<PlanNode*[]>              apBehaviors;
};

class PlanState
{
public:
    PlanState();
    ~PlanState();

    void                      ActivateBehavior(unsigned int nIndex);
    class GameBehaviorObject* GetBehaviorByIndex(unsigned int nIndex);
    class CollInteractor*     WalkLinkedList(short nIndex);
    void                      Teardown();
    PlanState*                Instantiate();
    void                      SetOwnerAndReset(class EngineObject* pOwner, struct PlanNode** ppBehaviors);
    void                      Reinitialize();
    unsigned char             SetAndInit(const union PlanWord* pStream);
    struct PlanNode**         Compile(const union PlanWord* pStream);

    char ParseNodeStream(int nDepth, const union PlanWord** ppStream, union PlanWord* pOut, char* pcPlanRef);

    void PostUpdateCleanup();

    const char* GetName() const
    {
        return m_pszName;
    }

    // Sixty bytes an entry, the stride EngineObject::GetPlanState indexes by.
    class EngineObject* m_pOwner; // 0x00
    // A name: fifteen bodies read it as a string.
    char* m_pszName; // 0x04
    // The engine tick copies this word into the object's own m_wSyncedPlan every pass.
    short                     m_wSyncValue;     // 0x08
    short                     m_nBehaviorCount; // 0x0a
    struct PlanNode**         m_ppBehaviors;    // 0x0c
    struct PlanNode**         m_ppCursor;       // 0x10
    class GameBehaviorObject* m_pHead;          // 0x14
    class GameBehaviorObject* m_pCurrent;       // 0x18
    short                     m_nIndex;         // 0x1c
    short                     m_nPrevious;      // 0x1e
    // The plan's global interactors, chained as GameBehaviorObject chains its own.
    class CollInteractor*    m_pCollInteractors;     // 0x20
    class MsgInteractor*     m_pMsgInteractors;      // 0x24
    short                    m_nInteractorCount;     // 0x28
    short                    m_nCollisionRearmTicks; // 0x2a
    std::unique_ptr<float[]> m_pflLocalReals;        // 0x2c
    short                    m_nLocalRealCount;      // 0x30
    // One interactor per name that id_SET_InteractorName gives; plancond.cpp reads them back.
    std::unique_ptr<class Interactor*[]> m_apNamedInteractors; // 0x34
    short                                m_nNamedInteractors;  // 0x38

private:
    // The set m_ppBehaviors points into, and the one the last compile built.
    CompiledPlan m_compiled;
    CompiledPlan m_pending;
};

#endif
