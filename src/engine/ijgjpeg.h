// IJG libjpeg 6a's declarations as djpeg sees them; linked from third_party/jpeg-6a.

#ifndef HYPERBLADE_IJGJPEG_H
#define HYPERBLADE_IJGJPEG_H

#include "common/compat.h"

#define XMD_H

// jmorecfg.h defines FAR again, empty, as windef.h's far already is.
#undef FAR

extern "C"
{
#include "cdjpeg.h"
}

#endif
