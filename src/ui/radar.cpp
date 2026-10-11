#include "common/compat.h"
#include "engine/cameraobject.h"
#include "math/vector.h"
#include "globals.h"
#include "uiglobals.h"
#include "radar.h"
#include "graphics/texture.h"
#include "engine/gameengine.h"
#include "graphics/cddbitmap.h"
#include "common/pathstring.h"
#include "engine/devicector.h"
#include "physics/physicsbody.h"
#include "engine/objectclasses.h"
#include "app/gameapp.h"
#include "hud.h"
#include "graphics/scrollbitmap.h"
#include <ddraw.h>
#include <new.h>
#include <math.h>
#include "math/point.h"
#include "common/staticinit.h"
#include "graphics/ddrawapi.h"

float        g_flUnusedRadarA = g_flZeroRadar;
float        g_flUnusedRadarB = g_flZeroRadar;
float        g_flUnusedRadarC = g_flZeroRadar;
SurfacePoint g_ptRadarInset(4, 4);

// FUNCTION: HYPERX 0x0043f3f0
Radar::Radar()
{
    ClearState();
}

// FUNCTION: HYPERX 0x0043f440
void Radar::ClearState()
{
    if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) == 0)
    {
        m_bVisible = 0;
    }

    m_hBackground = 0;
    m_vCentre.x = m_vCentre.y = m_vCentre.z = 0.0f;
    m_flScaleY                              = 0.0f;
    m_flScaleX                              = 0.0f;

    const float flSeed = g_flZeroRadar;

    m_dCos = flSeed;
    m_dSin = flSeed;

    m_ptDial.y = 0;
    m_ptDial.x = 0;
    m_nUnusedB = 0;
    m_nUnusedA = 0;

    SetRect((LPRECT)&m_rcOuter, 0, 0, 0, 0);
    SetRect(&m_rcInner, 0, 0, 0, 0);
}

// The pen and brush are module globals the radar did not allocate but frees.
// FUNCTION: HYPERX 0x0043f4d0
Radar::~Radar()
{
    if (m_hBackground != 0)
    {
        delete m_hBackground;

        m_hBackground = 0;
    }

    if (g_hRadarBrush != 0)
    {
        DeleteObject(g_hRadarBrush);

        g_hRadarBrush = 0;
    }

    if (g_hRadarPen != 0)
    {
        DeleteObject(g_hRadarPen);

        g_hRadarPen = 0;
    }
}

struct IDirectDrawSurface;

// FUNCTION: HYPERX 0x0043f570
void Radar::CacheObjectColors()
{
    m_dwColourGreen = DDraw::ColourToPixel(m_pSurface, 0x00ff00);

    m_dwColourDimYellow = DDraw::ColourToPixel(m_pSurface, 0xc8c800);

    m_dwColourYellow = DDraw::ColourToPixel(m_pSurface, 0xffff00);

    m_dwColourDimBlue = DDraw::ColourToPixel(m_pSurface, 0x0000c8);

    m_dwColourBlue = DDraw::ColourToPixel(m_pSurface, 0x0000ff);

    m_dwColourRok = DDraw::ColourToPixel(m_pSurface, 0x00ffff);

    m_dwColourBlack = DDraw::ColourToPixel(m_pSurface, 0x000000);

    if (g_pGameEngine != 0)
    {
        m_nTeamHome = g_pGameEngine->m_nTeamHomeIndex;
        m_nTeamAway = g_pGameEngine->m_nTeamAwayIndex;
        m_nArena    = g_pGameEngine->m_nArenaIndex;
    }
}

struct IDirectDrawSurface;

#define RADAR_COLOUR_MODE 1
#define RADAR_KEY_COLOUR  0xffffff

