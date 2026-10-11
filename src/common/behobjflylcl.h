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

    // Folded onto BehObj_Fly::SendTimedMessage in the original, so it carries no marker.
    virtual void SendTimedMessage()
    {
        m_pOwner->PostEvent(MSG_INITIATE, 0, m_pOwner->GetSlot());

        m_wPollCountdown = m_wLatency;

        if (m_pClipA != 0)
        {
            m_pOwner->StartClipPlayback(m_pClipA,
                                        m_flRateA,
                                        m_nClipFlagsA);
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
