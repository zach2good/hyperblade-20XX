// The pending-message list and the table a message name is built from.

#ifndef HYPERBLADE_MESSAGEQUEUE_H
#define HYPERBLADE_MESSAGEQUEUE_H
#include "common/compat.h"
#include "common/clock.h"

#include "globals.h"
#include <string.h>

// The node splices itself onto the tail and takes its own copy of the name.
struct MessageNode
{
    MessageNode(const char* pszText, int nLine, int nDurationMs);

    char*        m_pszText;     // 0x00
    int          m_nLine;       // 0x04
    int          m_nDurationMs; // 0x08
    MessageNode* m_pNext;       // 0x0c
};

inline MessageNode::MessageNode(const char* pszText, int nLine, int nDurationMs)
{
    char* pCopy;

    m_pNext = nullptr;

    if (g_pMessageHead != nullptr)
    {
        g_pMessageTail->m_pNext = this;
    }
    else
    {
        g_pMessageHead = this;
    }

    g_pMessageTail = this;

    m_pNext = nullptr;

    pCopy = static_cast<char*>(operator new(strlen(pszText) + 1));

    m_pszText     = strcpy(pCopy, pszText);
    m_nLine       = nLine;
    m_nDurationMs = nDurationMs;
}

class MessageQueue
{
public:
    MessageQueue();

    // Reads m_pnIds.
    int GetAt(int nIndex) const;

    void PushNamedFromTable(const char* pszPrefix, int nIndex, int nLine, int nDurationMs);

    // The same push with no prefix: the entry is the table text itself.
    void PushFromTable(int nIndex, int nLine, int nDurationMs);

    // The entry is the text itself, with no table and no prefix.
    void Push(const char* pszText, int nLine, int nDurationMs);

    // Takes down the message on show and empties the list.
    void ProcessAndClear();

    // Takes down the message on show once it expires, or shows the next one.
    void Update();

    // Frees every name and the block they came from, and returns the queue.
    MessageQueue* FreeAll();

    // One line per message, an id and then the text.
    unsigned int LoadList(const char* pszFileName);

private:
    const char** m_ppNames; // 0x00
    int*         m_pnIds;   // 0x04
    short        m_wCount;  // 0x08
    // When the message on show comes down; zero when none is.
    Clock::Milliseconds m_expiryTick; // 0x0c
};

#endif
