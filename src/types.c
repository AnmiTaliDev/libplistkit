/**
 * @file types.c
 * @brief Type conversion utilities: base64, date, XML escaping.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#endif

#include "internal.h"

#include <ctype.h>
#include <time.h>

/* ========================================================================== */
/* Base64                                                                      */
/* ========================================================================== */

static const char b64_enc_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char b64_dec_table[256] = {
    ['A']=0,  ['B']=1,  ['C']=2,  ['D']=3,  ['E']=4,  ['F']=5,  ['G']=6,
    ['H']=7,  ['I']=8,  ['J']=9,  ['K']=10, ['L']=11, ['M']=12, ['N']=13,
    ['O']=14, ['P']=15, ['Q']=16, ['R']=17, ['S']=18, ['T']=19, ['U']=20,
    ['V']=21, ['W']=22, ['X']=23, ['Y']=24, ['Z']=25,
    ['a']=26, ['b']=27, ['c']=28, ['d']=29, ['e']=30, ['f']=31, ['g']=32,
    ['h']=33, ['i']=34, ['j']=35, ['k']=36, ['l']=37, ['m']=38, ['n']=39,
    ['o']=40, ['p']=41, ['q']=42, ['r']=43, ['s']=44, ['t']=45, ['u']=46,
    ['v']=47, ['w']=48, ['x']=49, ['y']=50, ['z']=51,
    ['0']=52, ['1']=53, ['2']=54, ['3']=55, ['4']=56, ['5']=57, ['6']=58,
    ['7']=59, ['8']=60, ['9']=61, ['+']=62, ['/']=63,
};

static bool b64_is_valid_char(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
}

size_t
plistkit_base64_encoded_size(size_t binary_size)
{
    return ((binary_size + 2) / 3) * 4;
}

size_t
plistkit_base64_decoded_size(const char *encoded, size_t encoded_len)
{
    if (!encoded || encoded_len == 0)
        return 0;

    /* skip whitespace to count actual base64 chars */
    size_t actual = 0;
    size_t padding = 0;
    for (size_t i = 0; i < encoded_len; i++) {
        unsigned char c = (unsigned char)encoded[i];
        if (isspace(c)) continue;
        if (c == '=') padding++;
        actual++;
    }
    if (actual == 0) return 0;
    return (actual / 4) * 3 - padding;
}

