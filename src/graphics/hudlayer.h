// The in-match HUD's own 8-bit layer, a whole fraction of the back buffer, scaled up over the 3D.

#ifndef HYPERBLADE_HUDLAYER_H
#define HYPERBLADE_HUDLAYER_H

#include "common/compat.h"

struct ViewExtent;

namespace HudLayer
{

// The index the layer is cleared to; an RLE sprite never draws it.
constexpr auto KEY_INDEX = 255;

// How many back-buffer pixels one layer pixel covers on each axis.
int Scale();

// Points the back-buffer globals at the layer, or puts them back.
void Enter();
void Leave();
bool IsEntered();

// Enters for its lifetime, unless the layer is already entered.
class Scope
{
public:
    Scope()
    : m_bEntered(IsEntered() == false)
    {
        if (m_bEntered)
        {
            Enter();
        }
    }

    ~Scope()
    {
        if (m_bEntered)
        {
            Leave();
        }
    }

    Scope(const Scope&)            = delete;
    Scope& operator=(const Scope&) = delete;

private:
    bool m_bEntered;
};

// Clears the layer for a new frame's HUD.
void Clear();

struct Layer
{
    const unsigned char* pBits;
    int                  nPitch;
    int                  nWidth;
    int                  nHeight;
    int                  nScale;
    int                  nOriginX;
    int                  nOriginY;
};

const Layer& Pixels();

// Whether a HUD was drawn since the last call, so a frame presents it once.
bool TakeFrame();

// A layer position as the back buffer's, for what the 3D draws under the HUD.
ViewExtent ToBackBuffer(ViewExtent extent);

} // namespace HudLayer

#endif
