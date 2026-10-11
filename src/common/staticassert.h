// A compile-time assertion for VC6, which has no static_assert; two may not share a line.

#ifndef HYPERBLADE_STATICASSERT_H
#define HYPERBLADE_STATICASSERT_H

#define STATIC_ASSERT_NAME2(prefix, line) prefix##line
#define STATIC_ASSERT_NAME(prefix, line)  STATIC_ASSERT_NAME2(prefix, line)

#define STATIC_ASSERT(cond) \
    typedef char STATIC_ASSERT_NAME(StaticAssertAt, __LINE__)[(cond) ? 1 : -1]

#endif
