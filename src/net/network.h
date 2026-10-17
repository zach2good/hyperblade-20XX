// The network set-up the menus call into. Networking is stubbed out until it is rebuilt.

#ifndef HYPERBLADE_NETWORK_H
#define HYPERBLADE_NETWORK_H

namespace Network
{

// Waits for the other machines between periods; with no session there is nothing to wait for.
int ShowWaitingDialog();

} // namespace Network

#endif
