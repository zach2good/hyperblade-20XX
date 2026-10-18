#ifndef HYPERBLADE_POOLEDOWNERROOT_H
#define HYPERBLADE_POOLEDOWNERROOT_H

// The pool owner whose five slots the agenda item, behaviour and interactor pools share.
class PooledOwnerRoot
{
public:
    virtual ~PooledOwnerRoot() = default;

    // Answers what the holding pointer should hold afterwards; every caller assigns it straight back.
    virtual PooledOwnerRoot* Destroy();

    virtual void Init() = 0;

    // void* because the three owners chain through different offsets on unrelated hierarchies.
    virtual char Insert(void* pNode) = 0;

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;
};

#endif
