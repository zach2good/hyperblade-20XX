#ifndef HYPERBLADE_D3DVIEWPORT_H
#define HYPERBLADE_D3DVIEWPORT_H

#include "common/compat.h"

struct IDirect3DDevice;
class Texture;

namespace D3DViewport
{

int __cdecl   Setup(struct IDirect3DViewport** ppViewport, IDirect3DDevice* pDevice, int nWidth, int nHeight, int nX, int nY);
int __stdcall Clear(void);
int __cdecl   SetForTexture(unsigned int nViewportId, class Texture* pSource);

} // namespace D3DViewport

#endif
