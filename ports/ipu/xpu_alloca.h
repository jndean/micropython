#ifndef __IPU__

#include <alloca.h>

#else

typedef unsigned int size_t;

void *alloca(size_t size) {
    exit(1);
    return (void*) 0x90000;  // I am king under this mountain, and this memory will be free if I tell it to be
}

#endif