#ifndef HYPERBLADE_RECORDOFFSET_H
#define HYPERBLADE_RECORDOFFSET_H

// A file record's byte offset to a T, counted from the start of the record that holds it.
template <class T>
struct RecordOffset
{
    short nBytes;

    T* In(const void* pRecord) const
    {
        return (T*)(static_cast<const char*>(pRecord) + nBytes);
    }
};

#endif
