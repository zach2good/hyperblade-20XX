#ifndef HYPERBLADE_PLAYERSLOTNAME_H
#define HYPERBLADE_PLAYERSLOTNAME_H

// A player's name as the session keeps and sends it: the slot and role byte, then the name.
struct PlayerSlotName
{
    // The game messages carry the whole record as one string, the role byte first.
    char* GetWireName()
    {
        return &cSlotFlags;
    }

    char cSlotFlags;
    char szName[63];
};

#endif
