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
int CPaletteEffect::CreateVisualEffect(EffectKind kind, int nAnim, PaletteEntry peStart, int nDuration)
{
    const auto anRange = g_pColorPalette->GetFrameRange(nAnim);

    if (anRange.nFirst != 0 || anRange.nCount != 0)
    {
        switch (kind)
        {
            case EffectKind::kScalarA:
            case EffectKind::kScalarB:
            case EffectKind::kScalarC:
                m_pEffect = new PaletteScalarFade(&anRange);
                break;

            case EffectKind::kPlain:
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
