#ifndef HYPERBLADE_FLOATBITS_H
#define HYPERBLADE_FLOATBITS_H

// A float's bit pattern, for the encoders and rounding tricks that read it.
inline unsigned int FloatBits(const float& flValue)
{
    return *reinterpret_cast<const unsigned int*>(&flValue);
}

#endif
