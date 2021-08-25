//
// Created by zhijun on 2021/8/24.
//

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifndef SURFELMAPPING_SYSTEM_H
#define SURFELMAPPING_SYSTEM_H

static void _mkdir(const char *dir)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    mkdir(tmp, 0755);
}

#endif //SURFELMAPPING_SYSTEM_H
