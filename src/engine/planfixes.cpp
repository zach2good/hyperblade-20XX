#include "common/compat.h"
#include "engine/planfixes.h"
#include "engine/engineobject.h"
#include "engine/gameengine.h"
#include "globals.h"

namespace
{

bool s_bOn = true;

} // namespace

bool PlanFixes::On()
{
    return s_bOn;
}

void PlanFixes::Set(bool bOn)
{
    if (bOn == s_bOn || g_bSessionActive != 0)
    {
        return;
    }

    s_bOn = bOn;

    if (g_pGameEngine == nullptr || g_pGameEngine->m_ppObjects == nullptr)
    {
        return;
    }

    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        g_pGameEngine->m_ppObjects[i]->SwapPlanSet();
    }
}