// FUNCTION: HYPERX 0x0043f650
int Radar::Init(const char* pszName, const Vec3* pRadii, int nBlitMode)
{
    int nResult = 0;

    m_hBackground = new CDDBitMap;

    if (m_hBackground == 0)
    {
        // STRING: HYPERX 0x004c0d94
        g_gameApp.Error("Cannot create surface for radar bacground art!");
    }
    else
    {
        nResult = m_hBackground->LoadFromFile(pszName, RADAR_COLOUR_MODE, nBlitMode);
    }

    const Point size = m_hBackground->GetSize();

    m_ptDial.x = (size.x >> 1) - (g_ptRadarInset.x >> 1) - 1;
    m_ptDial.y = (size.y >> 1) - (g_ptRadarInset.x >> 1);

    SetRect(&m_rcOuter, m_ptDial.x, m_ptDial.y, g_ptRadarInset.x + m_ptDial.x, g_ptRadarInset.y + m_ptDial.y);

    float flScale = pRadii->x;

    if (flScale <= pRadii->y)
    {
        flScale = pRadii->y;
    }

    m_flScaleX = m_ptDial.x / flScale;
    m_flScaleY = m_ptDial.y / flScale;

    nResult += Texture::AllocSurface(SurfacePoint(size), 1, 1);

    CacheObjectColors();

    m_dwFillColor = DDraw::ColourToPixel(m_pSurface, RADAR_KEY_COLOUR);

    return nResult;
}

// GLOBAL: HYPERX 0x004ce6a0
extern double g_dRadarHeading;
// GLOBAL: HYPERX 0x004ce6a8
extern Vec3 g_vRadarCameraDir;
// GLOBAL: HYPERX 0x004ce6b4
extern class CameraObject* g_pRadarCameraObject;

struct BlitOffset
{
    BlitOffset()
    {
        m_pt.x = 0;
        m_pt.y = 0;
    }

    ~BlitOffset()
    {
    }

    POINT m_pt;
};

// FUNCTION: HYPERX 0x0043f7b0
int Radar::RestoreContents()
{
    int nDrawn = 0;

    if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
    {
        nDrawn = BltFill();
    }

    CDDBitMap* const pSurface = m_hBackground;

    if (pSurface != 0)
    {
        return pSurface->Draw();
    }

    return nDrawn;
}

// FUNCTION: HYPERX 0x0043f7e0
void Radar::SetPosition(Point pt)
{
    m_ptPosition.x = pt.x;
    m_ptPosition.y = pt.y;

    m_hBackground->SetPosition(pt);
}

// The heading lives in globals, so the cosine reloads the angle just stored.
// FUNCTION: HYPERX 0x0043f810
int Radar::Update(short nSlot)
{
    const Vec3* pStart;

    if (g_pGameEngine != 0 && ((unsigned int)m_nTeamHome != (unsigned int)((short)g_pGameEngine->m_nTeamHomeIndex & 0xffff) || (unsigned int)m_nTeamAway != (unsigned int)((short)g_pGameEngine->m_nTeamAwayIndex & 0xffff) || (unsigned int)m_nArena != (unsigned int)((short)g_pGameEngine->m_nArenaIndex & 0xffff)))
    {
        CacheObjectColors();
    }

    RestoreContents();

    pStart = g_pGameEngine->m_ppSlots[nSlot]->GetPositionPtr();

    m_vCentre = *pStart;

    CameraObject* const pCamera = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);

    g_pRadarCameraObject = pCamera;

    g_vRadarCameraDir = pCamera->m_dirOut;

    g_dRadarHeading = atan2(g_vRadarCameraDir.x, g_vRadarCameraDir.y);
    m_dSin          = sin(g_dRadarHeading);
    m_dCos          = cos(g_dRadarHeading);

    if (m_hBackground == 0)
    {
        BlitOffset offset;

        BltSprite(&offset.m_pt, m_dwColourGreen);
    }

    return 1;
}

// Four player cases and the rok, drawn as its carrier's arrowhead when carried.

