#ifdef HYPERX_V100
#include "common/compat.h"
#include "net/packedfields.h"

// TODO: The field count is a guess; nothing in v1.0.0.0 fills or bounds the table.
#define PACKED_FIELD_COUNT 16

// GLOBAL: HYPERX100 0x005265e8
PackedField g_aPackedFields[PACKED_FIELD_COUNT];

// FUNCTION: HYPERX100 0x00493290
int __cdecl PackedFields::Pack(unsigned int* pWords, int nField, unsigned int nValue)
{
    const PackedField& field = g_aPackedFields[nField];

    pWords[field.nWord] = field.nRadix * pWords[field.nWord] + nValue;

    return 1;
}

// FUNCTION: HYPERX100 0x004932d0
int __cdecl PackedFields::Unpack(unsigned int* pWords, int nField, unsigned int* pValue)
{
    const int nWord = g_aPackedFields[nField].nWord;

    *pValue       = pWords[nWord] % g_aPackedFields[nField].nRadix;
    pWords[nWord] = pWords[nWord] / g_aPackedFields[nField].nRadix;

    return 1;
}
#endif
