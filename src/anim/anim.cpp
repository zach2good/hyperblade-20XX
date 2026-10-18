// The animation data's packed rotation format: packing Euler triples into it, and unpacking it.

#include "common/compat.h"
#include <cmath>
#include <bit>
#include "engine/cameraprojectionstate.h"
#include "anim/anim.h"
#include "anim/animclip.h"

// A matrix cell as the packer reads it: the float, its bits, and its top half.
union FloatWord
{
    float        fl;
    unsigned int n;

    struct
    {
        unsigned short wLow;
        unsigned short wHigh;
    };
};

// Angles are in degrees; each matrix cell packs to 16 bits, the inverse of UnpackFixed16ToFloat.
// FUNCTION: HYPERX 0x00416ba0
void Anim::EulerToMatrix(const float* pInputEulers, PackedRotation* pOutput, short nFrames, const AnimClipHeader* pHeader, TransformObject* pTempXform)
{
    const float* pFrameEuler = pInputEulers;

    for (short nFrame = 0; nFrame < nFrames; nFrame++)
    {
        pTempXform->Reset(0);

        for (int nAxis = 0; nAxis < 3; nAxis++)
        {
            float flAngle = pFrameEuler[2 - nAxis];

            switch (pHeader->acAxisOrder[2 - nAxis])
            {
                case 'x':
                    flAngle *= 0.017453292f;

                    pTempXform->m_rotX.flCos = (float)cos(static_cast<double>(flAngle));
                    pTempXform->m_rotX.flSin = (float)sin(static_cast<double>(flAngle));

                    pTempXform->RotateXStaged();
                    break;

                case 'y':
                    flAngle *= 0.017453292f;

                    pTempXform->m_rotY.flCos = (float)cos(static_cast<double>(flAngle));
                    pTempXform->m_rotY.flSin = (float)sin(static_cast<double>(flAngle));

                    pTempXform->RotateAroundY();
                    break;

                case 'z':
                    flAngle *= 0.017453292f;

                    pTempXform->m_rotZ.flCos = (float)cos(static_cast<double>(flAngle));
                    pTempXform->m_rotZ.flSin = (float)sin(static_cast<double>(flAngle));

                    pTempXform->ApplyRotationZ();
                    break;
            }
        }

        pFrameEuler += 3;

        PackedRotation* const pRotation = &pOutput[nFrame];

        for (int nRow = 0; nRow < 3; nRow++)
        {
            for (int nCol = 0; nCol < 3; nCol++)
            {
                const float flCell = pTempXform->m_a[nRow][nCol];

                FloatWord cell;

                cell.fl = flCell;

                if (fabs(static_cast<double>(flCell)) < 5e-05)
                {
                    cell.fl = 0.0f;
                }

                const unsigned int nLow = cell.n & 0xfff;

                if (nLow > 0x800)
                {
                    cell.n += 0x1000;
                }

                if ((cell.n & 0x80000000) != 0)
                {
                    cell.n |= 0xf8000000;
                }
                else
                {
                    cell.n &= 0x07fff000;
                }

                cell.n <<= 4;

                pRotation->aw[nRow][nCol] = cell.wHigh;
            }
        }
    }
}

// Turns nine packed 16-bit values into nine floats without a conversion: the low fifteen bits
// become the top of the mantissa under a fixed exponent, and bit fifteen picks the sign. The
// values are a three by three block written into rows of four.
// FUNCTION: HYPERX 0x00416cf0
void Anim::UnpackFixed16ToFloat(const PackedRotation* pIn, float* pOut)
{
    for (int nRow = 0; nRow < 3; nRow++)
    {
        for (int nCol = 0; nCol < 3; nCol++)
        {
            const unsigned int nWord = pIn->aw[nRow][nCol];

            const auto nExponent = [&]
            {
                if ((nWord & 0x8000) != 0)
                {
                    return 0xb8000000u;
                }

                return 0x38000000u;
            }();

            pOut[nRow * 4 + nCol] = std::bit_cast<float>(((nWord & 0x7fff) << 12) | nExponent);
        }
    }
}
