///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      util.c
///  @brief     Common utilities
///  @author    Yi Tong Tse


#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "util.h"


size_t strlcpy (char *dst, const char *src, size_t size)
{
    size_t src_len = strlen(src);
    size_t cpy_len = src_len;
    if (cpy_len >= size)  cpy_len = size - 1;
    strncpy(dst, src, cpy_len);
    dst[cpy_len] = 0;           // NUL terminate dst
    return src_len;
}


int put_format (char **ptr, const char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsprintf(*ptr, format, arg_list);
    va_end(arg_list);
    *ptr += len;
    return len;
}


void get_byte (parser_t *psr, uint8 *data)
{
    PARSER_CHECK(psr, 1);
    *data = *(psr->ptr)++;
}


void get_short (parser_t *psr, uint16 *data)
{
    PARSER_CHECK(psr, 2);
    *data = *(psr->ptr)++ << 8; 
    *data |= *(psr->ptr)++;
}


void get_long (parser_t *psr, uint32 *data)
{
    PARSER_CHECK(psr, 4);
    *data = *(psr->ptr)++ << 24;
    *data |= *(psr->ptr)++ << 16;
    *data |= *(psr->ptr)++ << 8;
    *data |= *(psr->ptr)++;
}


void get_string (parser_t *psr, int len, char *data)
{
    PARSER_CHECK(psr, len);
    memcpy(data, psr->ptr, len);
    psr->ptr += len;
}


int str_lookup (const char *item, const char *choice[])
{
    int i = 0;
    while (strlen(choice[i]) > 0) {
        if (!strcmp(item, choice[i])) {
            return i;
        }
        i++;
    }
    return -1;
}


void print_hex (const void* ptr, int nbytes)
{
    int i;
    const uint8 *temp = ptr;

    for (i=0; nbytes>0; nbytes--) {
        printf("%02x ", *temp++);
        if (++i==16) {
            i = 0;
            printf("\n");
        }
    }
    if (i) {
        printf("\n");
    }
}
