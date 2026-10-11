// The DirectX 5 SDK's execute-buffer macros, which VC6's own headers do not carry.

#ifndef HYPERBLADE_D3DMACS_H
#define HYPERBLADE_D3DMACS_H

#include <d3d.h>

#define PUTD3DINSTRUCTION(op, sz, cnt, ptr)     \
    ((LPD3DINSTRUCTION)(ptr))->bOpcode = (op);  \
    ((LPD3DINSTRUCTION)(ptr))->bSize   = (sz);  \
    ((LPD3DINSTRUCTION)(ptr))->wCount  = (cnt); \
    (ptr) += sizeof(D3DINSTRUCTION)

#define OP_STATE_RENDER(cnt, ptr) PUTD3DINSTRUCTION(D3DOP_STATERENDER, sizeof(D3DSTATE), cnt, ptr)

#define STATE_DATA(type, arg, ptr)                     \
    ((LPD3DSTATE)(ptr))->drstRenderStateType = (type); \
    ((LPD3DSTATE)(ptr))->dwArg[0]            = (arg);  \
    (ptr) += sizeof(D3DSTATE)

#define OP_PROCESS_VERTICES(cnt, ptr) \
    PUTD3DINSTRUCTION(D3DOP_PROCESSVERTICES, sizeof(D3DPROCESSVERTICES), cnt, ptr)

#define PROCESSVERTICES_DATA(flgs, strt, cnt, ptr)      \
    ((LPD3DPROCESSVERTICES)(ptr))->dwFlags    = (flgs); \
    ((LPD3DPROCESSVERTICES)(ptr))->wStart     = (strt); \
    ((LPD3DPROCESSVERTICES)(ptr))->wDest      = (strt); \
    ((LPD3DPROCESSVERTICES)(ptr))->dwCount    = (cnt);  \
    ((LPD3DPROCESSVERTICES)(ptr))->dwReserved = 0;      \
    (ptr) += sizeof(D3DPROCESSVERTICES)

#define OP_TRIANGLE_LIST(cnt, ptr) PUTD3DINSTRUCTION(D3DOP_TRIANGLE, sizeof(D3DTRIANGLE), cnt, ptr)

#define OP_EXIT(ptr) PUTD3DINSTRUCTION(D3DOP_EXIT, 0, 0, ptr)

#define QWORD_ALIGNED(ptr) (((unsigned int)(ptr) & 7) == 0)

#endif
