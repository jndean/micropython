#ifndef _IPUSTRING_H
#define _IPUSTRING_H

typedef unsigned int size_t;


int strncmp (const char * str1, const char * str2, size_t num) {
    for (size_t i = 0; i < num; ++i) {
        if (str1[i] != str2[i]) return str1[i] - str2[i];
        if (str1[i] == '\0') break;
    }
    return 0;
}

#endif // _IPUSTRING_H