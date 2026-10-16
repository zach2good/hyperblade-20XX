// SDL's keys as the Win32 virtual-key codes the game reads and its key bindings store.

#ifndef HYPERBLADE_KEYMAP_H
#define HYPERBLADE_KEYMAP_H

namespace KeyMap
{

// Letters follow the keyboard layout, as virtual keys do; digits and every other key go by
// position.
// Zero for a key the game never saw, such as Alt and F10, which Windows sent as system keys.
unsigned int ToVirtualKey(int nScancode, int nKeycode);

} // namespace KeyMap

#endif
