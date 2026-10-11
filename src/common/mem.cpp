#include "compat.h"
#include "common/mem.h"
#include "common/staticinit.h"

float g_flUnusedDisplayModeA = g_flZeroDisplayMode;
float g_flUnusedDisplayModeB = g_flZeroDisplayMode;
float g_flUnusedDisplayModeC = g_flZeroDisplayMode;

// Hand-written assembly in the original: REP STOSW, which VC6 never emits, and a frame pointer
// in an /O2 function with no locals, which only an __asm block gives it.
// What it does: clears 192 bytes as three runs of 64, first dwords, then words, then bytes, with
// one pointer walking on through all three.
// Why assembly: VC6 turns every C spelling of the word and byte runs into REP STOSD.
// FUNCTION: HYPERX 0x00439590
void __cdecl Mem::ZeroBlock192(void* pBlock)
{
    __asm
    {
        mov edi, pBlock
        xor eax, eax
        mov ecx, 0x10
        rep stosd
        mov ecx, 0x20
        rep stosw
        mov ecx, 0x40
        rep stosb
    }
}

// Hand-written assembly in the original: two dead loads in an unrolled fill, of the dwords at the
// base and at 0x20 into EAX and EBX, which nothing reads; REP STOSW; and MOV stores in an order no
// C statement order gives.
// What it does: clears 128 bytes. Sixteen dwords go from 0x1c and 0x3c
// downwards in pairs, then the word run starts again from the base, so the first 64 bytes are
// cleared twice before a byte run clears the next 64.
// Why assembly: C has no dead load and VC6 never writes a word run with REP STOSW.
// FUNCTION: HYPERX 0x004395c0
void __cdecl Mem::ZeroBlockUnrolled128(void* pBlock)
{
    __asm
    {
        mov edi, pBlock
        xor ecx, ecx
        mov eax, [edi]
        mov ebx, [edi + 0x20]
        mov [edi + 0x1c], ecx
        mov [edi + 0x3c], ecx
        mov [edi + 0x18], ecx
        mov [edi + 0x38], ecx
        mov [edi + 0x14], ecx
        mov [edi + 0x34], ecx
        mov [edi + 0x10], ecx
        mov [edi + 0x30], ecx
        mov [edi + 0x0c], ecx
        mov [edi + 0x2c], ecx
        mov [edi + 0x08], ecx
        mov [edi + 0x28], ecx
        mov [edi + 0x04], ecx
        mov [edi + 0x24], ecx
        mov [edi], ecx
        mov [edi + 0x20], ecx
        xor eax, eax
        mov ecx, 0x20
        rep stosw
        mov ecx, 0x40
        rep stosb
    }
}
