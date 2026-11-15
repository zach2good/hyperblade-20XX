#ifndef HYPERBLADE_INERTWIDGET_H
#define HYPERBLADE_INERTWIDGET_H
#include "common/compat.h"
#include "ui/widget.h"

// Slots one and two of 0x004b2abc and 0x004b2b10 share one body, which the linker folds.
class InertWidget : public Widget
{
public:
    InertWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
    : Widget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
    {
    }

    // FUNCTION: HYPERX100 0x00477560
    // FUNCTION: HYPERX 0x0042be70
    virtual int HitTest(int, int)
    {
        return 0;
    }

    // FUNCTION: HYPERX100 0x00477570
    virtual int TryActivate(int, int)
    {
        return 0;
    }
};

#endif