// GLOBAL: HYPERX 0x004ce6d4
extern DWORD g_dwRadarBlipColour;

// GLOBAL: HYPERX 0x004ce698
extern float g_flRadarDialX;

// GLOBAL: HYPERX 0x004ce69c
extern float g_flRadarDialY;

// GLOBAL: HYPERX 0x004c0d90
extern DWORD g_dwRadarCarrierColour;

#define ARROW_POINTS 3

#define ARROW_TIP  190
#define ARROW_BASE 140
#define ARROW_HALF 15

// FUNCTION: HYPERX 0x0043f960
int Radar::DrawObject(int nWhich)
{
    IDirectDrawSurface* pSurface;
    HGDIOBJ             hOldPen;
    const Vec3*         pflWorld;
    PhysicsBody*        pRok;
    AttachPak*          pnCarrier;
    POINT               apt[ARROW_POINTS];
    short               nCarrier;
    int                 nHalfWidth;
    int                 nHalfHeight;
    const RECT* const   prc = &m_rcOuter;
    double              dRotated;

    HDC hdc = 0;

    switch (nWhich)
    {
        case SLOT_DROMER0:
            g_dwRadarBlipColour = m_dwColourDimYellow;
            break;

        case SLOT_DROMER2:
            g_dwRadarBlipColour = m_dwColourYellow;
            break;

        case SLOT_DROMER1:
            g_dwRadarBlipColour = m_dwColourDimBlue;
            break;

        case SLOT_DROMER3:
            g_dwRadarBlipColour = m_dwColourBlue;
            break;

        case SLOT_ROK:
            pRok = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody();

            pnCarrier = pRok->pAttach;

            if (pnCarrier != 0)
            {
                if (pnCarrier->nSlot < PLAYER_OBJECT_COUNT)
                {
                    nCarrier = pnCarrier->nSlot;

                    if (nCarrier == g_pGameEngine->m_nViewedSlot)
                    {
                        // The DC slot doubles as the arrow flag until GetDC fills it, as in the client.
                        hdc = (HDC)1;
                        if ((nCarrier & 1) != 0)
                        {
                            nWhich = SLOT_NGOAL;
                        }
                        else
                        {
                            nWhich = SLOT_SGOAL;
                        }

                        break;
                    }
                }
            }

            g_dwRadarBlipColour = m_dwColourRok;
            break;

        default:
            return 0;
    }

    pflWorld = g_pGameEngine->m_ppSlots[(short)nWhich]->GetPositionPtr();

    if (hdc != 0)
    {
        nHalfWidth  = (prc->right - prc->left) / 2;
        nHalfHeight = (prc->bottom - prc->top) / 2;
        apt[0].x    = 0;

        if ((nCarrier & 1) != 0)
        {
            apt[0].y = ARROW_TIP;
            apt[1].x = -ARROW_HALF;
            apt[1].y = ARROW_BASE;
            apt[2].x = ARROW_HALF;
            apt[2].y = ARROW_BASE;
        }
        else
        {
            apt[0].y = -ARROW_TIP;
            apt[1].x = ARROW_HALF;
            apt[1].y = -ARROW_BASE;
            apt[2].x = -ARROW_HALF;
            apt[2].y = -ARROW_BASE;
        }

        for (int i = 0; i < ARROW_POINTS; i++)
        {
            g_flRadarDialX = apt[i].x * m_flScaleX;
            g_flRadarDialY = apt[i].y * m_flScaleY;

            apt[i].x = (int)(g_flRadarDialX * m_dCos - g_flRadarDialY * m_dSin) + prc->left + nHalfWidth;

            dRotated = g_flRadarDialY * m_dCos;

            apt[i].y = prc->top - (int)(dRotated + g_flRadarDialX * m_dSin) + nHalfHeight;
        }
    }
    else
    {
        g_flRadarDialX = (pflWorld->x - m_vCentre.x) * m_flScaleX;
        g_flRadarDialY = (pflWorld->y - m_vCentre.y) * m_flScaleY;

        m_ptDial.x = (int)(g_flRadarDialX * m_dCos - g_flRadarDialY * m_dSin);

        dRotated = g_flRadarDialY * m_dCos;

        m_ptDial.y = -(int)(dRotated + g_flRadarDialX * m_dSin);
    }

    if (hdc == (HDC)1)
    {
        pSurface = m_pSurface;

        if (pSurface != 0 && pSurface->GetDC(&hdc) == 0)
        {
            if (g_hRadarBrush == 0)
            {
                static LOGBRUSH s_logBrush = { BS_SOLID, g_dwRadarCarrierColour, 0 };

                g_hRadarBrush = CreateBrushIndirect(&s_logBrush);
            }

            if (g_hRadarPen == 0)
            {
                g_hRadarPen = CreatePen(PS_SOLID, 0, g_dwRadarCarrierColour);
            }

            hOldPen           = SelectObject(hdc, g_hRadarPen);
            HGDIOBJ hOldBrush = SelectObject(hdc, g_hRadarBrush);

            Polygon(hdc, apt, ARROW_POINTS);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);

            m_pSurface->ReleaseDC(hdc);
        }
    }
    else if (BltSprite(&m_ptDial, g_dwRadarBlipColour) == 0)
    {
        return 0;
    }

    if (nWhich == SLOT_ROK)
    {
        nHalfWidth  = (prc->right - prc->left) / 2;
        nHalfHeight = (prc->bottom - prc->top) / 2;

        pSurface = m_pSurface;

        if (pSurface != 0 && pSurface->GetDC(&hdc) == 0)
        {
            HGDIOBJ hLinePen = CreatePen(PS_SOLID, 0, 0xffffef);

            MoveToEx(hdc, prc->left + nHalfWidth, m_rcOuter.top + nHalfHeight, 0);

            hOldPen = SelectObject(hdc, hLinePen);

            LineTo(hdc, m_ptDial.x + prc->left + nHalfWidth, m_ptDial.y + m_rcOuter.top + nHalfHeight);

            DeleteObject(SelectObject(hdc, hOldPen));

            m_pSurface->ReleaseDC(hdc);
        }
    }

    return 1;
}

