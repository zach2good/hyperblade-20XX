#include "common/compat.h"
#include "common/fileobj.h"
#include "math/vector.h"
#include <stdlib.h>
#include "globals.h"
#include "common/pathstring.h"
#include <string.h>
#include "app/gameapp.h"
#include "anim/animfile.h"
#include "graphics/modelfile.h"
#include "common/staticinit.h"
#include <SDL3/SDL.h>

float g_flFifthModelFile             = 0.2f * g_flOneModelFile;
float g_flDotPitchInchModelFile      = g_flDotsPerInchModelFile * g_flDotPitchModelFile;
float g_flScreenDotsPerInchModelFile = g_flDotsPerInchModelFile;
float g_flDotPitchCmModelFile        = g_flDotPitchInchModelFile / g_flCmPerInchModelFile;
float g_flScreenDotsPerCmModelFile   = g_flScreenDotsPerInchModelFile / g_flCmPerInchModelFile;

#define MODEL_NAME_LENGTH       6
#define MODEL_TAG_DIRECTORY     1
#define MODEL_INTERNAL_NAME_MAX 32

// FUNCTION: HYPERX 0x00435fe0
char ModelFile::Open(FileObj* pFile, const char* pszModelName)
{
    char        bSuccess = 1;
    PathString  strPath(MAX_PATH, g_gameApp.m_strModelPath.m_str.m_pchData, pszModelName, g_pszModelExtension);
    char        szInternalName[MODEL_INTERNAL_NAME_MAX];
    FileVersion version;

    if (g_bShadedDromeModels != 0 && g_cMdmTexLoaded != 0 && strlen(pszModelName) == MODEL_NAME_LENGTH
        // STRING: HYPERX 0x004bdf90
        && SDL_strcasecmp((pszModelName + 3), "dro") == 0)
    {
        strPath.m_str.AssignFromString(String::Concat(
            String::Concat(
                String::Concat(
                    // STRING: HYPERX 0x004bdf80
                    String::Concat(g_gameApp.m_strExeDir.m_str, "Models\\MdlShd\\"),
                    pszModelName),
                // STRING: HYPERX 0x004bdf7c
                "."),
            g_pszModelExtension));
    }

    if (pFile->Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        int nTag;

        if (ModelFile::ReadHeader(pFile, &nTag, szInternalName, &version) != 0 && nTag != MODEL_TAG_DIRECTORY)
        {
            if (AnimFile::VersionCompare(&g_modelVersion, &version) == 0)
            {
                goto done;
            }

            pFile->CloseAndTest();

            g_gameApp.Error(
                // STRING: HYPERX 0x004bdf5c
                "Wrong Version in model file %s.",
                strPath.m_str.m_pchData);
        }
        else
        {
            pFile->CloseAndTest();

            g_gameApp.Error(
                // STRING: HYPERX 0x004bdf18
                "Wrong Type in file %s. Expected database file - found DB directory.",
                strPath.m_str.m_pchData);
        }
    }
    else
    {
        g_gameApp.Error(
            // STRING: HYPERX 0x004bdef4
            "Model file %s could not be opened!",
            strPath.m_str.m_pchData);
    }

    bSuccess = 0;

done:
    return bSuccess;
}

// FUNCTION: HYPERX 0x00493be0
int ModelFile::ReadHeader(FileObj* pFile, int* pnTag, char* pszName, FileVersion* pVersion)
{
    FileHeader header;

    if (pFile->Read(&header, sizeof(header)) != 0)
    {
        *pnTag = header.nTag;

        SDL_strlcpy(pszName, header.szName, sizeof(header.szName));

        *pVersion = header.version;

        return 1;
    }

    return 0;
}
