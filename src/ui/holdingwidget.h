#ifndef HYPERBLADE_HOLDINGWIDGET_H
#define HYPERBLADE_HOLDINGWIDGET_H

#include "common/compat.h"
#include "ui/widget.h"

class Screen;

// Slot three in four tables: 0x0042bd90 sets the hold word and marks the widget dirty.
class HoldingWidget : public Widget
{
public:
    HoldingWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
    : Widget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
    {
    }

    int OnMouseDown() override;
};

#endif
