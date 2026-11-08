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
    virtual ~BehObj_FlyLCL();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);

    // v1.0.0.1 folds it onto BehObj_Fly::SendTimedMessage.
    // FUNCTION: HYPERX100 0x004cb900
    virtual void SendTimedMessage()
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

STATIC_ASSERT(sizeof(BehObj_FlyLCL) == 0x98);

#endif
