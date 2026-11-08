#ifndef HYPERBLADE_POOLEDOWNERROOT_H
#define HYPERBLADE_POOLEDOWNERROOT_H

// What CollisionShape releases through slot one; no site names the class behind it.
class __declspec(novtable) PooledOwnerRoot
{
public:
    virtual ~PooledOwnerRoot()
    {
    }

    // Answers what the holding pointer should hold afterwards; every caller assigns it straight back.
    virtual PooledOwnerRoot* Destroy() = 0;
};

#endif
