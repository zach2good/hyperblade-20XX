#ifndef HYPERBLADE_OBJECTDEF_H
#define HYPERBLADE_OBJECTDEF_H

#include "math/vector.h"

// What m_nRatingsMode records: a skater's sixty three ratings or a goalie's sixteen.
#define RATINGS_FIELD  1
#define RATINGS_GOALIE 2

// The fourth ratings table is the goalie set SetSpecialGoalieRatings writes.
#define RATINGS_SPECIAL_GOALIE 3

// The azimuth is in degrees.
class ObjectDef
{
public:
    explicit ObjectDef(const char* pszModel);

    char* m_pszName; // 0x00
    int   m_nType;   // 0x04
    int   m_nTypeId; // 0x08
    // The model name; compared against "null".
    char* m_pszModel;    // 0x0c
    char* m_pszMaterial; // 0x10
    // Cmd_DefineObject clears it as its last act and Cmd_SetRatings writes the mode into it.
    int              m_nRatingsMode;   // 0x14
    char             m_bOwnPalette;    // 0x18
    char             m_cUnused;        // 0x19
    short            m_wUnused;        // 0x1a
    class BubbleCmd* m_pCollisionList; // 0x1c
    char**           m_apszPlanNames;  // 0x20
    short            m_nPlanCount;     // 0x24
    Vec3             m_vPos;           // 0x28
    float            m_flAccel;        // 0x34
    float            m_flSpeed;        // 0x38
    float            m_flAzimuth;      // 0x3c
    float            m_flElevation;    // 0x40
    float            m_flFriction;     // 0x44
    float            m_flElasticity;   // 0x48
    // Forces the palette remap on where the object's own entry bit would leave it off.
    int m_bForceRemap; // 0x4c
    // One ratings table per difficulty level, then the special goalie set at RATINGS_SPECIAL_GOALIE.
    float* m_apRatings[4]; // 0x50

    char* GetModel() const
    {
        return m_pszModel;
    }

    ~ObjectDef();
};

#endif
