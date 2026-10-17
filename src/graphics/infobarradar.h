// The info bar's radar: it follows the viewed object and plots the other players and the rok round it.

#ifndef HYPERBLADE_INFOBARRADAR_H
#define HYPERBLADE_INFOBARRADAR_H

#include "math/vector.h"
#include "graphics/cdddrawable.h"

// VTABLE: HYPERX 0x004b32dc
class InfoBarRadar : public CDDDrawable
{
public:
    InfoBarRadar();
    ~InfoBarRadar() throw() override;

    int Draw() override;

    int  TrackSlot(int nSlot);
    void DrawMarker(int nUnused);
    void DrawBlip(int nSlot, int bTrail);

    void ComputeScale(const class CDDDrawable* pFrame, Vec3 vLimits);

public:
    float m_flScaleX; // 0x01c
    float m_flScaleY; // 0x020
    // The constructor zeroes it, and nothing in the image reads it.
    int   m_nUnused;   // 0x024
    int   m_nSlot;     // 0x028
    Vec3  m_vPosition; // 0x02c
    float m_flSin;     // 0x038
    float m_flCos;     // 0x03c
};

// SYNTHETIC: HYPERX 0x0044fd40
// InfoBarRadar::`scalar deleting destructor'

#endif
