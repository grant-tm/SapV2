#include "..\inc\SystemUtilities.h"

void panicf [[noreturn]] (const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    std::exit(EXIT_FAILURE);
}

void quit (void) {
    std::exit(EXIT_SUCCESS);
}