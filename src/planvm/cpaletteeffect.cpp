#include "common/compat.h"
#include "graphics/bitmapformat.h"
#include "planvm/agendaitemids.h"
#include "planvm/cpaletteeffect.h"
#include "globals.h"
#include "graphics/palettefade.h"
#include "engine/gameengine.h"
#include "graphics/colorpalette.h"
#include "common/stub.h"
#include "graphics/visualeffectfade.h"
#include "common/pooledownerroot.h"

// The four effect kinds the jump table covers, three of them scalar.
enum EffectKind
{
    EFFECT_SCALAR_A = 1,
    EFFECT_PLAIN    = 2,
    EFFECT_SCALAR_B = 3,
    EFFECT_SCALAR_C = 4,
};

// FUNCTION: HYPERX 0x00410b10
void CPaletteEffect::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind       = Agenda_CPaletteEffect;
    m_pNext       = nullptr;
    m_bReady      = 0;
    m_bPersistent = 1;
    m_pOwner      = pSubject;
}

// FUNCTION: HYPERX 0x00410b30
void CPaletteEffect::OnListChanged()
{
    if (m_bReady != 0)
    {
        return;
    }

    auto* const pHeld = m_pEffect;

    if (pHeld != nullptr)
    {
        delete pHeld;

        m_pEffect = nullptr;
    }

    m_nExpiry = 0;
    m_pOwner  = nullptr;
    m_pNext   = nullptr;

    m_bReady = 1;

    g_apAgendaItemPools[m_wKind]->Insert(this);
}

// Kind two's fade runs five hundred where the constructor sets a thousand.
// FUNCTION: HYPERX 0x00410b70
int CPaletteEffect::CreateVisualEffect(int nKind, int nAnim, PaletteEntry peStart, int nDuration)
{
    auto anRange = g_pColorPalette->GetFrameRange(nAnim);

    if (anRange.nFirst != 0 || anRange.nCount != 0)
    {
        switch (nKind)
        {
            case EFFECT_SCALAR_A:
            case EFFECT_SCALAR_B:
            case EFFECT_SCALAR_C:
                m_pEffect = new PaletteScalarFade(&anRange);
                break;

            case EFFECT_PLAIN:
                m_pEffect = new VisualEffectFade(&anRange);
                break;

            default:
                return 0;
        }

        if (m_pEffect->Start(&peStart) == 0)
        {
            delete m_pEffect;

            m_pEffect = nullptr;

            return 0;
        }

        m_nExpiry = g_pGameEngine->m_clock.nTickMs + nDuration;

        return 1;
    }

    return 0;
}

// The deadline comparison is unsigned, so a wrapped deadline keeps the effect alive.
// FUNCTION: HYPERX 0x00410cf0
void CPaletteEffect::Process(int nArg)
{
    m_pEffect->Tick();

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }

    if (g_pGameEngine->m_clock.nTickMs > m_nExpiry)
    {
        Detach();
    }
}