plistkit_error_t
plistkit_base64_encode(const uint8_t *src, size_t src_len,
                       char *dest, size_t dest_capacity)
{
    if (!dest)
        return PLISTKIT_ERR_INVALID_ARG;
    if (!src && src_len > 0)
        return PLISTKIT_ERR_INVALID_ARG;

    size_t needed = plistkit_base64_encoded_size(src_len) + 1;
    if (dest_capacity < needed)
        return PLISTKIT_ERR_OVERFLOW;

    size_t di = 0;
    size_t i;
    for (i = 0; i + 2 < src_len; i += 3) {
        uint32_t triple = ((uint32_t)src[i] << 16) |
                          ((uint32_t)src[i+1] << 8) |
                          ((uint32_t)src[i+2]);
        dest[di++] = b64_enc_table[(triple >> 18) & 0x3F];
        dest[di++] = b64_enc_table[(triple >> 12) & 0x3F];
        dest[di++] = b64_enc_table[(triple >> 6)  & 0x3F];
        dest[di++] = b64_enc_table[(triple)       & 0x3F];
    }

    size_t remaining = src_len - i;
    if (remaining == 1) {
        uint32_t val = (uint32_t)src[i] << 16;
        dest[di++] = b64_enc_table[(val >> 18) & 0x3F];
        dest[di++] = b64_enc_table[(val >> 12) & 0x3F];
        dest[di++] = '=';
        dest[di++] = '=';
    } else if (remaining == 2) {
        uint32_t val = ((uint32_t)src[i] << 16) | ((uint32_t)src[i+1] << 8);
        dest[di++] = b64_enc_table[(val >> 18) & 0x3F];
        dest[di++] = b64_enc_table[(val >> 12) & 0x3F];
        dest[di++] = b64_enc_table[(val >> 6)  & 0x3F];
        dest[di++] = '=';
    }

    dest[di] = '\0';
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_base64_decode(const char *src, size_t src_len,
                       uint8_t *dest, size_t dest_capacity,
                       size_t *decoded_size)
{
    if (!dest || !decoded_size)
        return PLISTKIT_ERR_INVALID_ARG;
    if (!src && src_len > 0)
        return PLISTKIT_ERR_INVALID_ARG;

    *decoded_size = 0;

    /* strip whitespace and validate, collect clean buffer */
    unsigned char *clean = (unsigned char *)malloc(src_len + 1);
    if (!clean)
        return PLISTKIT_ERR_NOMEM;

    size_t clean_len = 0;
    for (size_t i = 0; i < src_len; i++) {
        unsigned char c = (unsigned char)src[i];
        if (isspace(c)) continue;
        if (!b64_is_valid_char(c)) {
            free(clean);
            return PLISTKIT_ERR_INVALID_BASE64;
        }
        clean[clean_len++] = c;
    }

    if (clean_len == 0) {
        free(clean);
        *decoded_size = 0;
        return PLISTKIT_OK;
    }

    if (clean_len % 4 != 0) {
        free(clean);
        return PLISTKIT_ERR_INVALID_BASE64;
    }

    size_t out_len = (clean_len / 4) * 3;
    if (clean[clean_len - 1] == '=') out_len--;
    if (clean_len >= 2 && clean[clean_len - 2] == '=') out_len--;

    if (dest_capacity < out_len) {
        free(clean);
        return PLISTKIT_ERR_OVERFLOW;
    }

    size_t di = 0;
    for (size_t i = 0; i < clean_len; i += 4) {
        uint32_t a = b64_dec_table[clean[i]];
        uint32_t b = b64_dec_table[clean[i+1]];
        uint32_t c = (clean[i+2] == '=') ? 0 : b64_dec_table[clean[i+2]];
        uint32_t d = (clean[i+3] == '=') ? 0 : b64_dec_table[clean[i+3]];

        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

        if (di < out_len) dest[di++] = (uint8_t)((triple >> 16) & 0xFF);
        if (di < out_len) dest[di++] = (uint8_t)((triple >> 8)  & 0xFF);
        if (di < out_len) dest[di++] = (uint8_t)((triple)       & 0xFF);
    }

    free(clean);
    *decoded_size = out_len;
    return PLISTKIT_OK;
}

/* ========================================================================== */
/* Date                                                                        */
/* ========================================================================== */

plistkit_error_t
plistkit_date_parse(const char *str, int64_t *timestamp)
{
    if (!str || !timestamp)
        return PLISTKIT_ERR_INVALID_ARG;

    /* Expected format: YYYY-MM-DDTHH:MM:SSZ */
    int year, month, day, hour, min, sec;
    if (sscanf(str, "%d-%d-%dT%d:%d:%dZ",
               &year, &month, &day, &hour, &min, &sec) != 6)
        return PLISTKIT_ERR_INVALID_DATE;

    if (month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 60)
        return PLISTKIT_ERR_INVALID_DATE;

    struct tm tm_val;
    memset(&tm_val, 0, sizeof(tm_val));
    tm_val.tm_year = year - 1900;
    tm_val.tm_mon  = month - 1;
    tm_val.tm_mday = day;
    tm_val.tm_hour = hour;
    tm_val.tm_min  = min;
    tm_val.tm_sec  = sec;
    tm_val.tm_isdst = 0;

    /* Use timegm on POSIX or _mkgmtime on Windows */
#if defined(_WIN32)
    time_t t = _mkgmtime(&tm_val);
#else
    time_t t = timegm(&tm_val);
#endif

    if (t == (time_t)-1)
        return PLISTKIT_ERR_INVALID_DATE;

    *timestamp = (int64_t)t;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_date_format(int64_t timestamp, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size < 21)
        return PLISTKIT_ERR_INVALID_ARG;

    time_t t = (time_t)timestamp;
    struct tm tm_val;

#if defined(_WIN32)
    if (gmtime_s(&tm_val, &t) != 0)
        return PLISTKIT_ERR_OVERFLOW;
#else
    if (!gmtime_r(&t, &tm_val))
        return PLISTKIT_ERR_OVERFLOW;
#endif

    int n = snprintf(buffer, buffer_size, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                     tm_val.tm_year + 1900,
                     tm_val.tm_mon + 1,
                     tm_val.tm_mday,
                     tm_val.tm_hour,
                     tm_val.tm_min,
                     tm_val.tm_sec);

    if (n < 0 || (size_t)n >= buffer_size)
        return PLISTKIT_ERR_OVERFLOW;

    return PLISTKIT_OK;
}

/* ========================================================================== */
/* XML escape / unescape                                                       */
/* ========================================================================== */

char *
plistkit_xml_escape(const char *str)
{
    if (!str)
        return NULL;

    /* worst case: every char becomes &amp; (5x) */
    size_t len = strlen(str);
    size_t max_out = len * 6 + 1;
    char *out = (char *)malloc(max_out);
    if (!out)
        return NULL;

    size_t oi = 0;
    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
        case '&':
            memcpy(out + oi, "&amp;", 5);  oi += 5; break;
        case '<':
            memcpy(out + oi, "&lt;", 4);   oi += 4; break;
        case '>':
            memcpy(out + oi, "&gt;", 4);   oi += 4; break;
        case '"':
            memcpy(out + oi, "&quot;", 6);  oi += 6; break;
        case '\'':
            memcpy(out + oi, "&apos;", 6);  oi += 6; break;
        default:
            out[oi++] = str[i];
            break;
        }
    }
    out[oi] = '\0';
    return out;
}

