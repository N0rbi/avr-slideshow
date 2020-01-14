#include "utils.h"

#ifndef NO_AVR
int strlen(const char *str)
{
    int len = 0;

    while (*str++)
        len++;

    return len;
}

#endif
