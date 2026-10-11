#ifndef HYPERBLADE_OBJECTSTATS_H
#define HYPERBLADE_OBJECTSTATS_H

#include "common/staticassert.h"

// A player's ratings block, at 0xd20 on a PlayerObject.
class ObjectStats
{
public:
    ~ObjectStats()
    {
    }

    // The stat at the index, scaled by the object's own weight.
    float GetWeightedStat(int nIndex);

    float GetScalar(int nWhich) const;

    void SetStat(int nSlot, float flValue);

    float GetSpeedRatio() const
    {
        return m_flSpeedRatio;
    }

    // Skaters only.
    void ScaleTo(int nSlot);

    // Restores the selected ratings from m_aBackup, sixty three floats further on.
    void CopyBaseStats();

    ObjectStats();

    // m_aStats[k] holds rating k - 1; m_aStats[0] is the energy that scales every weighted read. Ids are RatingMeasureID.
    float m_aStats[64]; // 0x000
    // The pre-scale copy ScaleTo takes of ids one to sixty three.
    float m_aBackup[63];  // 0x100
    float m_flSpeedRatio; // 0x1fc
    float m_flScale;      // 0x200

    ObjectStats* InitDefaults();

    void UpdateMomentumAndEnergy();
};

// playerobject.h puts m_matchStats straight after this block.
STATIC_ASSERT(sizeof(ObjectStats) == 0x204);

inline ObjectStats::ObjectStats()
{
    this->ObjectStats::InitDefaults();
}

inline void ObjectStats::SetStat(int nSlot, float flValue)
{
    m_aStats[nSlot] = flValue;
}

#endif
