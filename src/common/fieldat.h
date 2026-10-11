// Asserts a field's offset at compile time, for a first field that sits on a base's boundary.

#ifndef HYPERBLADE_FIELDAT_H
#define HYPERBLADE_FIELDAT_H

#define HYPERBLADE_FIELD_AT(klass, member, offset) typedef char FieldAt_##klass##_##member[(int)(size_t)&((klass*)0)->member == (offset) ? 1 : -1]

#endif
