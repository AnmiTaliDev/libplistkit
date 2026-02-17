/**
 * @file utf8.c
 * @brief UTF-8 validation utilities.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"

bool
plistkit_utf8_validate(const char *str, size_t len)
{
    if (!str)
        return false;

    const unsigned char *p = (const unsigned char *)str;
    const unsigned char *end = p + len;

    while (p < end) {
        uint32_t cp;
        size_t   expect;

        if (p[0] < 0x80) {
            /* ASCII */
            p++;
            continue;
        } else if ((p[0] & 0xE0) == 0xC0) {
            expect = 2;
            cp = p[0] & 0x1F;
        } else if ((p[0] & 0xF0) == 0xE0) {
            expect = 3;
            cp = p[0] & 0x0F;
        } else if ((p[0] & 0xF8) == 0xF0) {
            expect = 4;
            cp = p[0] & 0x07;
        } else {
            return false; /* invalid lead byte */
        }

        if (p + expect > end)
            return false; /* truncated sequence */

        for (size_t i = 1; i < expect; i++) {
            if ((p[i] & 0xC0) != 0x80)
                return false; /* invalid continuation byte */
            cp = (cp << 6) | (p[i] & 0x3F);
        }

        /* reject overlong encodings */
        if (expect == 2 && cp < 0x80)   return false;
        if (expect == 3 && cp < 0x800)  return false;
        if (expect == 4 && cp < 0x10000) return false;

        /* reject surrogates and values beyond Unicode range */
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;
        if (cp > 0x10FFFF) return false;

        p += expect;
    }

    return true;
}

size_t
plistkit_utf8_codepoint_count(const char *str, size_t len)
{
    if (!str)
        return 0;

    const unsigned char *p = (const unsigned char *)str;
    const unsigned char *end = p + len;
    size_t count = 0;

    while (p < end) {
        if (p[0] < 0x80)             p += 1;
        else if ((p[0] & 0xE0) == 0xC0) p += 2;
        else if ((p[0] & 0xF0) == 0xE0) p += 3;
        else if ((p[0] & 0xF8) == 0xF0) p += 4;
        else                         p += 1; /* skip invalid */

        count++;
    }

    return count;
}
