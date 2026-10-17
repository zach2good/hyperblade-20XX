#include "common/compat.h"
#include "framepos.h"
#include "widget.h"
#include "ui/screen.h"
#include "globals.h"
#include "pausemenu.h"
#include "sound/audiomgr.h"
#include "engine/gameengine.h"
#include "buttonwidget.h"
#include "torchbitmaps.h"
#include "saveload.h"
#include "planvm/planvm.h"
#include <string.h>
#include "graphics/gdi.h"
#include "common/stub.h"
#include "common/staticinit.h"
#include <cstdlib>

float g_flFifthWidget             = 0.2f * g_flOneWidget;
float g_flDotPitchInchWidget      = g_flDotsPerInchWidget * g_flDotPitchWidget;
float g_flScreenDotsPerInchWidget = g_flDotsPerInchWidget;
float g_flDotPitchCmWidget        = g_flDotPitchInchWidget / g_flCmPerInchWidget;
float g_flScreenDotsPerCmWidget   = g_flScreenDotsPerInchWidget / g_flCmPerInchWidget;

// The first pass nulls duplicates and the screen's background; the second frees the rest.
// FUNCTION: HYPERX 0x0042b110
Widget::~Widget()
{
    int i;

    if (m_pFrames != 0)
    {
        GDI::Bitmap* hBackground = 0;

        if (m_pOwner != 0)
        {
            hBackground = m_pOwner->m_hBackground;
        }

        for (i = 0; i <= m_nFrameCount; i++)
        {
            GDI::Bitmap* const hFrame = m_pFrames[i].hBitmap;

            if (hFrame != 0)
            {
                if (hFrame == hBackground)
                {
                    m_pFrames[i].hBitmap = 0;
                }
                else
                {
                    for (int j = i + 1; j <= m_nFrameCount; j++)
                    {
                        if (m_pFrames[j].hBitmap == hFrame || m_pFrames[j].hBitmap == hBackground)
                        {
                            m_pFrames[j].hBitmap = 0;
                        }
                    }
                }
            }
        }

        for (i = 0; i <= m_nFrameCount; i++)
        {
            if (m_pFrames[i].hBitmap != 0)
            {
                if (m_pFrames[i].bCached == 0)
                {
                    GDI::SafeDeleteBitmap(m_pFrames[i].hBitmap);
                }

                m_pFrames[i].hBitmap = 0;
            }
        }

        delete[] m_pFrames;

        m_pFrames = 0;
    }

    if (m_pszText != 0)
    {
        Mem::SafeDelete(m_pszText);

        m_pszText = 0;
    }
}

// The seventh argument is the row count; one extra row is allocated, even for a negative count.
// FUNCTION: HYPERX 0x0042b220
Widget::Widget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
{
    m_nX                = nX;
    m_nY                = nY;
    m_nWidth            = nWidth;
    m_nHeight           = nHeight;
    m_bHighlighted      = nHighlighted;
    m_bDepressed        = nDepressed;
    m_nFrameCount       = nRows;
    m_nFrameIdle        = nFrameIdle;
    m_nType             = WIDGET_FLIPBOOK;
    m_pszText           = 0;
    m_nTextX            = 0;
    m_nTextY            = 0;
    m_nFrame            = 0;
    m_nFrameHighlighted = nFrameHighlighted;
    m_nTargetFrame      = nTargetFrame;

    m_pFrames = new WidgetDrawFrame[nRows + 1];
    m_bHidden = 0;

    for (int i = 0; i <= m_nFrameCount; i++)
    {
        m_pFrames[i].hBitmap = 0;
    }

    m_pOwner                          = pOwner;
    m_nFrameIndex                     = 0;
    m_nTextFlag                       = 0;
    m_flipbook.m_nSoundPastFirstLimit = -1;
    m_flipbook.m_nSoundBeforeTarget   = -1;
    m_flipbook.m_nMarkFrame           = -1;
    m_bDirty                          = 1;
    m_flipbook.m_nSoundAtMark         = nSoundAtMark;
}

