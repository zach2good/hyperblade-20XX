// Batch simulation: AI against AI, headless, as fast as the CPU allows, with a CSV of every
// player's match stats.

#ifndef HYPERBLADE_SIMULATOR_H
#define HYPERBLADE_SIMULATOR_H

namespace Simulator
{

// Takes the words after -sim: HOME AWAY COUNT [SEED] [shipped|fixed]. Answers how many it used.
int TakeArguments(int nCount, char** ppszWords);

bool Requested();

// Plays the matches and writes sim_results.csv beside the executable. Answers the exit code.
int Run();

} // namespace Simulator

#endif
