// A called time out, which only the string table names: one a period, and the skaters rest as if benched.

#ifndef HYPERBLADE_TIMEOUT_H
#define HYPERBLADE_TIMEOUT_H

namespace TimeOut
{

// False when the team has used its time out this period or play is live.
bool Call(int nTeam);

// Every period starts with both teams' time outs unused.
void Reset();

} // namespace TimeOut

#endif
