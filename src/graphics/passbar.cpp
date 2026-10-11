#include "common/compat.h"
#include "graphics/passbar.h"

// FUNCTION: HYPERX 0x00438f40
int PassBar::DrawAtPos(int nPosition)
{
    if (m_bVisible != 0)
    {
        CDDBitMap::Draw();

        return UpdateScrollPos(nPosition);
    }

    return 0;
}

// FUNCTION: HYPERX 0x00438f70
int PassBar::AutoScroll()
{
    if (m_bVisible != 0)
    {
        CDDBitMap::Draw();

        return UpdateScrollPos(m_nAutoPosition);
    }

    return 0;
}

// FUNCTION: HYPERX 0x00438fa0
int PassBar::UpdateScrollPos(int nPosition)
{
    int nChanged = 0;
    int nOther;

    m_nPosition = nPosition;

    if (nPosition > m_ptRange.y)
    {
        m_nPosition = m_ptRange.y;
        nChanged    = 1;
    }

    const int nLower = m_ptRange.x;

    if (m_nPosition < nLower)
    {
        m_nPosition = nLower;
        nChanged    = 1;
    }

    if (m_bHorizontal != 0)
    {
        nOther = m_ptInset.x;

        m_ptMarker.x = (int)((float)(nLower + m_nPosition) * m_flScale) - m_ptMarkerOffset.x + m_ptPosition.x + nOther;

        if (m_ptMarker.x > nOther)
        {
            m_pMarkerTexture->SetPosition(m_ptMarker);

            m_pMarkerTexture->Draw();
        }
    }
    else
    {
        nOther = m_ptInset.y;

        m_ptMarker.y = m_ptPosition.y - (int)((float)(nLower + m_nPosition) * m_flScale) - m_ptMarkerOffset.y - nOther + m_ptSize.y;

        if (m_ptMarker.y < m_ptSize.y - nOther)
        {
            m_pMarkerTexture->SetPosition(m_ptMarker);

            m_pMarkerTexture->Draw();
        }
    }

    return nChanged;
}
