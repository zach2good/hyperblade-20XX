// The size, then one row pointer per row into the runs behind.

#ifndef HYPERBLADE_RLESPRITE_H
#define HYPERBLADE_RLESPRITE_H

struct RLESprite
{
    int   cx;        // 0x00
    int   cy;        // 0x04
    char* apRows[1]; // 0x08, cy long
};

#endif
