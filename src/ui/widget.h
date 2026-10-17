#ifndef HYPERBLADE_WIDGET_H
#define HYPERBLADE_WIDGET_H

#include "common/compat.h"
#include "graphics/gdi.h"
#include "globals.h"

class Screen;

#include "framepos.h"
#include <string.h>

struct FramePos;

// A flipbook widget sends this to its owner when its frames run out; the rest are INPUT_ events.
#define WIDGET_MSG_LOOP_DONE 0x66

// What Widget::m_nType holds; the four button kinds follow ButtonWidget's constructor argument.
enum WidgetKind
{
    WIDGET_TEXT_INPUT,
    WIDGET_FLIPBOOK,
    WIDGET_BUTTON_0,
    WIDGET_BUTTON_1,
    WIDGET_BUTTON_2,
    WIDGET_BUTTON_3,
    WIDGET_LOOP_FLIPBOOK,
    WIDGET_DISPLAY
};

namespace Str
{

inline char* Copy(char*& pText, const char* pszSource)
{
    return pText = strcpy((char*)operator new(strlen(pszSource) + 1), pszSource);
}

inline char* Duplicate(char*& pText, const char* pszSource)
{
    pText = (char*)operator new(strlen(pszSource) + 1);

    return strcpy(pText, pszSource);
}

} // namespace Str

namespace Mem
{

template <class T>
inline void SafeDelete(T*& pBlock)
{
    if (pBlock != 0)
    {
        operator delete(pBlock);

        pBlock = 0;
    }
}

} // namespace Mem

// One entry of a widget's frame table. Thirty two bytes, indexed by the frame counter.
struct WidgetDrawFrame
{
    GDI::Bitmap* hBitmap;  // 0x00
    int          nOffsetX; // 0x04
    int          nOffsetY; // 0x08
    int          nSourceX; // 0x0c
    int          nSourceY; // 0x10
    int          nWidth;   // 0x14
    int          nHeight;  // 0x18
    int          bCached;  // 0x1c, the bitmap cache owns hBitmap
};

// All four are guarded against minus one before use.
struct FlipbookSounds
{
    // Played when the kind is six and the frame reaches the mark below.
    int m_nSoundAtMark; // 0x058
    // Played when the kind is one and the frame is one past m_nFrameIdle.
    int m_nSoundPastFirstLimit; // 0x05c
    // Played when the kind is one and the frame is one short of m_nTargetFrame.
    int m_nSoundBeforeTarget; // 0x060
    int m_nMarkFrame;         // 0x064
};

struct LayoutState;

// VTABLE: HYPERX 0x004b2a68
class Widget
{
public:
    // Free functions in the original.
    static LayoutState InitLayoutState(void* pBits, int nWidth, int nHeight, int nPitch, int nPrevWidth, int nPrevHeight, int nHalfWidth, int nHalfHeight);
    static int         RefuseInput(int nX, int nY);

public:
    int  GetTypeID() const;
    void SetHighlighted(int nValue);
    void SetDepressed(int nValue);
    void SetDirtyFlag(int nValue);
    void SetHidden(int nHidden);
    void SetCurrentFrame(int nValue);

    void     ClearFrames();
    FramePos AppendFrame(GDI::Bitmap* hBitmap);
    FramePos AddFrameBitmap(GDI::Bitmap* hBitmap, int bCached);
    FramePos AddFrameWithPos(GDI::Bitmap* hBitmap, int bCached, int nWidth, int nHeight, int nOffsetX, int nOffsetY, int nSourceX, int nSourceY);
    FramePos AppendFrameWithPos(GDI::Bitmap* hBitmap, int nWidth, int nHeight, int nOffsetX, int nOffsetY, int nSourceX, int nSourceY);
    void     InitAfterLoad();

    FramePos GetSize();

    // Table 0x004b2a68 has seven slots, three pure; slot nought is a key press, not a destructor.
    virtual int OnKeyPress(unsigned int nKey); // 0x0042be80
    virtual int HitTest(int nX, int nY);
    // Also read as the mouse move.
    virtual int TryActivate(int nX, int nY); // 0x0042b330
    // Pure in the original's own table. Enter on an activated widget runs slot four too.
    virtual int OnMouseDown() = 0;
    virtual int OnMouseUp()   = 0;
    // Nonzero on the display and looping flipbook widgets, which the focus walks skip.
    virtual int IsDisplayOnly() = 0;

    // Unused names for HoldingWidget::OnMouseDown and FlipbookWidget::OnMouseUp.
    int          BeginWithoutHold();
    int          BeginWithHold();
    virtual void Draw(int nSlot);
    void         SetText(const char* pszText);
    void         SetTargetFrame(int nValue);
    void         SetTextFlag(int nFlag);

    Widget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark);
    ~Widget();

public:
    // The constructor fills all 0x68 bytes.
    int   m_nType;       // 0x004
    int   m_nX;          // 0x008
    int   m_nY;          // 0x00c
    char* m_pszText;     // 0x010
    int   m_nTextX;      // 0x014
    int   m_nTextY;      // 0x018
    int   m_nWidth;      // 0x01c
    int   m_nHeight;     // 0x020
    int   m_nFrameCount; // 0x024
    int   m_bHidden;     // 0x028
    // Marks the one widget the screen has highlighted.
    int m_bHighlighted; // 0x02c
    // Overrides the highlight.
    int              m_bDepressed; // 0x030
    WidgetDrawFrame* m_pFrames;    // 0x034
    int              m_nFrame;     // 0x038
    // Idle, highlighted and depressed each walk toward their own frame.
    int m_nFrameIdle;        // 0x03c
    int m_nFrameHighlighted; // 0x040
    int m_nTargetFrame;      // 0x044
    int m_bDirty;            // 0x048

    int     m_nFrameIndex; // 0x04c
    Screen* m_pOwner;      // 0x050
    // Handed to GDI::DrawTransparentText, which ignores it; only the jumbotron scores set it.
    int m_nTextFlag; // 0x054

    FlipbookSounds m_flipbook; // 0x058

    int GetHighlighted() const;

    int GetDepressed() const;

    int GetHidden() const;

    int GetPositionY() const;

    int GetPositionX() const;

    int AdvanceFrame();
};

#endif
