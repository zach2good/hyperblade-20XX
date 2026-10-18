#ifndef HYPERBLADE_MATERIALNAME_H
#define HYPERBLADE_MATERIALNAME_H

// One name in a material or texture table, padded to a fixed width.
constexpr auto kMaterialNameBytes = 0x10;
typedef char   MaterialName[kMaterialNameBytes];

#endif
