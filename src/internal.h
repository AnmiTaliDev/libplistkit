/**
 * @file internal.h
 * @brief Private internal structures and utilities for libplistkit.
 *
 * This header is NOT part of the public API.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef PLISTKIT_INTERNAL_H
#define PLISTKIT_INTERNAL_H

#include "plistkit/plistkit.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================== */
/* Internal node structure                                                     */
/* ========================================================================== */

struct plistkit_node {
    plistkit_type_t type;

    union {
        bool boolean_val;
        int64_t integer_val;
        double real_val;

        struct {
            char  *ptr;
            size_t length; /* not including null terminator */
        } string_val;

        struct {
            uint8_t *ptr;
            size_t   size;
        } data_val;

        struct {
            int64_t timestamp; /* seconds since Unix epoch */
        } date_val;

        struct {
            plistkit_node_t **items;
            size_t count;
            size_t capacity;
        } array_val;

        struct {
            char            **keys;
            plistkit_node_t **values;
            size_t count;
            size_t capacity;
        } dict_val;
    } value;

    plistkit_node_t *parent;
};

/* ========================================================================== */
/* Memory helpers                                                              */
/* ========================================================================== */

static inline void *plistkit_malloc(size_t size)
{
    return malloc(size);
}

static inline void *plistkit_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

static inline void *plistkit_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

static inline void plistkit_free_ptr(void *ptr)
{
    free(ptr);
}

static inline char *plistkit_strdup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len + 1);
    }
    return dup;
}

static inline char *plistkit_strndup(const char *s, size_t n)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    if (len > n) len = n;
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}

/* ========================================================================== */
/* Parser context                                                              */
/* ========================================================================== */

typedef struct {
    const char *input;
    size_t      input_size;
    size_t      pos;
    int         line;
    int         column;

    plistkit_config_t config;

    /* stack for tracking nesting */
    plistkit_node_t **stack;
    size_t            stack_size;
    size_t            stack_capacity;

    /* current dict key being parsed */
    char *current_key;

    /* error output (may be NULL) */
    plistkit_error_ctx_t *error;

    /* current nesting depth */
    size_t depth;
} parser_ctx_t;

/* ========================================================================== */
/* Writer context                                                              */
/* ========================================================================== */

typedef struct {
    char  *buffer;
    size_t size;
    size_t capacity;
    int    indent_level;
    bool   failed; /* set on OOM to stop further writes */

    plistkit_writer_opts_t opts;
    plistkit_error_ctx_t  *error;
} writer_ctx_t;

/* ========================================================================== */
/* Base64                                                                      */
/* ========================================================================== */

size_t plistkit_base64_encoded_size(size_t binary_size);
size_t plistkit_base64_decoded_size(const char *encoded, size_t encoded_len);

plistkit_error_t plistkit_base64_encode(const uint8_t *src, size_t src_len,
                                        char *dest, size_t dest_capacity);

plistkit_error_t plistkit_base64_decode(const char *src, size_t src_len,
                                        uint8_t *dest, size_t dest_capacity,
                                        size_t *decoded_size);

/* ========================================================================== */
/* Date                                                                        */
/* ========================================================================== */

plistkit_error_t plistkit_date_parse(const char *str, int64_t *timestamp);
plistkit_error_t plistkit_date_format(int64_t timestamp, char *buffer,
                                      size_t buffer_size);

/* ========================================================================== */
/* XML helpers                                                                 */
/* ========================================================================== */

/** Escape XML special characters. Caller must free the result. */
char *plistkit_xml_escape(const char *str);

/** Unescape XML entities. Caller must free the result. */
char *plistkit_xml_unescape(const char *str, size_t len);

/* ========================================================================== */
/* UTF-8                                                                       */
/* ========================================================================== */

bool   plistkit_utf8_validate(const char *str, size_t len);
size_t plistkit_utf8_codepoint_count(const char *str, size_t len);

/* ========================================================================== */
/* Platform file I/O                                                           */
/* ========================================================================== */

plistkit_error_t plistkit_file_read(const char *path, void **data,
                                    size_t *size);
plistkit_error_t plistkit_file_write(const char *path, const void *data,
                                     size_t size);

#endif /* PLISTKIT_INTERNAL_H */
