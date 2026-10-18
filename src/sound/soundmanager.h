// The sound manager: fifteen slots of playing state, and the table the list file names.

#ifndef HYPERBLADE_SOUNDMANAGER_H
#define HYPERBLADE_SOUNDMANAGER_H

#define SOUNDMGR_SLOTS 15

// The manager's own "no sound", which the clip index compares against.
#define SOUNDMGR_NO_SOUND (-9999)

// SoundEntry::m_nPlayState.
#define SOUND_STATE_STOPPED 1
#define SOUND_STATE_IDLE    2
#define SOUND_STATE_PLAYING 4

// SoundEntry::m_nLoadState counts failed opens up to the last try; a bad file fails for good.
#define SOUND_LOAD_LAST_TRY 1
#define SOUND_LOAD_FAILED   (-1)

// SoundEntry::m_nLoadMode.
#define SOUND_LOAD_WHOLE 1

struct SoundEntry
{
    int   m_nIndex;      // 0x00, its own position in SoundManager::m_apSounds
    char* m_pszName;     // 0x04, a strcpy of SNDFILES.LST's first column
    int   m_nUnusedA;    // 0x08
    int   m_nDurationMs; // 0x0c, SNDFILES.LST's second column
    int   m_nGroup;      // 0x10, SNDFILES.LST's third column, via SoundManager::Load
    // DirectSound pan and volume in hundredths of a decibel; AnimEvent sets them and nothing reads them.
    int m_nPan;    // 0x14
    int m_nVolume; // 0x18
    // When the play started, in milliseconds since start up.
    unsigned int m_nStartedMs; // 0x1c
    int          m_nPlayState; // 0x20
    // Load clears it; CPanSound tests it through a sound pointer nothing sets.
    int   m_bPannable;      // 0x24
    int   m_nSamplesPerSec; // 0x28, WAVEFORMATEX::nSamplesPerSec
    int   m_nBitsPerSample; // 0x2c, WAVEFORMATEX::wBitsPerSample
    int   m_nChannels;      // 0x30, WAVEFORMATEX::nChannels
    int   m_nDataBytes;     // 0x34, the data chunk's cksize
    int   m_nDataOffset;    // 0x38, the data chunk's nDataOffset
    char* m_pSamples;       // 0x3c, the malloc sized by cksize the samples are read into
    int   m_nLoadMode;      // 0x40
    int   m_nUnusedB;       // 0x44
    int   m_nUnusedC;       // 0x48
    int   m_nLoadState;     // 0x4c
};

class SoundManager
{
public:
    SoundManager();

    // A file that will not open is thrown rather than reported.
    unsigned short Load(const char* pszListName);

    // Indexed by the fifteen SoundTypes ids in DATA/ENUMIDS.H.
    SoundEntry* PickFromGroup(int nGroup);

    void FreeEntries();

    int   m_nUnusedA; // 0x00
    short m_wUnusedA; // 0x04
    // One row per group: first sound, count, and longest length in samples at 44100 Hz.
    short        m_awGroupFirst[SOUNDMGR_SLOTS];   // 0x06
    short        m_awGroupCount[SOUNDMGR_SLOTS];   // 0x24
    short        m_wUnusedB;                       // 0x42
    int          m_anGroupSamples[SOUNDMGR_SLOTS]; // 0x44
    short        m_nSoundCount;                    // 0x80
    short        m_wUnusedC;                       // 0x82
    SoundEntry** m_apSounds;                       // 0x84
    int          m_nUnusedB;                       // 0x88
};

#endif
