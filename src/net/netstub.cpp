#include "net/cplayer.h"
#include "net/gamesession.h"
#include "net/network.h"

void GameSession::Disconnect()
{
}

int GameSession::PumpNetworkCommands()
{
    return 0;
}

NetMoveRecord* CPlayer::DequeueNetRecord()
{
    return nullptr;
}

int Network::ShowWaitingDialog()
{
    return 1;
}
