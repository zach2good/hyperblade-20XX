#ifndef HYPERBLADE_FLIPBOOKWIDGET_H
#define HYPERBLADE_FLIPBOOKWIDGET_H

#include "common/compat.h"

#include "globals.h"

#include "ui/widget.h"

#include "ui/holdingwidget.h"

class Screen;

// VTABLE: HYPERX 0x004b2a84
class FlipbookWidget : public HoldingWidget
{
public:
    FlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundPastFirstLimit, int nSoundBeforeTarget, int nSoundAtMark);

    // Table 0x004b2a84 differs from Widget's only in slots three to five; slot five is a stand-in.
    int OnMouseUp() override; // 0x0042ba50

    int IsDisplayOnly() override
    {
        return 0;
    }

private:
};

#endif