char *
plistkit_xml_unescape(const char *str, size_t len)
{
    if (!str)
        return NULL;

    char *out = (char *)malloc(len + 1);
    if (!out)
        return NULL;

    size_t oi = 0;
    size_t i = 0;

    while (i < len) {
        if (str[i] == '&') {
            if (i + 3 < len && strncmp(str + i, "&lt;", 4) == 0) {
                out[oi++] = '<';  i += 4;
            } else if (i + 3 < len && strncmp(str + i, "&gt;", 4) == 0) {
                out[oi++] = '>';  i += 4;
            } else if (i + 4 < len && strncmp(str + i, "&amp;", 5) == 0) {
                out[oi++] = '&';  i += 5;
            } else if (i + 5 < len && strncmp(str + i, "&quot;", 6) == 0) {
                out[oi++] = '"';  i += 6;
            } else if (i + 5 < len && strncmp(str + i, "&apos;", 6) == 0) {
                out[oi++] = '\''; i += 6;
            } else if (i + 2 < len && str[i+1] == '#') {
                /* numeric character reference &#NN; or &#xHH; */
                size_t start = i + 2;
                uint32_t cp = 0;
                bool hex = false;

                if (start < len && str[start] == 'x') {
                    hex = true;
                    start++;
                }

                size_t j = start;
                while (j < len && str[j] != ';') {
                    if (hex) {
                        if (isxdigit((unsigned char)str[j]))
                            cp = cp * 16 + (isdigit((unsigned char)str[j])
                                ? (uint32_t)(str[j] - '0')
                                : (uint32_t)(tolower((unsigned char)str[j]) - 'a' + 10));
                        else break;
                    } else {
                        if (isdigit((unsigned char)str[j]))
                            cp = cp * 10 + (uint32_t)(str[j] - '0');
                        else break;
                    }
                    j++;
                }

                if (j < len && str[j] == ';' && cp <= 0x7F) {
                    out[oi++] = (char)cp;
                    i = j + 1;
                } else if (j < len && str[j] == ';' && cp <= 0x7FF) {
                    out[oi++] = (char)(0xC0 | (cp >> 6));
                    out[oi++] = (char)(0x80 | (cp & 0x3F));
                    i = j + 1;
                } else if (j < len && str[j] == ';' && cp <= 0xFFFF) {
                    out[oi++] = (char)(0xE0 | (cp >> 12));
                    out[oi++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    out[oi++] = (char)(0x80 | (cp & 0x3F));
                    i = j + 1;
                } else if (j < len && str[j] == ';' && cp <= 0x10FFFF) {
                    out[oi++] = (char)(0xF0 | (cp >> 18));
                    out[oi++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                    out[oi++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    out[oi++] = (char)(0x80 | (cp & 0x3F));
                    i = j + 1;
                } else {
                    /* invalid entity, copy literally */
                    out[oi++] = str[i++];
                }
            } else {
                /* unknown entity, copy literally */
                out[oi++] = str[i++];
            }
        } else {
            out[oi++] = str[i++];
        }
    }

    out[oi] = '\0';
    return out;
}
