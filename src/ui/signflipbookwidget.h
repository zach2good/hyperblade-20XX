#ifndef HYPERBLADE_SIGNFLIPBOOKWIDGET_H
#define HYPERBLADE_SIGNFLIPBOOKWIDGET_H

#include "common/compat.h"
#include "ui/flipbookwidget.h"

class Screen;

// Table 0x004b2aa0 differs from FlipbookWidget's only in slot five: a sign is display only.
// VTABLE: HYPERX 0x004b2aa0
class SignFlipbookWidget : public FlipbookWidget
{
public:
    SignFlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark);

    ~SignFlipbookWidget() = default;

    // Slot five holds KeyConfigDialog::ClearKeys, one address several tables share.
    int IsDisplayOnly() override
    {
        return 1;
    }
};

#endif
