#ifndef HYPERBLADE_TEXTINPUTWIDGET_H
#define HYPERBLADE_TEXTINPUTWIDGET_H

#include "widget.h"
#include "ui/holdingwidget.h"

// Slot zero of 0x004b2af4; no other table holds 0x0042bda0.
// VTABLE: HYPERX 0x004b2af4
class TextInputWidget : public HoldingWidget
{
public:
    // Slots four and five hold KeyConfigDialog::SaveKeysAs, so both keep a stand-in.
    int OnMouseUp() override
    {
        return 0;
    }

    int IsDisplayOnly() override
    {
        return 0;
    }

public:
    TextInputWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark);

    ~TextInputWidget()
    {
    }

    int OnKeyPress(unsigned int nKey) override;

private:
    int   m_nLength;   // 0x068
    int   m_nCapacity; // 0x06c
    char* m_pszInput;  // 0x070
};

#endif
