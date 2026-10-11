#ifndef HYPERBLADE_LOOPFLIPBOOKWIDGET_H
#define HYPERBLADE_LOOPFLIPBOOKWIDGET_H

#include "common/compat.h"
#include "ui/widget.h"

#include "ui/holdingwidget.h"

#include "ui/buttonwidget.h"
#include "globals.h"

class Screen;

// Table 0x004b2abc; its slot six, 0x0042bb20, steps the frame. FlipbookWidget never steps.
// VTABLE: HYPERX 0x004b2abc
class LoopFlipbookWidget : public InertWidget
{
public:
    // Three shared addresses in slots three to five, three stand-ins.
    virtual int OnMouseDown()
    {
        return 0;
    }

    virtual int OnMouseUp()
    {
        return 0;
    }

    virtual int IsDisplayOnly()
    {
        return 1;
    }

public:
    LoopFlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nLoopFlag, int nSoundAtMark);

    ~LoopFlipbookWidget()
    {
    }

    // Slot six, in no other table.
    virtual void Draw(HDC hdc, HPALETTE hPalette, int nSlot);

private:
    int m_nLoopFlag; // 0x068, zero when the run stops at its last frame
};

#endif
