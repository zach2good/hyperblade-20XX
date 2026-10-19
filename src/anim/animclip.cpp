#include "common/compat.h"
#include "animclip.h"
#include "common/fileobj.h"
#include "globals.h"
#include "math/transformobject.h"
#include "math/vector.h"
#include "ui/hudglobals.h"
#include <memory>
#include <cstring>
#include "app/gameapp.h"
#include "anim/anim.h"

// FUNCTION: HYPERX 0x004163e0
AnimClip::~AnimClip() = default;

// Two of the eight formats are unimplemented; a size not a multiple of four overruns the buffer.
// FUNCTION: HYPERX 0x00416510
unsigned char AnimClip::Load(FileObj* pFile, const AnimHeader* pSetHeader)
{
    unsigned char  bSuccess = 0;
    AnimClipHeader header;

    do
    {
        if (pFile->Read(&header, sizeof(header)) == 0)
        {
            break;
        }

        m_nSegment = header.wSegment;
        m_nFrame   = header.wFrame;
        m_nFormat  = header.nFormat;

        const auto pBuffer = std::make_unique_for_overwrite<float[]>(static_cast<unsigned short>(header.nDataSize >> 2));

        if (pFile->Read(pBuffer.get(), header.nDataSize) != 0)
        {
            TransformObject xform;

            switch (m_nFormat)
            {
                case 1:
                    m_pTranslation = std::make_unique_for_overwrite<Vec3[]>(pSetHeader->m_nFrameCount);

                    memcpy(m_pTranslation.get(), pBuffer.get(), pSetHeader->m_nFrameCount * sizeof(Vec3));
                    break;

                case 3:
                    m_pTranslation = std::make_unique_for_overwrite<Vec3[]>(pSetHeader->m_nFrameCount);

                    memcpy(m_pTranslation.get(), pBuffer.get(), pSetHeader->m_nFrameCount * sizeof(Vec3));

                    m_pRotations = std::make_unique_for_overwrite<PackedRotation[]>(pSetHeader->m_nFrameCount);

                    Anim::EulerToMatrix(pBuffer.get() + pSetHeader->m_nFrameCount * 3,
                                        m_pRotations.get(),
                                        pSetHeader->m_nFrameCount,
                                        &header,
                                        &xform);
                    break;

                case 2:
                case 4:
                    m_pTranslation = std::make_unique_for_overwrite<Vec3[]>(1);

                    memcpy(m_pTranslation.get(), pBuffer.get(), sizeof(Vec3));

                    m_pRotations = std::make_unique_for_overwrite<PackedRotation[]>(pSetHeader->m_nFrameCount);

                    Anim::EulerToMatrix(pBuffer.get() + 3, m_pRotations.get(), pSetHeader->m_nFrameCount, &header, &xform);
                    break;

                case 5:
                    m_pTranslation = std::make_unique_for_overwrite<Vec3[]>(1);

                    memcpy(m_pTranslation.get(), pBuffer.get(), sizeof(Vec3));

                    m_pQuaternions = std::make_unique_for_overwrite<PackedQuaternion[]>(pSetHeader->m_nFrameCount);
                    break;

                case 6:
                    m_pTranslation = std::make_unique_for_overwrite<Vec3[]>(pSetHeader->m_nFrameCount);

                    memcpy(m_pTranslation.get(), pBuffer.get(), pSetHeader->m_nFrameCount * sizeof(Vec3));

                    m_pQuaternions = std::make_unique_for_overwrite<PackedQuaternion[]>(pSetHeader->m_nFrameCount);
                    break;

                case 7:
                    // STRING: HYPERX 0x004bb7cc
                    g_gameApp.Fatal("ANIM SETUP ERROR: ONE_TRAN_3x3 format not yet support!");
                    break;

                case 8:
                    // STRING: HYPERX 0x004bb794
                    g_gameApp.Fatal("ANIM SETUP ERROR: ALL_TRAN_3x3 format not yet support!");
                    break;
            }

            bSuccess = 1;
        }

    } while (0);

    return bSuccess;
}

// Reading one frame out of a clip, whichever way the clip stores it.

