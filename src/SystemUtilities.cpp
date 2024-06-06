#include "SystemUtilities.h"

//-----------------------------------------------------------------------------
// errlog
// ----------------------------------------------------------------------------
// Prints a message to stderr
//-----------------------------------------------------------------------------
void 
errlog 
(
    const char *msg, 
    ...
){
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

//-----------------------------------------------------------------------------
// panicf
// ----------------------------------------------------------------------------
// Prints a message to stderr and exits the program with EXIT_FAILURE code.
//-----------------------------------------------------------------------------
void
panicf
(
    const char* msg,
    ...
){
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    std::exit(EXIT_FAILURE);
}

//-----------------------------------------------------------------------------
// quit
// ----------------------------------------------------------------------------
// Exits the program with EXIT_SUCCESS code.
//-----------------------------------------------------------------------------
void
quit
(
    void
){
    std::exit(EXIT_SUCCESS);
}