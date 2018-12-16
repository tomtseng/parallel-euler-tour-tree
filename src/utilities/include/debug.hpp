#pragma once

// #define DEBUG

// If DEBUG is defined, then if we have variables x, y, and z, writing
//   TRACE(x _ y _ z)
// prints out "x _ y _ z = <x printed> _ <y printed> _ <z printed>\n" to stderr.
#ifdef DEBUG
#include <cassert>
#include <iostream>
#define TRACE(x) std::cerr << #x << " = " << x << std::endl;
#define _ << " _ " <<
#else
#define TRACE(x) ((void)0)
#endif
