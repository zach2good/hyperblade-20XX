// Both goals: hidden, each part posed at its rest position and then the whole.

#include "common/compat.h"
#include "devicector.h"
#include "graphics/meshbone.h"
#include "objectclasses.h"
#include "engine/goalobject.h"
#include "engine/engineobject.h"

// FUNCTION: HYPERX 0x00474490
GoalObject::GoalObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_nFlags |= ENGINEOBJECT_HIDDEN;

    for (int nIndex = 0; nIndex < m_wBoneCount; nIndex++)
    {
        const Vec3* const pRest = &m_pBones[nIndex].m_pDesc->m_vRest;

        m_pBones[nIndex].m_local.SetTranslation(pRest->x, pRest->y, pRest->z);
    }

    ApplyRenderSlotPose();
}

// FUNCTION: HYPERX 0x00474560
GoalObject::~GoalObject() = default;
