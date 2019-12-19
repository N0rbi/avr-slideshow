#ifndef UTILS_H
#define UTILS_H

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
int power(int base, int exp);


#ifdef NO_AVR
#include <string.h>
#else
int strlen(const char *message);

#endif

#endif // UTILS_H

