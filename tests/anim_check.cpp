// A frame of NaN angles packs as the frame before it, or as zero angles when it comes first, so
// every compiler packs the same rotation.

#include "anim/anim.h"
#include "anim/animclip.h"
#include "math/transformobject.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

// globals.cpp defines these for the game; it brings the whole game with it.
extern const float  g_flTransformZero   = 0.0f;
extern const float  g_flTransformOne    = 1.0f;
extern const double g_dNormalizeEpsilon = 1e-07;
extern const float  g_flZero            = 0.0f;
extern const float  g_flRandRangeBias   = 0.4999f;

static int s_nFailed = 0;

static void Expect(bool bOK, const std::string& strWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", strWhat.c_str());
        s_nFailed++;
    }
}

static bool Same(const PackedRotation& a, const PackedRotation& b)
{
    return std::memcmp(&a, &b, sizeof(PackedRotation)) == 0;
}

int main()
{
    constexpr auto flNaN = std::numeric_limits<float>::quiet_NaN();

    auto header = AnimClipHeader{};
    std::memcpy(header.acAxisOrder, "zyx", 3);

    auto xform = TransformObject();

    const float    aflHeld[] = { 10.0f, 20.0f, 30.0f, flNaN, 20.0f, -flNaN, 5.0f, 6.0f, 7.0f };
    PackedRotation aHeld[3];
    Anim::EulerToMatrix(aflHeld, aHeld, 3, &header, &xform);

    Expect(Same(aHeld[1], aHeld[0]), "a NaN frame packs as the frame before it");
    Expect(!Same(aHeld[2], aHeld[0]), "a later frame keeps its own angles");

    const float    aflFirst[] = { flNaN, -flNaN, flNaN };
    const float    aflZero[]  = { 0.0f, 0.0f, 0.0f };
    PackedRotation first;
    PackedRotation zero;
    Anim::EulerToMatrix(aflFirst, &first, 1, &header, &xform);
    Anim::EulerToMatrix(aflZero, &zero, 1, &header, &xform);

    Expect(Same(first, zero), "a NaN first frame packs as zero angles");

    if (s_nFailed == 0)
    {
        std::printf("anim: all passed\n");
    }

    return static_cast<int>(s_nFailed != 0);
}
