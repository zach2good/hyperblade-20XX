// The message words: packing numbers into one word and splitting them out again.

#include "common/compat.h"
#include "engine/msgevent.h"

// Four fields, one per octet, with the top bit set to mark the word; only the low field is masked.
// FUNCTION: HYPERX 0x0045d730
int MsgEvent::Pack(short nKind, short nHigh, short nMiddle, short nLow)
{
    return (((((nKind | 0x80) * 0x100) | nHigh) * 0x100 | nMiddle) * 0x100) | (nLow & 0xff);
}

// Fields of ten, eight and fourteen bits, each masked to its width.
// FUNCTION: HYPERX 0x0045d760
unsigned int MsgEvent::PackScene(unsigned int nMessage, unsigned int nSender, short nValue)
{
    return ((((nMessage & 0x3ff) << 8) | (nSender & 0xff)) * 0x4000) | (nValue & 0x3fff);
}

// Splits a PackScene word into its three fields; each lands in a short.
// FUNCTION: HYPERX 0x0045d790
void MsgEvent::UnpackScene(unsigned int nPacked, short* pMessage, short* pSender, short* pValue)
{
    *pMessage = static_cast<short>((nPacked >> 22) & 0x3ff);
    *pSender  = static_cast<short>((nPacked >> 14) & 0xff);
    *pValue   = static_cast<short>(nPacked & 0x3fff);
}

// FUNCTION: HYPERX 0x0045d7d0
void MsgEvent::Unpack(unsigned int nPacked, short* pKind, short* pHigh, short* pMiddle, short* pLow)
{
    *pKind   = static_cast<short>((nPacked >> 24) & 0x7f);
    *pHigh   = static_cast<short>((nPacked >> 16) & 0xff);
    *pMiddle = static_cast<short>((nPacked >> 8) & 0xff);
    *pLow    = static_cast<short>(nPacked & 0xff);
}
