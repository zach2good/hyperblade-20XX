// The destructor carries no marker: the linker folded every empty body onto 0x00416e30.

#include "common/compat.h"
#include "meshmaterial.h"
#include "modeldatabase.h"
#include "globals.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include <new.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include "common/fileobj.h"
#include "app/gameapp.h"

extern Vec3 g_vLightDirModel;

// Answers the face's FaceShade and level; a face with no normal is lit white.
// FUNCTION: HYPERX 0x00433820
char __cdecl MeshMaterial::CalcLightIntensity(float* pflOut, const MeshMaterial* pMaterial, const Vec3* pNormal)
{
    float flLevel;

    if (pNormal == 0)
    {
        return SHADE_WHITE;
    }

    flLevel = (g_vLightDirModel.x * pNormal->x + (g_vLightDirModel.y * pNormal->y + g_vLightDirModel.z * pNormal->z)) * pMaterial->flDiffuse;
    flLevel += pMaterial->flAmbient;
    flLevel += pMaterial->flEmissive;

    *pflOut = flLevel;

    if (flLevel > g_flZeroLightLevel)
    {
        if (flLevel < 1.0f)
        {
            return SHADE_SCALED;
        }

        return SHADE_WHITE;
    }

    return SHADE_BLACK;
}

#define LINE_MAX      120
#define NAME_COPY_MAX 0x14
#define CHANNEL_MAX   0xff
#define MATERIAL_MAX  0xff

