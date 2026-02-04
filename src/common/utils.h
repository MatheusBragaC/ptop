#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <string.h>

#define APPEND_LIT(buf, str) (memcpy(buf, str, sizeof(str) - 1), buf + sizeof(str) - 1)

static inline char *append_int(char *buf, int val)
{
    char temp[16];
    char *p = temp;

    do
    {
        *p++ = (val % 10) + '0';
        val /= 10;
    } while (val > 0);

    while (temp < p)
    {
        *buf++ = *--p;
    }

    return buf;
}

static inline char *append_str(char *buf, const char *str)
{
    while (*str)
        *buf++ = *str++;
    return buf;
}

#endif