// Half open at the far edge. m_bHidden set to one also blocks the hit test.
// FUNCTION: HYPERX 0x0042b2f0
int Widget::HitTest(int nX, int nY)
{
    if (m_bHidden != 1)
    {
        if (nY >= m_nY)
        {
            if (nY < m_nY + m_nHeight)
            {
                if (nX >= m_nX)
                {
                    if (nX < m_nX + m_nWidth)
                    {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x0042b330
int Widget::TryActivate(int, int)
{
    if (m_bHidden != 1 && m_bHighlighted == 0)
    {
        m_pOwner->ResetAllHighlights(0);

        m_bHighlighted = 1;
        m_bDirty       = 1;

        return 1;
    }

    return 0;
}

// A step sets its limit before testing its event, so a mode with no event still moves the limit.
// FUNCTION: HYPERX 0x0042b370
int Widget::AdvanceFrame()
{
    const FlipbookSounds* const pSounds = &m_flipbook;

    if (m_bHidden == 1)
    {
        return 0;
    }

    int nLimit = m_nFrameIdle;

    if (m_bHighlighted == 1)
    {
        nLimit = m_nFrameHighlighted;

        if (m_nType == WIDGET_FLIPBOOK && m_nFrameCount > 2 && pSounds->m_nSoundPastFirstLimit > -1 && m_nFrame == m_nFrameIdle + 1)
        {
            g_pAudioMgr->PlaySpecific(pSounds->m_nSoundPastFirstLimit);
        }
    }

    if (m_bDepressed == 1)
    {
        nLimit = m_nTargetFrame;

        if (m_nType == WIDGET_LOOP_FLIPBOOK && pSounds->m_nSoundAtMark > -1 && m_nFrame == pSounds->m_nMarkFrame)
        {
            g_pAudioMgr->PlaySpecific(pSounds->m_nSoundAtMark);
        }
    }

    if (nLimit == m_nFrameIdle && m_nType == WIDGET_FLIPBOOK && m_nFrameCount > 2 && pSounds->m_nSoundBeforeTarget > -1 && m_nFrame == m_nTargetFrame - 1)
    {
        g_pAudioMgr->PlaySpecific(pSounds->m_nSoundBeforeTarget);
    }

    const int nFrame = m_nFrame;

    if (nFrame < nLimit)
    {
        m_bDirty = 1;
        m_nFrame = nFrame + 1;

        return 1;
    }

    if (nFrame > nLimit)
    {
        m_bDirty = 1;
        m_nFrame = nFrame - 1;

        return 1;
    }

    return 0;
}

// Drawing clears the dirty flag whether or not there was a frame.

// FUNCTION: HYPERX 0x0042b450
void Widget::Draw(int)
{
    if (m_bHidden == 1)
    {
        return;
    }

    if (m_bDirty == 0)
    {
        return;
    }

    WidgetDrawFrame* pFrame = m_pFrames;

    if (pFrame != 0)
    {
        pFrame += m_nFrame;

        GDI::Blit(pFrame->nOffsetX + m_nX, pFrame->nOffsetY + m_nY, pFrame->nWidth, pFrame->nHeight, pFrame->hBitmap, pFrame->nSourceX, pFrame->nSourceY);
    }

    m_bDirty = 0;

    if (m_bHighlighted != 1 && m_nType != WIDGET_DISPLAY && m_nType != WIDGET_TEXT_INPUT)
    {
        return;
    }

    auto color = (std::uint32_t)0xffff;

    if (m_bHighlighted != 0)
    {
        color = 0xffffff;
    }

    if (m_bDepressed != 0)
    {
        color = 0xff;
    }

    if (m_pszText != 0)
    {
        GDI::DrawTransparentText(color, m_nTextX + m_nX, m_nTextY + m_nY, m_pszText, (int)strlen(m_pszText), m_nTextFlag);
    }
}

// FUNCTION: HYPERX 0x0042b530
void Widget::SetHighlighted(int nHighlighted)
{
    m_bHighlighted = nHighlighted;
}

// FUNCTION: HYPERX 0x0042b540
int Widget::GetHighlighted() const
{
    return m_bHighlighted;
}

// FUNCTION: HYPERX 0x0042b550
void Widget::SetDepressed(int nDepressed)
{
    m_bDepressed = nDepressed;
}

// FUNCTION: HYPERX 0x0042b560
int Widget::GetDepressed() const
{
    return m_bDepressed;
}

// FUNCTION: HYPERX 0x0042b570
void Widget::SetTargetFrame(int nTargetFrame)
{
    m_nTargetFrame = nTargetFrame;
}

// FUNCTION: HYPERX 0x0042b580
void Widget::SetDirtyFlag(int nDirty)
{
    m_bDirty = nDirty;
}

// FUNCTION: HYPERX 0x0042b590
int Widget::GetPositionY() const
{
    return m_nY;
}

// FUNCTION: HYPERX 0x0042b5a0
void Widget::SetHidden(int nHidden)
{
    m_bHidden = nHidden;
}

// FUNCTION: HYPERX 0x0042b5b0
int Widget::GetHidden() const
{
    return m_bHidden;
}

// FUNCTION: HYPERX 0x0042b5c0
FramePos Widget::AppendFrameWithPos(GDI::Bitmap* hBitmap, int nWidth, int nHeight, int nOffsetX, int nOffsetY, int nSourceX, int nSourceY)
{
    return AddFrameWithPos(hBitmap, 0, nWidth, nHeight, nOffsetX, nOffsetY, nSourceX, nSourceY);
}

// A null bitmap gives a zero-sized frame rather than being refused.
// FUNCTION: HYPERX 0x0042b610
FramePos Widget::AppendFrame(GDI::Bitmap* hBitmap)
{
    return AddFrameBitmap(hBitmap, 0);
}

// FUNCTION: HYPERX 0x0042b640
FramePos Widget::AddFrameBitmap(GDI::Bitmap* hBitmap, int bCached)
{
    FramePos        pos;
    GDI::BitmapSize bm;

    if (m_pFrames == 0)
    {
        m_pFrames = new WidgetDrawFrame[m_nFrameCount + 1];
    }

    m_pFrames[m_nFrameIndex].hBitmap = hBitmap;
    m_pFrames[m_nFrameIndex].bCached = bCached;

    RELEASE_ASSERT(m_nFrameIndex <= m_nFrameCount);

    if (hBitmap != 0)
    {
        GDI::GetBitmapDimensions(hBitmap, &bm);
        m_pFrames[m_nFrameIndex].nWidth  = bm.bmWidth;
        m_pFrames[m_nFrameIndex].nHeight = bm.bmHeight;
    }
    else
    {
        m_pFrames[m_nFrameIndex].nWidth  = 0;
        m_pFrames[m_nFrameIndex].nHeight = 0;
    }

    m_pFrames[m_nFrameIndex].nOffsetX = 0;
    m_pFrames[m_nFrameIndex].nOffsetY = 0;
    m_pFrames[m_nFrameIndex].nSourceX = 0;
    m_pFrames[m_nFrameIndex].nSourceY = 0;

    pos.x = m_pFrames[m_nFrameIndex].nWidth;
    pos.y = m_pFrames[m_nFrameIndex].nHeight;

    m_nFrameIndex++;

    return pos;
}

// The capacity check goes to a stub, so too many frames write past the end.
// FUNCTION: HYPERX 0x0042b750
FramePos Widget::AddFrameWithPos(GDI::Bitmap* hBitmap, int bCached, int nWidth, int nHeight, int nOffsetX, int nOffsetY, int nSourceX, int nSourceY)
{
    FramePos pos;

    RELEASE_ASSERT(m_nFrameIndex <= m_nFrameCount);

    if (m_pFrames == 0)
    {
        m_pFrames = new WidgetDrawFrame[m_nFrameCount];
    }

    m_pFrames[m_nFrameIndex].hBitmap  = hBitmap;
    m_pFrames[m_nFrameIndex].nWidth   = nWidth;
    m_pFrames[m_nFrameIndex].nHeight  = nHeight;
    m_pFrames[m_nFrameIndex].nOffsetX = nOffsetX;
    m_pFrames[m_nFrameIndex].nOffsetY = nOffsetY;
    m_pFrames[m_nFrameIndex].nSourceX = nSourceX;
    m_pFrames[m_nFrameIndex].nSourceY = nSourceY;
    m_pFrames[m_nFrameIndex].bCached  = bCached;

    m_nFrameIndex++;

    pos.x = nWidth;
    pos.y = nHeight;

    return pos;
}

// FUNCTION: HYPERX 0x0042b830
FramePos Widget::GetSize()
{
    FramePos pos;

    pos.x = m_nWidth;
    pos.y = m_nHeight;

    return pos;
}

// FUNCTION: HYPERX 0x0042b850
void Widget::ClearFrames()
{
    GDI::Bitmap* hOwned = 0;

    if (m_pOwner != 0)
    {
        hOwned = m_pOwner->m_hBackground;
    }

    for (int nIndex = 0; nIndex <= m_nFrameCount; nIndex++)
    {
        GDI::Bitmap* const hBitmap = m_pFrames[nIndex].hBitmap;

        if (hBitmap != 0)
        {
            if (hBitmap == hOwned)
            {
                m_pFrames[nIndex].hBitmap = 0;
            }
            else
            {
                // A later frame sharing this bitmap or the screen's background must not delete it again.
                for (int nScan = nIndex + 1; nScan <= m_nFrameCount; nScan++)
                {
                    GDI::Bitmap* const hLater = m_pFrames[nScan].hBitmap;

                    if (hLater == hBitmap || hLater == hOwned)
                    {
                        m_pFrames[nScan].hBitmap = 0;
                    }
                }
            }
        }
    }

    for (int nDrop = 0; nDrop <= m_nFrameCount; nDrop++)
    {
        GDI::Bitmap* const hBitmap = m_pFrames[nDrop].hBitmap;

        if (hBitmap != 0)
        {
            if (m_pFrames[nDrop].bCached == 0)
            {
                GDI::SafeDeleteBitmap(hBitmap);
            }

            m_pFrames[nDrop].hBitmap = 0;
        }
    }

    m_nFrameIndex  = 0;
    m_bDepressed   = 0;
    m_bHighlighted = 0;
    m_bDirty       = 0;
    m_nFrame       = 0;
}

// FUNCTION: HYPERX 0x0042b940
void Widget::SetCurrentFrame(int nFrame)
{
    m_nFrame = nFrame;
}

// FUNCTION: HYPERX 0x0042b950
void Widget::SetText(const char* pszText)
{
    m_pszText = Str::Copy(m_pszText, pszText);
}

// Both checks go to a stub that discards them.
// FUNCTION: HYPERX 0x0042b9a0
void Widget::InitAfterLoad(void)
{
    if (m_nFrameIndex == 0)
    {
        RELEASE_ASSERT(m_nFrameCount == 0);

        return;
    }

    RELEASE_ASSERT(m_nFrameIndex == m_nFrameCount + 1);
}

// FUNCTION: HYPERX 0x0042b9d0
void Widget::SetTextFlag(int nFlag)
{
    m_nTextFlag = nFlag;
}

// A stub body kept out of its callers' unit.
// FUNCTION: HYPERX 0x0042be70
int Widget::RefuseInput(int, int)
{
    return 0;
}

// Table 0x004b2a68. Slot nought's `return 0` also serves FlipbookWidget's table.
// FUNCTION: HYPERX 0x0042be80
int Widget::OnKeyPress(unsigned int)
{
    return 0;
}

int Widget::OnMouseDown()
{
    std::abort();
}

int Widget::OnMouseUp()
{
    std::abort();
}

int Widget::IsDisplayOnly()
{
    std::abort();
}

// FUNCTION: HYPERX 0x0047a8d0
LayoutState Widget::InitLayoutState(void* pBits, int nWidth, int nHeight, int nPitch, int nPrevWidth, int nPrevHeight, int nHalfWidth, int nHalfHeight)
{
    LayoutState state;

    memset(&state, 0, sizeof(state));

    state.pCamera      = 0;
    state.pBits        = pBits;
    state.nHeight      = nHeight;
    state.flPrevRight  = (float)nPrevWidth - 1.0f;
    state.nByteSize    = nHeight * nPitch;
    state.nPitch       = nPitch;
    state.nWidth       = nWidth;
    state.nHalfWidth   = nHalfWidth;
    state.nPrevWidth   = nPrevWidth;
    state.flPrevBottom = (float)nPrevHeight - 1.0f;
    state.nPrevHeight  = nPrevHeight;
    state.nHalfHeight  = nHalfHeight;

    return state;
}

// FUNCTION: HYPERX 0x0048cbb0
int Widget::GetPositionX() const
{
    return m_nX;
}

// FUNCTION: HYPERX 0x00492b50
int Widget::GetTypeID() const
{
    return m_nType;
}
