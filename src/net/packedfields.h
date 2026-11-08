// v1.0.0.0's mixed-radix packer: each field multiplies into, or divides out of, one word of a block.

#ifdef HYPERX_V100
#ifndef HYPERBLADE_PACKEDFIELDS_H
#define HYPERBLADE_PACKEDFIELDS_H

struct PackedField
{
    unsigned int nRadix; // 0x00
    int          nWord;  // 0x04
};

namespace PackedFields
{

// Both answer one.
int __cdecl Pack(unsigned int* pWords, int nField, unsigned int nValue);
int __cdecl Unpack(unsigned int* pWords, int nField, unsigned int* pValue);

} // namespace PackedFields

#endif
#endif
