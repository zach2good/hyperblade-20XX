// The animation data's packed rotation format: packing Euler triples into it, and unpacking it.

#include "common/compat.h"
#include <math.h>
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
void __cdecl Anim::EulerToMatrix(const float* pInputEulers, PackedRotation* pOutput, short nFrames, const AnimClipHeader* pHeader, TransformObject* pTempXform)
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

                    pTempXform->m_rotX.flCos = (float)cos(flAngle);
                    pTempXform->m_rotX.flSin = (float)sin(flAngle);

                    pTempXform->RotateXStaged();
                    break;

                case 'y':
                    flAngle *= 0.017453292f;

                    pTempXform->m_rotY.flCos = (float)cos(flAngle);
                    pTempXform->m_rotY.flSin = (float)sin(flAngle);

                    pTempXform->RotateAroundY();
                    break;

                case 'z':
                    flAngle *= 0.017453292f;

                    pTempXform->m_rotZ.flCos = (float)cos(flAngle);
                    pTempXform->m_rotZ.flSin = (float)sin(flAngle);

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

                if (fabs(flCell) < 5e-05)
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

// Hand-written assembly in the original: each word is loaded with MOV AX, which leaves the top
// half of EAX as it was, and its sign is read by a branch on the carry of SHL EAX, 17. VC6 emits
// neither at /Ox, and the function keeps a frame pointer only an __asm block gives it.
// What it does: turns nine packed 16-bit values into nine floats without a conversion
// instruction. The low fifteen bits become the top of the mantissa under a fixed exponent, and
// bit fifteen picks the sign. The values are a three by three block written into rows of four.
// Why assembly: C cannot load a word into AX without clearing the rest of EAX, nor branch on the
// bit a shift leaves in the carry.
// FUNCTION: HYPERX 0x00416cf0
void __cdecl Anim::UnpackFixed16ToFloat(const PackedRotation* pIn, float* pOut)
{
    __asm
    {
        mov edi, pOut
        mov esi, pIn

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative1
        shr eax, 5
        or  eax, 0x38000000
        jmp store1
negative1:
        shr eax, 5
        or  eax, 0xb8000000
store1:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 4

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative2
        shr eax, 5
        or  eax, 0x38000000
        jmp store2
negative2:
        shr eax, 5
        or  eax, 0xb8000000
store2:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 4

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative3
        shr eax, 5
        or  eax, 0x38000000
        jmp store3
negative3:
        shr eax, 5
        or  eax, 0xb8000000
store3:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 8

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative4
        shr eax, 5
        or  eax, 0x38000000
        jmp store4
negative4:
        shr eax, 5
        or  eax, 0xb8000000
store4:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 4

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative5
        shr eax, 5
        or  eax, 0x38000000
        jmp store5
negative5:
        shr eax, 5
        or  eax, 0xb8000000
store5:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 4

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative6
        shr eax, 5
        or  eax, 0x38000000
        jmp store6
negative6:
        shr eax, 5
        or  eax, 0xb8000000
store6:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 8

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative7
        shr eax, 5
        or  eax, 0x38000000
        jmp store7
negative7:
        shr eax, 5
        or  eax, 0xb8000000
store7:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 4

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative8
        shr eax, 5
        or  eax, 0x38000000
        jmp store8
negative8:
        shr eax, 5
        or  eax, 0xb8000000
store8:
        mov dword ptr [edi], eax
        add esi, 2
        add edi, 4

        mov ax, word ptr [esi]
        shl eax, 17
        jc  negative9
        shr eax, 5
        or  eax, 0x38000000
        jmp store9
negative9:
        shr eax, 5
        or  eax, 0xb8000000
store9:
        mov dword ptr [edi], eax
    }
}
