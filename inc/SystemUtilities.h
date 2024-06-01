#ifndef SYSTEM_UTILITIES_H
#define SYSTEM_UTILITIES_H

// Standard Library Inclusions
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

void panicf [[noreturn]] (const char* msg, ...);

void quit (void);

#endif // SYSTEM_UTILITIES_H