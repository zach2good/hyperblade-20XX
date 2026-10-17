// Puts finished frames on screen, on the game's own window. The renderer runs on a GPU device
// when one can be made, which the hardware renderer then shares, and on plain SDL otherwise.

#ifndef HYPERBLADE_PRESENTER_H
#define HYPERBLADE_PRESENTER_H

#include "common/compat.h"
#include "graphics/bitmapformat.h"

namespace Presenter
{

// The software back buffer, with the HUD layer over it when the frame drew one.
void Frame(const unsigned char* pBits, int nPitch, int nWidth, int nHeight, const RgbQuad* pColours);

// A 32-bit top-down canvas, fitted to the window keeping its shape.
void Canvas(const void* pBits, int nPitch, int nWidth, int nHeight);

// Whether the renderer runs on a GPU device, so the hardware renderer can draw.
bool HasGPU();

// True while a frame is inside SDL's renderer, which can deliver window messages from its waits.
bool Busy();

} // namespace Presenter

#endif
