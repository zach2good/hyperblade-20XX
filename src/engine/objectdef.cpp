// The object definition record's constructor and destructor.

#include "common/compat.h"
#include "common/staticinit.h"
#include "objectdef.h"
#include <string.h>
#include "gameengine.h"
#include "physics/bubblecmd.h"

// FUNCTION: HYPERX 0x00484580
ObjectDef::ObjectDef(const char* pszModel)
{
    m_pszModel       = 0;
    m_pszMaterial    = 0;
    m_pszName        = 0;
    m_nTypeId        = 0;
    m_nRatingsMode   = 0;
    m_bOwnPalette    = 0;
    m_wUnused        = 0;
    m_nPlanCount     = 0;
    m_pCollisionList = 0;
    m_apszPlanNames  = 0;

    m_flElasticity = g_flZeroGameEngine;
    m_flFriction   = g_flZeroGameEngine;
    m_flElevation  = g_flZeroGameEngine;
    m_flAzimuth    = g_flZeroGameEngine;
    m_flSpeed      = g_flZeroGameEngine;
    m_flAccel      = g_flZeroGameEngine;
    m_vPos.x = m_vPos.y = m_vPos.z = g_flZeroGameEngine;

    m_bForceRemap  = 0;
    m_apRatings[0] = m_apRatings[1] = m_apRatings[2] = m_apRatings[3] = 0;

    if (pszModel != 0)
    {
        char* const pOwned = new char[strlen(pszModel) + 1];
        strcpy(pOwned, pszModel);

        m_pszModel = pOwned;
    }
}

// FUNCTION: HYPERX 0x00484660
ObjectDef::~ObjectDef()
{
    m_nTypeId = 0;

    if (m_pszName != 0)
    {
        delete[] m_pszName;
        m_pszName = 0;
    }

    if (m_pszModel != 0)
    {
        delete[] m_pszModel;
        m_pszModel = 0;
    }

    if (m_pszMaterial != 0)
    {
        delete[] m_pszMaterial;
        m_pszMaterial = 0;
    }

    delete m_pCollisionList;

    if (m_apszPlanNames != 0)
    {
        while (m_nPlanCount-- != 0)
        {
            delete[] m_apszPlanNames[m_nPlanCount];
        }

        delete[] m_apszPlanNames;
    }

    for (int i = 0; i < 4; i++)
    {
        if (m_apRatings[i] != 0)
        {
            delete[] m_apRatings[i];
        }
    }
}
