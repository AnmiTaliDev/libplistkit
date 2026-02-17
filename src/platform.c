/**
 * @file platform.c
 * @brief Platform abstraction layer (file I/O).
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"

#include <errno.h>

/* -------------------------------------------------------------------------- */
/* File reading                                                                */
/* -------------------------------------------------------------------------- */

plistkit_error_t
plistkit_file_read(const char *path, void **data, size_t *size)
{
    if (!path || !data || !size)
        return PLISTKIT_ERR_INVALID_ARG;

    *data = NULL;
    *size = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return PLISTKIT_ERR_IO;

    /* determine file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return PLISTKIT_ERR_IO;
    }

    long file_len = ftell(fp);
    if (file_len < 0) {
        fclose(fp);
        return PLISTKIT_ERR_IO;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return PLISTKIT_ERR_IO;
    }

    size_t fsize = (size_t)file_len;
    char *buf = (char *)malloc(fsize + 1);
    if (!buf) {
        fclose(fp);
        return PLISTKIT_ERR_NOMEM;
    }

    size_t nread = fread(buf, 1, fsize, fp);
    fclose(fp);

    if (nread != fsize) {
        free(buf);
        return PLISTKIT_ERR_IO;
    }

    buf[fsize] = '\0'; /* null-terminate for convenience */
    *data = buf;
    *size = fsize;
    return PLISTKIT_OK;
}

/* -------------------------------------------------------------------------- */
/* File writing                                                                */
/* -------------------------------------------------------------------------- */

plistkit_error_t
plistkit_file_write(const char *path, const void *data, size_t size)
{
    if (!path || (!data && size > 0))
        return PLISTKIT_ERR_INVALID_ARG;

    FILE *fp = fopen(path, "wb");
    if (!fp)
        return PLISTKIT_ERR_IO;

    if (size > 0) {
        size_t nwritten = fwrite(data, 1, size, fp);
        if (nwritten != size) {
            fclose(fp);
            return PLISTKIT_ERR_IO;
        }
    }

    if (fclose(fp) != 0)
        return PLISTKIT_ERR_IO;

    return PLISTKIT_OK;
}
