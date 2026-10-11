// MDM1 LZSS: sixteen flags a word; a reference is 12 bits of distance and 4 of length plus three.

#include "common/compat.h"
#include "common/fileobj.h"
#include <stdlib.h>
#include "common/staticinit.h"

float g_flUnusedMDM1A = g_flZeroMDM1;
float g_flUnusedMDM1B = g_flZeroMDM1;
float g_flUnusedMDM1C = g_flZeroMDM1;

// The decompressor's own arguments, which the refill reads through its EBP.
#define CALLER_READ_FN     0x08
#define CALLER_FILE        0x0c
#define CALLER_WINDOW      0x14
#define CALLER_WINDOW_SIZE 0x18

// Where PUSHAD saved EAX, so POPAD returns what is written there.
#define PUSHAD_EAX 0x1c

#define FLAG_SENTINEL 0x10000
#define FLAG_MASK     0x1ffff

#define MATCH_LENGTH_BIAS 3
#define MATCH_LENGTH_LONG 0x12
#define MATCH_LENGTH_END  0x10

#define WINDOW_MARGIN 0x80

#define WINDOW_BYTES 0x1000

// The window is freed before the size check that can discard the output.
// FUNCTION: HYPERX 0x00440140
void* MDM1::Decompress(FileObj* pFile)
{
    int nSize;

    unsigned char* const pWindow = (unsigned char*)malloc(WINDOW_BYTES);

    if (pWindow == 0)
    {
        return pWindow;
    }

    if (pFile->Read(&nSize, sizeof(nSize)) != sizeof(nSize))
    {
        return 0;
    }

    unsigned char* const pOut = (unsigned char*)malloc(nSize);

    if (pOut == 0)
    {
        free(pWindow);

        return 0;
    }

    const int nDecompressed = MDM1::LZSSDecompress(MDM1::ReadCallback, pFile, pOut, pWindow, WINDOW_BYTES);

    free(pWindow);

    if (nDecompressed != nSize)
    {
        free(pOut);

        return 0;
    }

    return pOut;
}

// FUNCTION: HYPERX 0x004401e0
int MDM1::ReadCallback(char* pBuffer, int nCount, FileObj* pFile)
{
    return pFile->Read(pBuffer, nCount);
}

// VC6 drops __declspec(naked) from a namespace-qualified definition.
namespace MDM1
{

// Hand-written assembly in the original: a naked routine that saves every register with PUSHAD,
// writes its answer into EAX's slot of the PUSHAD frame, and calls a helper that shares its EBP
// frame and passes the read position in ESI and the result in the flags.
// What it does: decompresses one LZSS stream from the read callback into pOut, through the
// sliding window, and returns the number of bytes written, or minus one if a read fails. EBX
// holds the flag word with a sentinel bit above it, so the shifts end when the sentinel falls
// out. A carry out of the long-form length ends the stream.
// Why assembly: C cannot share a frame with a callee, return through a PUSHAD slot, or branch
// on the flags a call returns.
// FUNCTION: HYPERX 0x00493f80
__declspec(naked) int LZSSDecompress(MDM1ReadFn pfnRead, FileObj* pFile, unsigned char* pOut, unsigned char* pWindow, int nWindowSize)
{
    __asm
    {
        push ebp
        mov ebp, esp
        mov eax, 0xffffffff
        pushad
        cld

        mov edi, pOut
        mov esi, pWindow
        lea edx, [esi - WINDOW_MARGIN]
        mov eax, nWindowSize
        add esi, eax
        add edx, eax
        xor ecx, ecx
        call RefillInputWindow
        jle done

    nextWord:
        cmp esi, edx
        jnc refill

    loadFlags:
        mov ebx, [esi]
        add esi, 2
        or ebx, FLAG_SENTINEL
        and ebx, FLAG_MASK
        shr ebx, 1
        jc match

    literal:
        mov al, [esi]
        inc esi
        mov [edi], al
        inc edi
        shr ebx, 1
        jc flagSet

        mov al, [esi]
        inc esi
        mov [edi], al
        inc edi
        shr ebx, 1
        jc flagSet

        mov al, [esi]
        inc esi
        mov [edi], al
        inc edi
        shr ebx, 1
        jc flagSet

        mov al, [esi]
        inc esi
        mov [edi], al
        inc edi
        shr ebx, 1
        jnc literal

    flagSet:
        jz nextWord

    match:
        mov eax, [esi]
        add esi, 2
        and eax, 0xffff
        mov cl, al
        shr eax, 4
        neg eax
        add eax, edi
        and cl, 0xf
        add cl, MATCH_LENGTH_BIAS
        cmp cl, MATCH_LENGTH_LONG
        jz longMatch

        push esi
        mov esi, eax
        rep movsb
        pop esi
        shr ebx, 1
        jc flagSet
        jmp literal

    refill:
        call RefillInputWindow
        jge loadFlags

    done:
        popad
        pop ebp
        ret

    longMatch:
        add cl, [esi]
        inc esi
        jc lengthCarry

        push esi
        mov esi, eax
        rep movsb
        pop esi

    nextFlag:
        shr ebx, 1
        jc flagSet
        jmp literal

    lengthCarry:
        cmp cl, MATCH_LENGTH_END
        jz nextFlag

        sub edi, pOut
        mov [esp + PUSHAD_EAX], edi
        popad
        pop ebp
        ret
    }
}

// Hand-written assembly in the original: a naked helper that reads its caller's arguments
// through the caller's EBP, takes and returns the read position in ESI, and returns the read
// callback's count in the flags with OR EAX, EAX.
// What it does: moves the unread tail of the window to its front, asks the read callback to
// fill the rest, and leaves ESI at the front of the window for the caller to read on.
// Why assembly: C cannot read another function's frame or pass values in ESI and the flags.
// FUNCTION: HYPERX 0x0049403a
__declspec(naked) void RefillInputWindow()
{
    __asm
    {
        push ebx
        push ecx
        push edx
        push edi

        mov ecx, [ebp + CALLER_WINDOW]
        mov edi, ecx
        add ecx, [ebp + CALLER_WINDOW_SIZE]
        push ecx
        sub ecx, esi
        mov eax, ecx
        shr ecx, 2
        rep movsd
        mov ecx, eax
        and ecx, 3
        rep movsb
        pop ecx

        mov esi, esp
        sub ecx, edi
        push [ebp + CALLER_FILE]
        push ecx
        push edi
        call [ebp + CALLER_READ_FN]
        mov esp, esi

        mov esi, [ebp + CALLER_WINDOW]
        or eax, eax

        pop edi
        pop edx
        pop ecx
        pop ebx
        ret
    }
}

} // namespace MDM1
