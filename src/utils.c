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
int power(int base, int exp) {
    if(exp < 0)
        return -1;

    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}