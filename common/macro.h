#ifndef MACRO_H
#define MACRO_H

#include <cassert>
#include <iostream>

extern "C" size_t line_no;

#define UNREACHABLE(msg) std::cerr <<"#ERROR(:" << line_no << "): " << msg << std::endl; abort(); __builtin_unreachable()
#define ASSERT(x) if(!(x)){ std::cerr <<"#lineno: (:" << line_no << "): " << std::endl; assert(x); }

#endif  // MACRO_H