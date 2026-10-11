#ifndef HYPERBLADE_NETSETUP_H
#define HYPERBLADE_NETSETUP_H

#include "common/compat.h"

struct PlayerSlotName;

namespace NetSetup
{

void __cdecl   OnPlayerReady(unsigned int nSlot);
void __cdecl   OnPlayerDisconnect(DWORD nId);
void           DisplayChatMessage(int nUnused, const char* pszText, int nLength);
void           ProcessAddPlayerMsg(int nDpid, int nSlot, PlayerSlotName* pName);
void __stdcall OnHostStartGame(void);

} // namespace NetSetup

#endif
