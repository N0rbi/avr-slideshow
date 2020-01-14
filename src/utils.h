#ifndef UTILS_H
#define UTILS_H

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
int power(int base, int exp);


#ifdef NO_AVR
#include <string.h>
#else
int strlen(const char *message);


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
#endif

#endif // UTILS_H
