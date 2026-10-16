#ifndef HYPERBLADE_GRAPHICS_H
#define HYPERBLADE_GRAPHICS_H

#include "common/compat.h"

namespace Graphics
{

unsigned int LoadMdmTexDLL(void);
void         UnloadMdmTexDLL(void);
void         Initialize(int bResize);

} // namespace Graphics

#endif
