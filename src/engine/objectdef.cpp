// The object definition record's constructor and destructor.

#include "common/compat.h"
#include "engine/objectkind.h"
#include "common/staticinit.h"
#include "objectdef.h"
#include <cstring>
#include "gameengine.h"
#include "physics/bubblecmd.h"

// FUNCTION: HYPERX 0x00484580
ObjectDef::ObjectDef(const char* pszModel)
{
    m_nType        = 0;
    m_nTypeId      = ObjectKind::kNull;
    m_nRatingsMode = 0;
    m_bOwnPalette  = 0;
    m_cUnused      = 0;
    m_wUnused      = 0;
    m_nPlanCount   = 0;

    m_flElasticity = g_flZeroGameEngine;
    m_flFriction   = g_flZeroGameEngine;
    m_flElevation  = g_flZeroGameEngine;
    m_flAzimuth    = g_flZeroGameEngine;
    m_flSpeed      = g_flZeroGameEngine;
    m_flAccel      = g_flZeroGameEngine;
    m_vPos.x = m_vPos.y = m_vPos.z = g_flZeroGameEngine;

    m_bForceRemap = 0;

    if (pszModel != nullptr)
    {
        m_pszModel = std::make_unique<char[]>(strlen(pszModel) + 1);

        strcpy(m_pszModel.get(), pszModel);
    }
}

// FUNCTION: HYPERX 0x00484660
ObjectDef::~ObjectDef() = default;
