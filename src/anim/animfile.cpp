#include "common/compat.h"
#include "graphics/modelfile.h"
#include "common/fileobj.h"
#include "globals.h"
#include "common/pathstring.h"
#include <cstring>
#include "app/gameapp.h"
#include "anim/animfile.h"

// The extension every animation file carries.
// GLOBAL: HYPERX 0x004b40f4
extern const char* const g_pszAnimExtension;

// The type names the mismatch complaint prints, indexed by type.
// GLOBAL: HYPERX 0x004b411c
extern const char* const g_apFileTypeNames[9];

// The version an animation file must carry.
// GLOBAL: HYPERX 0x004bb8a8
extern FileVersion g_animVersion;

constexpr auto kAnimFileType        = 4;
constexpr auto kAnimInternalNameMax = 32;

// Minus one when any component of the right is smaller, one when any is larger, else nought.
// FUNCTION: HYPERX 0x00493c40
short AnimFile::VersionCompare(const FileVersion* pLeft, const FileVersion* pRight)
{
    if (pRight->nMajor < pLeft->nMajor || pRight->nMinor < pLeft->nMinor || pRight->nRevision < pLeft->nRevision || pRight->nBuild < pLeft->nBuild)
    {
        return -1;
    }

    if (pRight->nMajor > pLeft->nMajor || pRight->nMinor > pLeft->nMinor || pRight->nRevision > pLeft->nRevision || pRight->nBuild > pLeft->nBuild)
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00493cb0
AnimClipSet* AnimFile::Open(const char* pszName, AnimRegistry* pRegistry, int nFlags, AnimLoadFn pfnLoad)
{
    AnimClipSet* pResult = nullptr;
    PathString   strPath(kMaxPathLength, g_gameApp.m_strAnimDataPath.m_str.m_pchData, pszName, g_pszAnimExtension);
    OwnedFile    file;
    int          nType;
    FileVersion  version;

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) != 0)
    {
        char szInternalName[kAnimInternalNameMax] = { 0 };

        if (ModelFile::ReadHeader(&file.m_file, &nType, szInternalName, &version) != 0)
        {
            if (nType == kAnimFileType)
            {
                if (AnimFile::CheckVersion(szInternalName, &g_animVersion, &version) != 0)
                {
                    pResult = pfnLoad(&file.m_file, pRegistry, nFlags, pszName);

                    file.m_file.CloseAndTest();

                    goto done;
                }
            }
            else
            {
                // STRING: HYPERX 0x004c8dc8
                g_gameApp.Error("Error: %s expected - %s encountered!",
                                g_apFileTypeNames[kAnimFileType],
                                g_apFileTypeNames[nType]);

                file.m_file.CloseAndTest();

                goto done;
            }
        }
        else
        {
            // STRING: HYPERX 0x004c8da0
            g_gameApp.Error("Error reading Animation file header!");
        }

        // One close serves a header that would not read and a version that did not match.
        file.m_file.CloseAndTest();
    }
    else
    {
        // STRING: HYPERX 0x004c8d78
        g_gameApp.Error("Animation file %s could not be opened!");
    }

done:
    return pResult;
}

// FUNCTION: HYPERX 0x00493e60
int AnimFile::CheckVersion(const char* pszName, const FileVersion* pExpected, const FileVersion* pFound)
{
    const int nOrder = AnimFile::VersionCompare(pExpected, pFound);

    if (nOrder != -1)
    {
        if (nOrder != 0)
        {
            // STRING: HYPERX 0x004c8e3c
            g_gameApp.Error("Animation file %s version is greater than the version expected for the game.", pszName);

            return 0;
        }

        return 1;
    }

    // STRING: HYPERX 0x004c8df0
    g_gameApp.Error("Animation file %s version is ealier than the version expected for the game.", pszName);

    return 0;
}

// The version an animation file must carry: 1, 1, 0.
FileVersion g_animVersion = { 1, 1, 0, 0, 0 };