// FUNCTION: HYPERX 0x00416800
unsigned char AnimClip::GetBoneTransform(MatrixRow* pDest, short nFrame)
{
    const PackedBoneMatrix* pSrc;

    switch (m_nFormat)
    {
        case 1:
        case 6:
            pDest[0].af[3] = m_pTranslation[nFrame].x;
            pDest[1].af[3] = m_pTranslation[nFrame].y;
            pDest[2].af[3] = m_pTranslation[nFrame].z;
            return 1;

        case 2:
        case 4:
            Anim::UnpackFixed16ToFloat(&m_pRotations[nFrame], pDest[0].af);
            break;

        case 7:
            pSrc = &m_pMatrices[nFrame];

            pDest[0].vAxis = pSrc->aRows[0].vAxis;
            pDest[1].vAxis = pSrc->aRows[1].vAxis;
            pDest[2].vAxis = pSrc->aRows[2].vAxis;

            return 0;

        case 3:
            pDest[0].af[3] = m_pTranslation[nFrame].x;
            pDest[1].af[3] = m_pTranslation[nFrame].y;
            pDest[2].af[3] = m_pTranslation[nFrame].z;

            Anim::UnpackFixed16ToFloat(&m_pRotations[nFrame], pDest[0].af);

            return 1;
    }

    return 0;
}

// The marker is on the definition in src/globals.cpp.
extern const float g_flOneBone;

// Formats five and seven take the clip's single translation whatever frame was asked for.
// FUNCTION: HYPERX 0x00416930
unsigned char AnimClip::GetBoneTransformFull(MatrixRow* pDest, short nFrame)
{
    const PackedBoneMatrix* pSrc;

    switch (m_nFormat)
    {
        case 1:
            pDest[2].af[1] = g_flZeroBone;
            pDest[2].af[0] = g_flZeroBone;
            pDest[1].af[2] = g_flZeroBone;
            pDest[1].af[0] = g_flZeroBone;
            pDest[0].af[2] = g_flZeroBone;
            pDest[0].af[1] = g_flZeroBone;

            pDest[3].af[3] = g_flOneBone;
            pDest[2].af[2] = g_flOneBone;
            pDest[1].af[1] = g_flOneBone;
            pDest[0].af[0] = g_flOneBone;

            pDest[0].af[3] = m_pTranslation[nFrame].x;
            pDest[1].af[3] = m_pTranslation[nFrame].y;
            pDest[2].af[3] = m_pTranslation[nFrame].z;

            return 1;

        case 2:
        case 4:
            Anim::UnpackFixed16ToFloat(&m_pRotations[nFrame], pDest[0].af);
            break;

        case 5:
            pDest[0].af[3] = m_pTranslation[0].x;
            pDest[1].af[3] = m_pTranslation[0].y;
            pDest[2].af[3] = m_pTranslation[0].z;

            return 1;

        case 6:
            pDest[0].af[3] = m_pTranslation[nFrame].x;
            pDest[1].af[3] = m_pTranslation[nFrame].y;
            pDest[2].af[3] = m_pTranslation[nFrame].z;

            return 1;

        case 7:
            pDest[0].af[3] = m_pTranslation[0].x;
            pDest[1].af[3] = m_pTranslation[0].y;
            pDest[2].af[3] = m_pTranslation[0].z;

            pSrc = &m_pMatrices[nFrame];

            pDest[0].vAxis = pSrc->aRows[0].vAxis;
            pDest[1].vAxis = pSrc->aRows[1].vAxis;
            pDest[2].vAxis = pSrc->aRows[2].vAxis;

            return 1;

        case 8:
            pDest[0].af[3] = m_pTranslation[nFrame].x;
            pDest[1].af[3] = m_pTranslation[nFrame].y;
            pDest[2].af[3] = m_pTranslation[nFrame].z;

            pSrc = &m_pMatrices[nFrame];

            pDest[0].vAxis = pSrc->aRows[0].vAxis;
            pDest[1].vAxis = pSrc->aRows[1].vAxis;
            pDest[2].vAxis = pSrc->aRows[2].vAxis;

            return 1;

        case 3:
            pDest[0].af[3] = m_pTranslation[nFrame].x;
            pDest[1].af[3] = m_pTranslation[nFrame].y;
            pDest[2].af[3] = m_pTranslation[nFrame].z;

            Anim::UnpackFixed16ToFloat(&m_pRotations[nFrame], pDest[0].af);

            return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00417910
AnimClip::AnimClip()
: m_nSegment(0)
, m_nFrame(0)
, m_nFormat(0)
{
}
