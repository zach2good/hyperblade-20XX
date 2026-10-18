// A file record read whole into one block and used through its header struct, whose length the
// file gives rather than the type.

#ifndef HYPERBLADE_RECORD_H
#define HYPERBLADE_RECORD_H

#include <cstddef>
#include <memory>

template <class T>
struct RecordDeleter
{
    void operator()(T* pRecord) const noexcept
    {
        const auto bytes = std::unique_ptr<std::byte[]>(reinterpret_cast<std::byte*>(pRecord));
    }
};

template <class T>
using Record = std::unique_ptr<T, RecordDeleter<T> >;

// Zeroed, as the loaders read into it and expect any gap to be clear.
template <class T>
Record<T> MakeRecord(std::size_t nBytes)
{
    return Record<T>(reinterpret_cast<T*>(std::make_unique<std::byte[]>(nBytes).release()));
}

#endif
