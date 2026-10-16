#include "common/compat.h"
#include "globals.h"
#include "mixer.h"
#include <mmsystem.h>
#include <new.h>
#include <stdio.h>
#include <string.h>

Mixer g_mixerCompactDisc;

#define MIXER_MESSAGE_MAX 0x104

// Returned when the search ran out of devices, which is not a multimedia result at all.
#define MIXER_NOT_FOUND ((unsigned int)-1)

// Every exit from the search is a throw; zero is the fader found and minus one is none.
// FUNCTION: HYPERX 0x00491330
int Mixer::Open(unsigned int nComponentType)
{
    char              szMessage[MIXER_MESSAGE_MAX];
    MIXERCAPS         caps;
    MIXERLINE         line;
    MIXERLINECONTROLS controls;

    if (m_hMixer != 0)
    {
        mixerClose(m_hMixer);

        m_hMixer = 0;
    }

    HMIXER hMixer = 0;

    const unsigned int nDevices = mixerGetNumDevs();

    if (nDevices == 0)
    {
        return 0;
    }

    try
    {
        for (unsigned int nDevice = 0; nDevice < nDevices; nDevice++)
        {
            MMRESULT nStatus = mixerGetDevCapsA(nDevice, &caps, sizeof(caps));

            if (nStatus != MMSYSERR_NOERROR)
            {
                throw (unsigned int)nStatus;
            }

            nStatus = mixerOpen(&hMixer, nDevice, 0, 0, 0);

            if (nStatus != MMSYSERR_NOERROR)
            {
                throw (unsigned int)nStatus;
            }

            for (unsigned int nLine = 0; nLine < caps.cDestinations; nLine++)
            {
                line.cbStruct        = sizeof(line);
                line.dwDestination   = nLine;
                line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

                if (mixerGetLineInfoA((HMIXEROBJ)hMixer, &line, MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
                {
                    continue;
                }

                line.cbStruct        = sizeof(line);
                line.dwDestination   = nLine;
                line.dwComponentType = nComponentType;

                if (mixerGetLineInfoA((HMIXEROBJ)hMixer, &line, MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
                {
                    continue;
                }

                if (line.cControls == 0)
                {
                    continue;
                }

                MIXERCONTROL* pControls = new MIXERCONTROL[line.cControls];

                controls.cbStruct  = sizeof(controls);
                controls.dwLineID  = line.dwLineID;
                controls.cControls = line.cControls;
                controls.cbmxctrl  = sizeof(MIXERCONTROL);
                controls.pamxctrl  = pControls;

                if (mixerGetLineControlsA((HMIXEROBJ)hMixer, &controls, MIXER_GETLINECONTROLSF_ALL) != MMSYSERR_NOERROR)
                {
                    delete[] pControls;

                    continue;
                }

                for (unsigned int nControl = 0; nControl < controls.cControls; nControl++)
                {
                    if ((pControls[nControl].fdwControl & MIXERCONTROL_CONTROLF_DISABLED) != 0)
                    {
                        continue;
                    }

                    MIXERCONTROL* const pControl = &pControls[nControl];

                    if ((pControl->dwControlType & MIXERCONTROL_CT_CLASS_MASK) == MIXERCONTROL_CT_CLASS_FADER)
                    {
                        m_line    = line;
                        m_control = *pControl;

                        delete[] pControls;

                        throw (unsigned int)MMSYSERR_NOERROR;
                    }
                }

                delete[] pControls;
            }
        }

        throw MIXER_NOT_FOUND;
    }
    catch (unsigned int nResult)
    {
        switch (nResult)
        {
            case MMSYSERR_NOERROR:
                m_hMixer = hMixer;

                return 1;

            case MMSYSERR_BADDEVICEID:
                // STRING: HYPERX 0x004c8bec
                strcpy(szMessage, "The hmxobj parameter specifies an invalid device identifier.");
                break;

            case MMSYSERR_INVALFLAG:
                // STRING: HYPERX 0x004c8bcc
                strcpy(szMessage, "One or more flags are invalid.");
                break;

            case MMSYSERR_INVALHANDLE:
                // STRING: HYPERX 0x004c8b98
                strcpy(szMessage, "The hmxobj parameter specifies an invalid handle.");
                break;

            case MMSYSERR_INVALPARAM:
                // STRING: HYPERX 0x004c8b74
                strcpy(szMessage, "One or more parameters are invalid.");
                break;

            case MMSYSERR_NODRIVER:
                // STRING: HYPERX 0x004c8b30
                strcpy(szMessage, "No mixer device is available for the object specified by hmxobj.");
                break;

            case MIXERR_INVALLINE:
                // STRING: HYPERX 0x004c8b08
                strcpy(szMessage, "The audio line reference is invalid.");
                break;

            case MIXER_NOT_FOUND:
                // STRING: HYPERX 0x004c8ad8
                strcpy(szMessage, "No CD Audio mixer found.");
                break;

            case MIXERR_INVALCONTROL:
                // STRING: HYPERX 0x004c8ab4
                strcpy(szMessage, "The control reference is invalid.");
                break;

            default:
                // STRING: HYPERX 0x004c8af4
                sprintf(szMessage, "mixer error: %08X\n", nResult);
                break;
        }

        if (hMixer != 0)
        {
            mixerClose(hMixer);
        }

        if (m_hMixer != 0)
        {
            mixerClose(m_hMixer);

            m_hMixer = 0;
        }
    }

    return 0;
}

// Thirty-two slots reserved for a request that asks for four.
#define MIXER_DETAIL_SLOTS 32

// Unlike ReadVolume, the result is clamped to the control's own bounds.
// FUNCTION: HYPERX 0x004916c0
int Mixer::FadeVolume(int nVolume)
{
    MIXERCONTROLDETAILS        details;
    MIXERCONTROLDETAILS_SIGNED aValues[MIXER_DETAIL_SLOTS];

    if (m_hMixer == 0)
    {
        return 0;
    }

    details.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    details.dwControlID    = m_control.dwControlID;
    details.cChannels      = 1;
    details.cMultipleItems = m_control.cMultipleItems;
    details.cbDetails      = sizeof(MIXERCONTROLDETAILS_SIGNED);
    details.paDetails      = aValues;

    if (mixerGetControlDetailsA((HMIXEROBJ)m_hMixer, &details, 0) != MMSYSERR_NOERROR)
    {
        return 0;
    }

    const int nMaximum = m_control.Bounds.lMaximum;
    const int nMinimum = m_control.Bounds.lMinimum;

    const float flRange = (float)(nMaximum - nMinimum);

    int nValue = (int)(flRange * ((float)nVolume * (1.0f / (MIXER_VOLUME_LEVELS - 1))) + 0.5f) + nMinimum;

    if (nValue < nMinimum)
    {
        nValue = nMinimum;
    }
    else if (nValue > nMaximum)
    {
        nValue = nMaximum;
    }

    aValues[0].lValue = nValue;

    return mixerSetControlDetails((HMIXEROBJ)m_hMixer, &details, 0) == MMSYSERR_NOERROR;
}

// Nothing clamps the result, so a control outside its own bounds returns a step out of range.
// FUNCTION: HYPERX 0x004917a0
int Mixer::ReadVolume()
{
    MIXERCONTROLDETAILS        details;
    MIXERCONTROLDETAILS_SIGNED aValues[MIXER_DETAIL_SLOTS];

    if (m_hMixer == 0)
    {
        return 0;
    }

    details.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    details.dwControlID    = m_control.dwControlID;
    details.cChannels      = 1;
    details.cMultipleItems = m_control.cMultipleItems;
    details.cbDetails      = sizeof(MIXERCONTROLDETAILS_SIGNED);
    details.paDetails      = aValues;

    if (mixerGetControlDetailsA((HMIXEROBJ)m_hMixer, &details, 0) != MMSYSERR_NOERROR)
    {
        return 0;
    }

    const float flOffset = (float)(aValues[0].lValue - m_control.Bounds.lMinimum);
    const float flRange  = (float)(m_control.Bounds.lMaximum - m_control.Bounds.lMinimum);

    return (int)(flOffset / flRange * (MIXER_VOLUME_LEVELS - 1) + 0.5f);
}
