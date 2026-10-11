#ifndef HYPERBLADE_RADAR_H
#define HYPERBLADE_RADAR_H

#include "graphics/texture.h"
#include "common/staticassert.h"
#include "math/vec3.h"

// A RECT with a destructor, for the constructor's trylevel.
struct RadarRect : public RECT
{
    ~RadarRect()
    {
    }
};

// VTABLE: HYPERX 0x004b30d4

class Radar : public Texture
{
public:
    void ClearState();

    virtual ~Radar();
    virtual void SetPosition(Point pt);
    virtual int  RestoreContents();

    Radar();
    int Init(const char* pszName, const Vec3* pRadii, int nBlitMode);

    void CacheObjectColors();

    // Four of the tracked objects are players and the sixth the rok.
    int Update(short nSlot);
    int DrawObject(int nWhich);
    int BltSprite(const POINT* pOffset, DWORD dwColour);

private:
    class CDDBitMap* m_hBackground; // 0x0c0

    // The followed object's position.
    Vec3   m_vCentre;  // 0x0c4
    float  m_flScaleX; // 0x0d0
    float  m_flScaleY; // 0x0d4
    double m_dSin;     // 0x0d8
    double m_dCos;     // 0x0e0
    POINT  m_ptDial;   // 0x0e8
    // ClearState zeroes these two beside the dial point, and nothing in the image reads them.
    int       m_nUnusedA;          // 0x0f0
    int       m_nUnusedB;          // 0x0f4
    DWORD     m_dwColourGreen;     // 0x0f8
    DWORD     m_dwColourDimYellow; // 0x0fc
    DWORD     m_dwColourYellow;    // 0x100
    DWORD     m_dwColourDimBlue;   // 0x104
    DWORD     m_dwColourBlue;      // 0x108
    DWORD     m_dwColourRok;       // 0x10c
    DWORD     m_dwColourBlack;     // 0x110
    RadarRect m_rcInner;           // 0x114
    RadarRect m_rcOuter;           // 0x124
    int       m_nTeamHome;         // 0x134
    int       m_nTeamAway;         // 0x138
    int       m_nArena;            // 0x13c
};

// SYNTHETIC: HYPERX 0x0043f3d0
// Radar::`scalar deleting destructor'

STATIC_ASSERT(sizeof(Radar) == 0x140);
#endif
