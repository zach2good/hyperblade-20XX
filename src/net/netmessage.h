#ifndef HYPERBLADE_NETMESSAGE_H
#define HYPERBLADE_NETMESSAGE_H

#pragma pack(push, 1)

// Every game message opens with its kind; the kind decides the rest.
struct NetMessage
{
    unsigned char nKind; // 0x00
};

#pragma pack(pop)

#endif
