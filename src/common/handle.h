// An opaque pointer to an object of a library a header must not name. Only the module that made
// the handle and the modules that use it know the type, and they say so at the cast.

#ifndef HYPERBLADE_HANDLE_H
#define HYPERBLADE_HANDLE_H

// The tag keeps handles of different kinds from mixing.
template <class Tag>
class Handle
{
public:
    Handle() = default;

    explicit Handle(void* pObject)
    : m_pObject(pObject)
    {
    }

    template <class T>
    T* As() const
    {
        return static_cast<T*>(m_pObject);
    }

    explicit operator bool() const
    {
        return m_pObject != nullptr;
    }

private:
    void* m_pObject = nullptr;
};

#endif
