#ifndef HYPERBLADE_BUTTONWIDGET_H
#define HYPERBLADE_BUTTONWIDGET_H

#include "common/compat.h"
#include "globals.h"

class Screen;

#include "framepos.h"
#include "widget.h"
#include "ui/screen.h"
#include "graphics/gdi.h"

// Table 0x004b2ad8; slot four holds a body no other table has.
// VTABLE: HYPERX 0x004b2ad8
class ButtonWidget : public Widget
{
public:
    // FlipbookWidget::OnMouseDown's body; v1.0.0.1's link folds the two, v1.0.0.0 keeps both.
    virtual int OnMouseDown();

    // Reached only through this table.
    virtual int OnMouseUp();

    // Slot five is KeyConfigDialog::SaveKeysAs, shared by many tables, so this is a stand-in.
    // FUNCTION: HYPERX100 0x004777a0
    virtual int IsDisplayOnly()
    {
        return 0;
    }

    ButtonWidget(int nFlags, int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark);

    ~ButtonWidget()
    {
    }

private:
};

#include "ui/inertwidget.h"

#endif
