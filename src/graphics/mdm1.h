#ifndef HYPERBLADE_MDM1_H
#define HYPERBLADE_MDM1_H

#include "common/compat.h"

class FileObj;

// The tag an MDM1-packed file opens with, "MDM1" read as a little-endian dword.
#define MDM1_MAGIC 0x314d444d

typedef int (*MDM1ReadFn)(char* pBuffer, int nCount, FileObj* pFile);

// The decoder for MDM1, the LZSS packing every asset file can carry.
namespace MDM1
{

int   ReadCallback(char* pBuffer, int nCount, FileObj* pFile);
void* Decompress(FileObj* pFile);
int   LZSSDecompress(MDM1ReadFn pfnRead, FileObj* pFile, unsigned char* pOut, int nOutCapacity, unsigned char* pWindow, int nWindowSize);

} // namespace MDM1

#endif
