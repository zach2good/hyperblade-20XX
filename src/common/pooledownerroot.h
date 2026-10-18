#ifndef HYPERBLADE_POOLEDOWNERROOT_H
#define HYPERBLADE_POOLEDOWNERROOT_H

// The pool owner whose slots the agenda item, behaviour and interactor pools share.
class PooledOwnerRoot
{
public:
    PooledOwnerRoot()                                  = default;
    PooledOwnerRoot(const PooledOwnerRoot&)            = delete;
    PooledOwnerRoot(PooledOwnerRoot&&)                 = delete;
    PooledOwnerRoot& operator=(const PooledOwnerRoot&) = delete;
    PooledOwnerRoot& operator=(PooledOwnerRoot&&)      = delete;
    virtual ~PooledOwnerRoot()                         = default;

    virtual void Init() = 0;

    // void* because the three owners chain through different offsets on unrelated hierarchies.
    virtual char Insert(void* pNode) = 0;

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;
};

#endif
