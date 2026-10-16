#ifndef HYPERBLADE_NETWORK_H
#define HYPERBLADE_NETWORK_H

#include "common/compat.h"

struct GameSession;
struct SyncBlockHeader;
struct PlayerSlotName;

namespace Network
{

int          ShowWaitingDialog(void);
void         FlushPendingPlayerName(void);
int          AddDPID(struct GameSession* pSession, DWORD nId);
int          RemoveDPID(struct GameSession* pSession, DWORD nId);
int          ValidateStaticObjects(void);
unsigned int SetupPlayerSlot(unsigned int nDpid, int nSlot);
unsigned int AbortAndCleanup(HWND hWnd);
void         SendAssetData(int nRequesterId);
void         ReceiveAssetPacket(const SyncBlockHeader* pPacket, unsigned int nLength, int nSenderId);
void         OnPlayerConnected(DWORD idFrom, int nSlot);
void         OnAcceptPlayerSlot(int nPlayer);
int          AddPlayer(struct GameSession* pSession, unsigned int nDpid, PlayerSlotName* pName);
void         OnRejectPlayerSlot(int nPlayer);

} // namespace Network

#endif
