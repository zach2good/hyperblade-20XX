// A file that closes itself.

#ifndef HYPERBLADE_OWNEDFILE_H
#define HYPERBLADE_OWNEDFILE_H
#include "common/compat.h"
#include "common/fileobj.h"

// FileObj has no destructor, because GDI::ReadBitmapFile closes its own file on every path.
struct OwnedFile
{
    ~OwnedFile()
    {
        m_file.Close();
    }

    FileObj m_file;
};

#endif
