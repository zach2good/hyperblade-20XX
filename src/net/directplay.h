#ifndef HYPERBLADE_DIRECTPLAY_H
#define HYPERBLADE_DIRECTPLAY_H

#include "common/compat.h"
#include <windows.h>
#include <dplay.h>

namespace DirectPlay
{

char* __cdecl GetErrorString(int hr);

}

#endif
