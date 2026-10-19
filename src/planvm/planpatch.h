#ifndef HYPERBLADE_PLANPATCH_H
#define HYPERBLADE_PLANPATCH_H

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace PlanPatch
{

// The shipped plan with the fixes from docs/plan_bugs.md applied. Nothing when the plan has no
// fixes, or its words differ from the ones the fixes were written against.
std::optional<std::vector<int> > Fixed(std::string_view name, std::span<const int> words);

// Every plan Fixed has fixes for.
std::vector<std::string_view> Names();

} // namespace PlanPatch

#endif
