#ifndef HYPERBLADE_BEHOBJFLY_H
#define HYPERBLADE_BEHOBJFLY_H

#include "common/flightbehaviorobject.h"
#include "engine/engineobject.h"
#include "engine/messageids.h"
#include "engine/modelids.h"

class EngineObject;

// VTABLE: HYPERX 0x004b269c
class BehObj_Fly : public FlightBehaviorObject
{
public:
    ~BehObj_Fly() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;

    // FUNCTION: HYPERX 0x0041dce0
    void SendTimedMessage() override
    {
        m_pOwner->PostEvent(MSG_INITIATE, 0, m_pOwner->GetSlot());

        m_wPollCountdown = m_wLatency;

        if (m_pAnimation != nullptr)
        {
            m_pOwner->StartClipPlayback(m_pAnimation,
                                        m_flAnimationRate,
                                        m_nAnimationFlags);
        }

        if (m_nEventTarget != MODEL_ID_NONE && m_nEventKind != MSG_NOMSG)
        {
            m_pOwner->BindAnimEventChannel(0,
                                           m_nEventTarget,
                                           m_nEventKind,
                                           m_nEventParam);
        }
    }

    BehObj_Fly();
};

// SYNTHETIC: HYPERX 0x0041dc30
// BehObj_Fly::`scalar deleting destructor'

#endif
