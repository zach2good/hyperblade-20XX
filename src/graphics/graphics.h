#ifndef HYPERBLADE_GRAPHICS_H
#define HYPERBLADE_GRAPHICS_H

#include "common/compat.h"

namespace Graphics
{

unsigned int LoadMdmTexDLL();
void         UnloadMdmTexDLL();
void         Initialize(int bResize);
void         FreeEdgeStacks();

} // namespace Graphics

#endif
