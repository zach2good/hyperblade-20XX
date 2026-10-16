// Puts finished frames on screen through the GPU renderer, on the game's own window.

#ifndef HYPERBLADE_PRESENTER_H
#define HYPERBLADE_PRESENTER_H

#include "common/compat.h"

namespace Presenter
{

// The software back buffer, with the HUD layer over it when the frame drew one.
void Frame(const unsigned char* pBits, int nPitch, int nWidth, int nHeight, const RGBQUAD* pColours);

// A 32-bit top-down canvas, fitted to the window keeping its shape.
void Canvas(const void* pBits, int nPitch, int nWidth, int nHeight);

} // namespace Presenter

#endif