// A complaint before the first newmtl names index minus one. r8 and r16 both store nought.
// FUNCTION: HYPERX 0x004364e0
char __cdecl MeshMaterial::ParseFile(FileObj* pFile, const char*, MeshMaterial** ppMaterials, MaterialName** ppMatNames, MaterialName** ppTexNames, unsigned char* pnMatCount, short* pnTexCount)
{
    float flValue;
    short nColourIndex;
    short nBlue;
    short nGreen;
    short nRed;
    char  szKeyword[LINE_MAX];
    char  szValue[LINE_MAX];
    char  szRamp[80];
    char  szLine[LINE_MAX];

    int   nLength;
    int   i;
    short j;

    *pnMatCount              = 0;
    *pnTexCount              = 0;
    MaterialName* paTexNames = 0;
    int           nTexCount  = 0;
    *ppTexNames              = 0;
    char* pszLine            = szLine;
    short nMatIndex          = -1;
    char  bResult            = 1;

    while ((nLength = pFile->GetNextLine(&pszLine)) > 0)
    {
        if (nLength > LINE_MAX)
        {
            g_gameApp.Error(
                // STRING: HYPERX 0x004be3ac
                "Read error: Material file line > 120 characters!");

            bResult = 0;
        }
        else
        {
            // STRING: HYPERX 0x004be3a8
            if (sscanf(pszLine, "%s ", szKeyword) == 1)
            {
                // STRING: HYPERX 0x004be3a0
                if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"newmtl") == 0)
                {
                    (*pnMatCount)++;
                }
                // STRING: HYPERX 0x004be39c
                else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"tx") == 0)
                {
                    (*pnTexCount)++;
                }
            }
        }
    }

    MeshMaterial* const pMaterials = new MeshMaterial[*pnMatCount];

    *ppMaterials = pMaterials;

    MaterialName* const paMatNames = new MaterialName[*pnMatCount];

    *ppMatNames = paMatNames;

    if (*pnTexCount != 0)
    {
        paTexNames = new MaterialName[*pnTexCount];

        *ppTexNames = paTexNames;

        memset(paTexNames, 0, *pnTexCount * sizeof(MaterialName));
    }

    memset(paMatNames, 0, *pnMatCount * sizeof(MaterialName));

    pFile->SeekRelative(0, FILE_SEEK_START);

    while ((nLength = pFile->GetNextLine(&pszLine)) > 0)
    {
        if (nLength > LINE_MAX)
        {
            g_gameApp.Error("Read error: Material file line > 120 characters!");

            bResult = 0;
        }
        else if (sscanf(pszLine, "%s ", szKeyword) == 1)
        {
            if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"newmtl") == 0)
            {
                // STRING: HYPERX 0x004be394
                if (sscanf(pszLine, "%s %s", szKeyword, szValue) != 2)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be364
                        "Material File Error reading material name: %s!",
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    int nCopy = NAME_COPY_MAX;
                    nLength   = strlen(szValue);

                    if (nLength < nCopy)
                    {
                        nCopy = nLength;
                    }

                    if (strlen(szValue) >= MATERIAL_NAME_BYTES)
                    {
                        g_gameApp.Error(
                            // STRING: HYPERX 0x004be344
                            "material name %s > %d too long!",
                            szValue,
                            MATERIAL_NAME_BYTES);

                        bResult = 0;
                    }

                    nMatIndex++;

                    _mbsncpy((unsigned char*)(paMatNames[nMatIndex]),
                             (unsigned char*)szValue,
                             nCopy);

                    pMaterials[nMatIndex].nColour       = 0;
                    pMaterials[nMatIndex].flAmbient     = 0.0f;
                    pMaterials[nMatIndex].flDiffuse     = 0.0f;
                    pMaterials[nMatIndex].flEmissive    = 0.0f;
                    pMaterials[nMatIndex].nTextureIndex = 0;
                    pMaterials[nMatIndex].nBackIndex    = -1;
                }
            }
            // STRING: HYPERX 0x004be340
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"Cr") == 0)
            {
                // STRING: HYPERX 0x004be32c
                const int nFields = sscanf(pszLine, "%s %hd %hd %hd %s", szKeyword, &nRed, &nGreen, &nBlue, szRamp);

                if (nFields < 4)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be2fc
                        "Material File Error reading color for: %s %s!",
                        paMatNames[nMatIndex],
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    if (nRed > CHANNEL_MAX || nRed < 0 || nBlue > CHANNEL_MAX || nBlue < 0 || nGreen > CHANNEL_MAX || nGreen < 0)
                    {
                        g_gameApp.Error("Material File Error reading color for: %s %s!",
                                        paMatNames[nMatIndex],
                                        pszLine);

                        bResult = 0;
                    }

                    if (nFields == 5)
                    {
                        // STRING: HYPERX 0x004be2f8
                        if (_mbsicmp((const unsigned char*)szRamp, (const unsigned char*)"r8") == 0)
                        {
                            pMaterials[nMatIndex].nColour = 0;
                        }
                        // STRING: HYPERX 0x004be2f4
                        else if (_mbsicmp((const unsigned char*)szRamp, (const unsigned char*)"r16") != 0)
                        {
                            g_gameApp.Error(
                                // STRING: HYPERX 0x004be2c4
                                "Material File Error bad ramp code for: %s %s!",
                                paMatNames[nMatIndex],
                                pszLine);

                            bResult = 0;
                        }
                        else
                        {
                            pMaterials[nMatIndex].nColour = 0;
                        }
                    }
                    else
                    {
                        pMaterials[nMatIndex].nColour = 0;
                    }
                }
            }
            // STRING: HYPERX 0x004be2c0
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"Cp") == 0)
            {
                // STRING: HYPERX 0x004be2b8
                if (sscanf(pszLine, "%s %hd", szKeyword, &nColourIndex) != 2)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be284
                        "Material File Error reading color index for: %s %s!",
                        paMatNames[nMatIndex],
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    pMaterials[nMatIndex].nColour = nColourIndex;
                }
            }
            // STRING: HYPERX 0x004be280
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"Ca") == 0)
            {
                // STRING: HYPERX 0x004be278
                if (sscanf(pszLine, "%s %f", szKeyword, &flValue) != 2)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be240
                        "Material File Error reading ambient coeff for: %s %s!",
                        paMatNames[nMatIndex],
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    pMaterials[nMatIndex].flAmbient = flValue;
                }
            }
            // STRING: HYPERX 0x004be23c
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"Cd") == 0)
            {
                if (sscanf(pszLine, "%s %f", szKeyword, &flValue) != 2)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be204
                        "Material File Error reading disfuse coeff for: %s %s!",
                        paMatNames[nMatIndex],
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    pMaterials[nMatIndex].flDiffuse = flValue;
                }
            }
            // STRING: HYPERX 0x004be200
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"Ce") == 0)
            {
                if (sscanf(pszLine, "%s %f", szKeyword, &flValue) != 2)
                {
                    // The only complaint here that is fatal rather than a report.
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004be1c8
                        "Material File Error reading emittance coeff for: %s %s!",
                        paMatNames[nMatIndex],
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    pMaterials[nMatIndex].flEmissive = flValue;
                }
            }
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"tx") == 0)
            {
                if (sscanf(pszLine, "%s %s", szKeyword, szValue) != 2)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be190
                        "Material File Error decoding texture name for: %s %s!",
                        paMatNames[nMatIndex],
                        pszLine);

                    bResult = 0;
                }
                else
                {
                    if (strlen(szValue) >= MATERIAL_NAME_BYTES)
                    {
                        g_gameApp.Error(
                            // STRING: HYPERX 0x004be168
                            "texture name %s > %d too long for: %s!",
                            szValue,
                            MATERIAL_NAME_BYTES,
                            paMatNames[nMatIndex]);

                        bResult = 0;
                    }

                    const int nKnownTextures = (short)nTexCount;

                    for (i = 0; i < nKnownTextures; i++)
                    {
                        if (_mbsicmp((const unsigned char*)paTexNames[i], (const unsigned char*)szValue) == 0)
                        {
                            break;
                        }
                    }

                    if (i < nKnownTextures)
                    {
                        pMaterials[nMatIndex].nTextureIndex = i + 1;
                    }
                    else
                    {
                        strcpy(paTexNames[nKnownTextures], szValue);

                        nTexCount++;

                        pMaterials[nMatIndex].nTextureIndex = (short)nTexCount;
                    }
                }
            }
            // STRING: HYPERX 0x004be164
            else if (_mbsicmp((const unsigned char*)szKeyword, (const unsigned char*)"bf") == 0)
            {
                if (sscanf(pszLine, "%s %s", szKeyword, szValue) != 2)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004be124
                        "Material File Error decoding backface material name for: %s %s!",
                        paMatNames[nMatIndex],
                        szValue);

                    bResult = 0;
                }
                else
                {
                    for (j = 0; j <= nMatIndex; j++)
                    {
                        if (_mbsicmp((const unsigned char*)szValue, (const unsigned char*)paMatNames[j]) == 0)
                        {
                            pMaterials[nMatIndex].nBackIndex = j;

                            break;
                        }
                    }

                    if (j > nMatIndex)
                    {
                        g_gameApp.Error(
                            // STRING: HYPERX 0x004be0e4
                            "Material File Error backface material name - not found for: %s!",
                            paMatNames[nMatIndex],
                            szValue);

                        bResult = 0;
                    }
                }
            }
        }
    }

    nMatIndex++;

    if (nMatIndex > MATERIAL_MAX)
    {
        g_gameApp.Error(
            // STRING: HYPERX 0x004be0b4
            "Material File Error: Material Count > 255:: %d!",
            nMatIndex);

        return 0;
    }

    return bResult;
}
