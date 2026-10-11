// The two team comparisons choose opposite ways round for the banner and the kiosk.

#ifndef HYPERBLADE_DROMEEXTERIORSCREEN_H
#define HYPERBLADE_DROMEEXTERIORSCREEN_H

#include "common/staticassert.h"
#include "ui/pausemenu.h"

// GLOBAL: HYPERX 0x004c1fd0
extern int g_nDromeTeamHome;
// GLOBAL: HYPERX 0x004c1fd4
extern int g_nDromeTeamAway;
// GLOBAL: HYPERX 0x004c1fd8
extern int g_nDromeArena;

// GLOBAL: HYPERX 0x004c85bc
extern const char* g_pszKioskWhite;
// GLOBAL: HYPERX 0x004c85c0
extern const char* g_pszKioskBlue;
// GLOBAL: HYPERX 0x004c85c4
extern const char* g_pszKioskGreen;

// GLOBAL: HYPERX 0x004c85c8
extern const char* g_apszVersusBanner[14];
// GLOBAL: HYPERX 0x004c8600
extern const char* g_apszMatchBanner[13];

// VTABLE: HYPERX 0x004b4070
class DromeExteriorScreen : public Screen
{
public:
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
    DromeExteriorScreen();
    ~DromeExteriorScreen();
};

STATIC_ASSERT(sizeof(DromeExteriorScreen) == 0x464);

// SYNTHETIC: HYPERX 0x0048ef80
// DromeExteriorScreen::`scalar deleting destructor'

#endif
