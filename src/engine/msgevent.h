#ifndef HYPERBLADE_MSGEVENT_H
#define HYPERBLADE_MSGEVENT_H

#include "common/compat.h"

// Pack sets the top bit of a collision event; a scene event's message field leaves it clear.
#define MSG_EVENT_COLLISION_MARK 0x80000000

namespace MsgEvent
{

int          Pack(short nKind, short nHigh, short nMiddle, short nLow);
unsigned int PackScene(unsigned int nMessage, unsigned int nSender, short nValue);
void         UnpackScene(unsigned int nPacked, short* pMessage, short* pSender, short* pValue);
void         Unpack(unsigned int nPacked, short* pKind, short* pHigh, short* pMiddle, short* pLow);

} // namespace MsgEvent

#endif
