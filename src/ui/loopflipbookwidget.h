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
    int OnMouseDown() override
    {
        return 0;
    }

    int OnMouseUp() override
    {
        return 0;
    }

    int IsDisplayOnly() override
    {
        return 1;
    }

public:
    LoopFlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nLoopFlag, int nSoundAtMark);

    ~LoopFlipbookWidget()
    {
    }

    // Slot six, in no other table.
    void Draw(int nSlot) override;

private:
    int m_nLoopFlag; // 0x068, zero when the run stops at its last frame
};

#endif
