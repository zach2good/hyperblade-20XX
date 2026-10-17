// Controller button pictures, Kenney's Input Prompts (CC0), as PNG from the pak.

#ifndef HYPERBLADE_PROMPTICONS_H
#define HYPERBLADE_PROMPTICONS_H

#include <vector>

namespace PromptIcons
{

// The PNG named for a [Gamepad] control, such as "A" or "RightTrigger", or for a stick direction,
// "StickUp" to "StickRight"; empty for a name without one.
std::vector<unsigned char> Find(const char* pszName);

} // namespace PromptIcons

#endif
