#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "engine/engineobject.h"
#include "engine/messageids.h"
#include "engine/modelids.h"

// FUNCTION: HYPERX 0x0041e600
BehObj_OpenAndClose::BehObj_OpenAndClose()
{
    m_wLatency = 1;
}

// FUNCTION: HYPERX 0x0041e650
BehObj_OpenAndClose::~BehObj_OpenAndClose() = default;

// FUNCTION: HYPERX 0x0041e660
void BehObj_OpenAndClose::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc380
    m_pszName = "OpenAndClose";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind      = BEH_OPEN_AND_CLOSE;
    m_apSlots[0] = nullptr;
}

// FUNCTION: HYPERX 0x0041e6b0
void BehObj_OpenAndClose::SendTimedMessage()
{
    m_pOwner->PostEvent(MSG_INITIATE, 0, m_pOwner->GetSlot());

    m_wPollCountdown = m_wLatency;

    if (m_pAnimation != nullptr)
    {
        m_pOwner->StartClipPlayback(m_pAnimation,
                                    m_flAnimationRate,
                                    m_nAnimationFlags);

        if (m_nEventTarget != MODEL_ID_NONE && m_nEventKind != MSG_NOMSG)
        {
            m_pOwner->BindAnimEventChannel(0,
                                           m_nEventTarget,
                                           m_nEventKind,
                                           m_nEventParam);
        }
    }
}
