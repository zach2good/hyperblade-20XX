#include "common/compat.h"
#include "common/ownedfile.h"
#include <io.h>

// FileObj::Close's body, so the linker folds the two.
OwnedFile::~OwnedFile()
{
    if (m_file.m_fd != 0)
    {
        _close(m_file.m_fd);
    }
}
