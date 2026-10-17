#ifndef HYPERBLADE_BEHOBJFLYLCL_H
#define HYPERBLADE_BEHOBJFLYLCL_H

#include "common/flightbehaviorobject.h"
#include "engine/engineobject.h"
#include "engine/messageids.h"
#include "engine/modelids.h"

class EngineObject;

// VTABLE: HYPERX 0x004b26b4
class BehObj_FlyLCL : public FlightBehaviorObject
{
public:
    ~BehObj_FlyLCL() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;

    // Folded onto BehObj_Fly::SendTimedMessage in the original, so it carries no marker.
    void SendTimedMessage() override
    {
        m_pOwner->PostEvent(MSG_INITIATE, 0, m_pOwner->GetSlot());

        m_wPollCountdown = m_wLatency;

        if (m_pAnimation != 0)
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

    BehObj_FlyLCL();
};

// SYNTHETIC: HYPERX 0x0041dd60
// BehObj_FlyLCL::`scalar deleting destructor'

#endif
