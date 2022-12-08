#ifndef MACRO_H
#define MACRO_H

#include <cassert>
#include <iostream>

#define UNREACHABLE(msg) std::cerr <<"#ERROR: " << msg << std::endl; abort(); __builtin_unreachable()
#define ASSERT(x) assert(x)

#endif  // MACRO_H