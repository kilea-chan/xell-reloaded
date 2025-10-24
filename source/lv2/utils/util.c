#include <string.h>

char *strip(char *buf)
{
    while (*buf == ' ' || *buf == '\t')
        buf++;
    char *end = buf + strlen(buf) - 1;
    while (*end == ' ' || *end == '\t')
        *end-- = 0;

    return buf;
}

void split(char *buf, char **left, char **right, char delim)
{
    char *p = strchr(buf, delim);

    if (p) {
        *p = 0;
        *left = strip(buf);
        *right = strip(p+1);
    } else {
        *left = strip(buf);
        *right = NULL;
    }
}
