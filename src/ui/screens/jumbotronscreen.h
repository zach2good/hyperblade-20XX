#ifndef HYPERBLADE_JUMBOTRONSCREEN_H
#define HYPERBLADE_JUMBOTRONSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// VTABLE: HYPERX 0x004b4064
class JumbotronScreen : public Screen
{
public:
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
    JumbotronScreen();
    ~JumbotronScreen();
};

static_assert(sizeof(JumbotronScreen) == 0x464);

// SYNTHETIC: HYPERX 0x0048e720
// JumbotronScreen::`scalar deleting destructor'

#endif