// A lost surface still answers zero.
// FUNCTION: HYPERX 0x0043fdd0
int Radar::BltSprite(const POINT* pOffset, DWORD dwColour)
{
    RECT rcScreen;

    LPRECT lprc = &m_rcInner;

    *lprc = m_rcOuter;

    OffsetRect(lprc, pOffset->x, pOffset->y);

    // The windef macros evaluate the inner maximum twice, as the client does.
    m_rcInner.left   = min(max(m_rcInner.left, 0), m_ptSize.x - 4);
    m_rcInner.top    = min(max(m_rcInner.top, 0), m_ptSize.y - 4);
    m_rcInner.right  = min(max(m_rcInner.right, 4), m_ptSize.x);
    m_rcInner.bottom = min(max(m_rcInner.bottom, 4), m_ptSize.y);

    m_bltfx.dwFillColor = dwColour;

    if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
    {
        m_hr = m_pSurface->Blt(lprc, 0, lprc, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }
    else
    {
        SetRect(&rcScreen,
                m_ptPosition.x + m_rcInner.left - 1,
                m_ptPosition.y + m_rcInner.top - 1,
                m_ptPosition.x + m_rcInner.right - 1,
                m_ptPosition.y + m_rcInner.bottom - 1);

        m_hr = g_pSurfaceBack->Blt(&rcScreen, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }

    if (m_hr < 0)
    {
        if (m_hr == DDERR_SURFACELOST)
        {
            RestoreContents();
        }

        if (m_hr != 0)
        {
            return 0;
        }
    }

    return 1;
}

DWORD g_dwRadarCarrierColour = 0x18bd9c;
