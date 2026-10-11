#include "common/compat.h"
#include "animclipset.h"
#include "globals.h"
#include "animglobals.h"
#include "animclip.h"
#include "common/fileobj.h"
#include <new.h>
#include <string.h>
#include "app/gameapp.h"

// FUNCTION: HYPERX 0x004172a0
void AnimClipSet::Init()
{
    m_pszName     = 0;
    m_wEndFrame   = 0;
    m_wUnused     = 0;
    m_wFrameRate  = 0;
    m_wFrameCount = 0;
    m_nClipCount  = 0;
    m_flDuration  = g_flZeroAnimClipSet;
    m_pClips      = 0;
}

// A clip that will not load is fatal and names the set, as is a header that will not read.
// FUNCTION: HYPERX 0x004172d0
void AnimClipSet::Load(FileObj* pFile, const char* pszName)
{
    Init();

    m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);

    AnimHeader header;

    if (pFile->Read(&header, sizeof(header)) != 0)
    {
        m_wFrameCount = header.m_nFrameCount;
        m_wFrameRate  = header.m_wFrameRate;
        m_nClipCount  = header.m_nClipCount;
        m_flDuration  = header.m_flDuration;
        m_wUnused     = header.m_wUnused;
        m_wEndFrame   = header.m_wEndFrame;

        m_pClips = new AnimClip[header.m_nClipCount];

        AnimClip* pClips = m_pClips;

        for (int nIndex = 0; nIndex < m_nClipCount; nIndex++)
        {
            if (pClips->Load(pFile, &header) == 0)
            {
                // STRING: HYPERX 0x004bb840
                g_gameApp.Fatal("Error loading animation %s!", pszName);
            }

            pClips++;
        }
    }
    else
    {
        // STRING: HYPERX 0x004bb820
        g_gameApp.Fatal("Error reading AnimHeader %s!", pszName);
    }
}

// FUNCTION: HYPERX 0x004174a0
AnimClipSet::~AnimClipSet()
{
    if (m_pClips != 0)
    {
        delete[] m_pClips;

        m_pClips = 0;
    }

    if (m_pszName != 0)
    {
        delete[] m_pszName;

        m_pszName = 0;
    }
}
