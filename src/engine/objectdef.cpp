// The object definition record's constructor and destructor.

#include "common/compat.h"
#include "common/staticinit.h"
#include "objectdef.h"
#include <cstring>
#include "gameengine.h"
#include "physics/bubblecmd.h"

// FUNCTION: HYPERX 0x00484580
ObjectDef::ObjectDef(const char* pszModel)
{
    m_pszModel       = nullptr;
    m_pszMaterial    = nullptr;
    m_pszName        = nullptr;
    m_nTypeId        = 0;
    m_nRatingsMode   = 0;
    m_bOwnPalette    = 0;
    m_wUnused        = 0;
    m_nPlanCount     = 0;
    m_pCollisionList = nullptr;
    m_apszPlanNames  = nullptr;

    m_flElasticity = g_flZeroGameEngine;
    m_flFriction   = g_flZeroGameEngine;
    m_flElevation  = g_flZeroGameEngine;
    m_flAzimuth    = g_flZeroGameEngine;
    m_flSpeed      = g_flZeroGameEngine;
    m_flAccel      = g_flZeroGameEngine;
    m_vPos.x = m_vPos.y = m_vPos.z = g_flZeroGameEngine;

    m_bForceRemap  = 0;
    m_apRatings[0] = m_apRatings[1] = m_apRatings[2] = m_apRatings[3] = nullptr;

    if (pszModel != nullptr)
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

    if (m_pszName != nullptr)
    {
        delete[] m_pszName;
        m_pszName = nullptr;
    }

    if (m_pszModel != nullptr)
    {
        delete[] m_pszModel;
        m_pszModel = nullptr;
    }

    if (m_pszMaterial != nullptr)
    {
        delete[] m_pszMaterial;
        m_pszMaterial = nullptr;
    }

    delete m_pCollisionList;

    if (m_apszPlanNames != nullptr)
    {
        while (m_nPlanCount-- != 0)
        {
            delete[] m_apszPlanNames[m_nPlanCount];
        }

        delete[] m_apszPlanNames;
    }

    for (int i = 0; i < 4; i++)
    {
        if (m_apRatings[i] != nullptr)
        {
            delete[] m_apRatings[i];
        }
    }
}
