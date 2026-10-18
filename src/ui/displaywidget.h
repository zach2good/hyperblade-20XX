#ifndef HYPERBLADE_DISPLAYWIDGET_H
#define HYPERBLADE_DISPLAYWIDGET_H

#include "ui/signflipbookwidget.h"

#include "globals.h"
#include "ui/loopflipbookwidget.h"

#include "ui/widget.h"

#include "ui/holdingwidget.h"
#include "ui/flipbookwidget.h"

#include "common/compat.h"
#include "ui/buttonwidget.h"

class Screen;

// Table 0x004b2b10; every slot it changes from Widget's is a shared body.
// VTABLE: HYPERX 0x004b2b10
class DisplayWidget : public InertWidget
{
public:
    // Stand-ins for shared stubs whose addresses several tables hold, so none carries a marker.
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
    DisplayWidget(int nX, int nY, int nHighlighted, int nDepressed, Screen* pOwner, int nSoundAtMark);

    ~DisplayWidget() override = default;

private:
};

#endif
