// A network match. Networking is stubbed out until it is rebuilt, so there is never a session
// and g_pGameSession stays null.

#ifndef HYPERBLADE_GAMESESSION_H
#define HYPERBLADE_GAMESESSION_H

struct GameSession
{
    static void Disconnect();

    // Nought ends the session.
    int PumpNetworkCommands();

    unsigned int nLagMs = 0;
};

#endif
