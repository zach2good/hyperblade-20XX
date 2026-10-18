// The pending-message list and the table a message name is built from.

#ifndef HYPERBLADE_MESSAGEQUEUE_H
#define HYPERBLADE_MESSAGEQUEUE_H
#include "common/compat.h"
#include "common/clock.h"

#include "globals.h"
#include <deque>
#include <string>
#include <vector>

struct MessageNode
{
    std::string m_text;
    int         m_nLine;
    int         m_nDurationMs;
};

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

    // Frees every name and id, and returns the queue.
    MessageQueue* FreeAll();

    // One line per message, an id and then the text.
    unsigned int LoadList(const char* pszFileName);

private:
    std::vector<std::string> m_names;
    std::vector<int>         m_ids;
    // When the message on show comes down; zero when none is.
    Clock::Milliseconds m_expiryTick;
    // Oldest first.
    std::deque<MessageNode> m_pending;
};

#endif
