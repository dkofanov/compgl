#ifndef MACRO_H
#define MACRO_H

#include <cassert>

#define UNREACHABLE() __builtin_unreachable()
#define ASSERT(x) assert(x)

#endif  // MACRO_H