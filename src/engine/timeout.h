// A called time out, which the original's string table names and nothing in either release does.
// A team calls one a period while play is stopped, and its two skaters rest as if benched.

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
