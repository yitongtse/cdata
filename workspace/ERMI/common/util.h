///  Copyright (c) 2005-2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      util.h
///  @brief     Header file for common utilities


#ifndef __UTIL_H__
#define __UTIL_H__


#include <string.h>
#include "common.h"


/// Parser error codes
#define PARSER_OK                       0
#define PARSER_ERR_EOF                  1
#define PARSER_ERR_BAD_SYNTAX_LEN       2
#define PARSER_ERR_UNKNOWN_SYNTAX       3
#define PARSER_ERR_SYNTAX_TOO_LONG      4


/// Parser
typedef struct {
    const uint8 *ptr;   // read pointer
    int len;            // remaining length in buffer
    int err;            // parser error code
} parser_t;


/// Message parser check utility
#define PARSER_CHECK(psr, data_len)             \
    if ((psr)->err)  return;                    \
    (psr)->len -= (data_len);                   \
    if ((psr)->len < 0) {                       \
        (psr)->err = PARSER_ERR_EOF;            \
        return;                                 \
    }


/// Skip over a 2-byte short
static inline
char* skip_short (char **buf)
{
    char* buf2 = *buf;
    *buf += 2;
    return buf2;
}


/// Skip over a 4-byte long
static inline
char* skip_long (char **buf)
{
    char* buf2 = *buf;
    *buf += 4;
    return buf2;
}


/// Put a byte, and advance the write pointer
static inline
int put_byte (char **buf, uint8 data)
{
    *(*buf)++ = data;
    return 1;
}


/// Put a 2-byte short, and advance the write pointer
static inline
int put_short (char **buf, uint16 data)
{
    *(*buf)++ = (data >> 8) & 0xFF;
    *(*buf)++ = data & 0xFF;
    return 2;
}


/// Put a 4-byte long, and advance the write pointer
static inline
int put_long (char **buf, uint32 data)
{
    *(*buf)++ = (data >> 24) & 0xFF;
    *(*buf)++ = (data >> 16) & 0xFF;
    *(*buf)++ = (data >> 8) & 0xFF;
    *(*buf)++ = data & 0xFF;
    return 4;
}


/// Put a string, and advance the write pointer
/// @note
///   - the string in buf will NOT be null terminated
static inline
int put_string (char **buf, const char *data)
{
    int len = strlen(data);
    memcpy(*buf, data, len);
    *buf += len;
    return len;
}


/// Write data field with specified length
/// @return Number of bytes written
static inline
int put_data (char **buf, const char *data, int len)
{
    memcpy(*buf, data, len);
    *buf += len;
    return len;
}


/// Write formatted data
int put_format(char **ptr, const char *format, ...)
                __attribute__ ((format ( __printf__, 2, 3)));



/// Get a byte
void get_byte(parser_t *psr, uint8 *data);


/// Get a 16-bit short, and advance the read pointer
void get_short(parser_t *psr, uint16 *data);


/// Get a 32-bit long, and advance the read pointer
void get_long(parser_t *psr, uint32 *data);


/// Get a string with specified length, and advance the read pointer
void get_string(parser_t *psr, int len, char *data);


/// Look up a string from an array of choices
/// @param item    Item to be looked up
/// @param choice  Array of choices, terminated by an empty string
/// @return        Index of matching choice, or -1 if not matching found
int str_lookup(const char *item, const char *choice[]);

/// String copy
size_t strlcpy (char *dst, const char *src, size_t size);


/// Print memory in hex
void print_hex(const void* ptr, int nbytes);


#endif  // __UTIL_H__
