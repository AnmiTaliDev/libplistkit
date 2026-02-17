/**
 * @file plist.c
 * @brief Core plist node operations: creation, destruction, copy, getters, setters.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"
#include "version.h"

#include <math.h>
#include <float.h>

/* ========================================================================== */
/* Helpers                                                                     */
/* ========================================================================== */

static plistkit_node_t *
node_alloc(plistkit_type_t type)
{
    plistkit_node_t *n = (plistkit_node_t *)plistkit_calloc(1, sizeof(*n));
    if (n)
        n->type = type;
    return n;
}

/** Free the value payload of a node (not the node itself, not children). */
static void
node_free_value(plistkit_node_t *node)
{
    switch (node->type) {
    case PLISTKIT_TYPE_STRING:
        free(node->value.string_val.ptr);
        node->value.string_val.ptr = NULL;
        node->value.string_val.length = 0;
        break;
    case PLISTKIT_TYPE_DATA:
        free(node->value.data_val.ptr);
        node->value.data_val.ptr = NULL;
        node->value.data_val.size = 0;
        break;
    case PLISTKIT_TYPE_ARRAY:
        /* items freed by plistkit_node_free recursion */
        free(node->value.array_val.items);
        node->value.array_val.items = NULL;
        break;
    case PLISTKIT_TYPE_DICT:
        /* keys and values freed by plistkit_node_free recursion */
        for (size_t i = 0; i < node->value.dict_val.count; i++)
            free(node->value.dict_val.keys[i]);
        free(node->value.dict_val.keys);
        free(node->value.dict_val.values);
        node->value.dict_val.keys = NULL;
        node->value.dict_val.values = NULL;
        break;
    default:
        break;
    }
}

/* ========================================================================== */
/* Version                                                                     */
/* ========================================================================== */

const char *
plistkit_version(void)
{
    return PLISTKIT_VERSION_STRING;
}

void
plistkit_version_get(int *major, int *minor, int *patch)
{
    if (major) *major = PLISTKIT_VERSION_MAJOR;
    if (minor) *minor = PLISTKIT_VERSION_MINOR;
    if (patch) *patch = PLISTKIT_VERSION_PATCH;
}

/* ========================================================================== */
/* Config defaults                                                             */
/* ========================================================================== */

plistkit_config_t
plistkit_config_default(void)
{
    plistkit_config_t c;
    memset(&c, 0, sizeof(c));
    c.max_file_size     = 100u * 1024u * 1024u;  /* 100 MB */
    c.max_depth         = 100;
    c.max_string_length = 10u * 1024u * 1024u;   /* 10 MB */
    c.max_data_size     = 100u * 1024u * 1024u;   /* 100 MB */
    c.max_array_items   = 1000000;
    c.max_dict_entries  = 1000000;
    c.flags             = 0;
    return c;
}

plistkit_writer_opts_t
plistkit_writer_opts_default(void)
{
    plistkit_writer_opts_t o;
    memset(&o, 0, sizeof(o));
    o.pretty_print    = true;
    o.indent_size     = 2;
    o.xml_declaration = true;
    o.doctype         = true;
    return o;
}

/* ========================================================================== */
/* Node creation                                                               */
/* ========================================================================== */

plistkit_node_t *
plistkit_node_new_boolean(bool value)
{
    plistkit_node_t *n = node_alloc(PLISTKIT_TYPE_BOOLEAN);
    if (n)
        n->value.boolean_val = value;
    return n;
}

plistkit_node_t *
plistkit_node_new_integer(int64_t value)
{
    plistkit_node_t *n = node_alloc(PLISTKIT_TYPE_INTEGER);
    if (n)
        n->value.integer_val = value;
    return n;
}

plistkit_node_t *
plistkit_node_new_real(double value)
{
    plistkit_node_t *n = node_alloc(PLISTKIT_TYPE_REAL);
    if (n)
        n->value.real_val = value;
    return n;
}

plistkit_node_t *
plistkit_node_new_string(const char *value)
{
    if (!value)
        return NULL;

    plistkit_node_t *n = node_alloc(PLISTKIT_TYPE_STRING);
    if (!n)
        return NULL;

    n->value.string_val.length = strlen(value);
    n->value.string_val.ptr = plistkit_strdup(value);
    if (!n->value.string_val.ptr) {
        free(n);
        return NULL;
    }
    return n;
}

plistkit_node_t *
plistkit_node_new_data(const void *data, size_t size)
{
    if (!data && size > 0)
        return NULL;

    plistkit_node_t *n = node_alloc(PLISTKIT_TYPE_DATA);
    if (!n)
        return NULL;

    if (size > 0) {
        n->value.data_val.ptr = (uint8_t *)malloc(size);
        if (!n->value.data_val.ptr) {
            free(n);
            return NULL;
        }
        memcpy(n->value.data_val.ptr, data, size);
    }
    n->value.data_val.size = size;
    return n;
}

plistkit_node_t *
plistkit_node_new_date(int64_t timestamp)
{
    plistkit_node_t *n = node_alloc(PLISTKIT_TYPE_DATE);
    if (n)
        n->value.date_val.timestamp = timestamp;
    return n;
}

plistkit_node_t *
plistkit_node_new_array(void)
{
    return node_alloc(PLISTKIT_TYPE_ARRAY);
}

plistkit_node_t *
plistkit_node_new_dict(void)
{
    return node_alloc(PLISTKIT_TYPE_DICT);
}

/* ========================================================================== */
/* Node destruction                                                            */
/* ========================================================================== */

void
plistkit_node_free(plistkit_node_t *node)
{
    if (!node)
        return;

    if (node->type == PLISTKIT_TYPE_ARRAY) {
        for (size_t i = 0; i < node->value.array_val.count; i++)
            plistkit_node_free(node->value.array_val.items[i]);
    } else if (node->type == PLISTKIT_TYPE_DICT) {
        for (size_t i = 0; i < node->value.dict_val.count; i++)
            plistkit_node_free(node->value.dict_val.values[i]);
    }

    node_free_value(node);
    free(node);
}

/* ========================================================================== */
/* Deep copy                                                                   */
/* ========================================================================== */

plistkit_node_t *
plistkit_node_copy(const plistkit_node_t *node)
{
    if (!node)
        return NULL;

    plistkit_node_t *copy = NULL;

    switch (node->type) {
    case PLISTKIT_TYPE_BOOLEAN:
        copy = plistkit_node_new_boolean(node->value.boolean_val);
        break;
    case PLISTKIT_TYPE_INTEGER:
        copy = plistkit_node_new_integer(node->value.integer_val);
        break;
    case PLISTKIT_TYPE_REAL:
        copy = plistkit_node_new_real(node->value.real_val);
        break;
    case PLISTKIT_TYPE_STRING:
        copy = plistkit_node_new_string(node->value.string_val.ptr);
        break;
    case PLISTKIT_TYPE_DATA:
        copy = plistkit_node_new_data(node->value.data_val.ptr,
                                      node->value.data_val.size);
        break;
    case PLISTKIT_TYPE_DATE:
        copy = plistkit_node_new_date(node->value.date_val.timestamp);
        break;
    case PLISTKIT_TYPE_ARRAY:
        copy = plistkit_node_new_array();
        if (!copy) return NULL;
        for (size_t i = 0; i < node->value.array_val.count; i++) {
            plistkit_node_t *item_copy =
                plistkit_node_copy(node->value.array_val.items[i]);
            if (!item_copy) {
                plistkit_node_free(copy);
                return NULL;
            }
            if (plistkit_array_append(copy, item_copy) != PLISTKIT_OK) {
                plistkit_node_free(item_copy);
                plistkit_node_free(copy);
                return NULL;
            }
        }
        break;
    case PLISTKIT_TYPE_DICT:
        copy = plistkit_node_new_dict();
        if (!copy) return NULL;
        for (size_t i = 0; i < node->value.dict_val.count; i++) {
            plistkit_node_t *val_copy =
                plistkit_node_copy(node->value.dict_val.values[i]);
            if (!val_copy) {
                plistkit_node_free(copy);
                return NULL;
            }
            if (plistkit_dict_set_value(copy, node->value.dict_val.keys[i],
                                        val_copy) != PLISTKIT_OK) {
                plistkit_node_free(val_copy);
                plistkit_node_free(copy);
                return NULL;
            }
        }
        break;
    default:
        return NULL;
    }

    return copy;
}

/* ========================================================================== */
/* Introspection                                                               */
/* ========================================================================== */

plistkit_type_t
plistkit_node_get_type(const plistkit_node_t *node)
{
    if (!node)
        return PLISTKIT_TYPE_NONE;
    return node->type;
}

bool
plistkit_node_is_type(const plistkit_node_t *node, plistkit_type_t type)
{
    if (!node)
        return false;
    return node->type == type;
}

bool
plistkit_node_equal(const plistkit_node_t *a, const plistkit_node_t *b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    if (a->type != b->type)
        return false;

    switch (a->type) {
    case PLISTKIT_TYPE_BOOLEAN:
        return a->value.boolean_val == b->value.boolean_val;
    case PLISTKIT_TYPE_INTEGER:
        return a->value.integer_val == b->value.integer_val;
    case PLISTKIT_TYPE_REAL:
        return fabs(a->value.real_val - b->value.real_val) < DBL_EPSILON;
    case PLISTKIT_TYPE_STRING:
        return a->value.string_val.length == b->value.string_val.length &&
               strcmp(a->value.string_val.ptr, b->value.string_val.ptr) == 0;
    case PLISTKIT_TYPE_DATA:
        return a->value.data_val.size == b->value.data_val.size &&
               (a->value.data_val.size == 0 ||
                memcmp(a->value.data_val.ptr, b->value.data_val.ptr,
                       a->value.data_val.size) == 0);
    case PLISTKIT_TYPE_DATE:
        return a->value.date_val.timestamp == b->value.date_val.timestamp;
    case PLISTKIT_TYPE_ARRAY:
        if (a->value.array_val.count != b->value.array_val.count)
            return false;
        for (size_t i = 0; i < a->value.array_val.count; i++) {
            if (!plistkit_node_equal(a->value.array_val.items[i],
                                     b->value.array_val.items[i]))
                return false;
        }
        return true;
    case PLISTKIT_TYPE_DICT:
        if (a->value.dict_val.count != b->value.dict_val.count)
            return false;
        for (size_t i = 0; i < a->value.dict_val.count; i++) {
            /* find key in b */
            bool found = false;
            for (size_t j = 0; j < b->value.dict_val.count; j++) {
                if (strcmp(a->value.dict_val.keys[i],
                           b->value.dict_val.keys[j]) == 0) {
                    if (!plistkit_node_equal(a->value.dict_val.values[i],
                                             b->value.dict_val.values[j]))
                        return false;
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    default:
        return false;
    }
}

/* ========================================================================== */
/* Getters                                                                     */
/* ========================================================================== */

plistkit_error_t
plistkit_node_get_boolean(const plistkit_node_t *node, bool *out)
{
    if (!node || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_BOOLEAN) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = node->value.boolean_val;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_get_integer(const plistkit_node_t *node, int64_t *out)
{
    if (!node || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_INTEGER) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = node->value.integer_val;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_get_real(const plistkit_node_t *node, double *out)
{
    if (!node || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_REAL) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = node->value.real_val;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_get_string(const plistkit_node_t *node, const char **out)
{
    if (!node || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_STRING) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = node->value.string_val.ptr;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_get_data(const plistkit_node_t *node,
                       const void **out, size_t *size)
{
    if (!node || !out || !size) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_DATA) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = node->value.data_val.ptr;
    *size = node->value.data_val.size;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_get_date(const plistkit_node_t *node, int64_t *out)
{
    if (!node || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_DATE) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = node->value.date_val.timestamp;
    return PLISTKIT_OK;
}

/* ========================================================================== */
/* Setters                                                                     */
/* ========================================================================== */

plistkit_error_t
plistkit_node_set_boolean(plistkit_node_t *node, bool value)
{
    if (!node) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_BOOLEAN) return PLISTKIT_ERR_TYPE_MISMATCH;
    node->value.boolean_val = value;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_set_integer(plistkit_node_t *node, int64_t value)
{
    if (!node) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_INTEGER) return PLISTKIT_ERR_TYPE_MISMATCH;
    node->value.integer_val = value;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_set_real(plistkit_node_t *node, double value)
{
    if (!node) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_REAL) return PLISTKIT_ERR_TYPE_MISMATCH;
    node->value.real_val = value;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_set_string(plistkit_node_t *node, const char *value)
{
    if (!node || !value) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_STRING) return PLISTKIT_ERR_TYPE_MISMATCH;

    char *dup = plistkit_strdup(value);
    if (!dup) return PLISTKIT_ERR_NOMEM;

    free(node->value.string_val.ptr);
    node->value.string_val.ptr = dup;
    node->value.string_val.length = strlen(value);
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_set_data(plistkit_node_t *node, const void *data, size_t size)
{
    if (!node) return PLISTKIT_ERR_INVALID_ARG;
    if (!data && size > 0) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_DATA) return PLISTKIT_ERR_TYPE_MISMATCH;

    uint8_t *buf = NULL;
    if (size > 0) {
        buf = (uint8_t *)malloc(size);
        if (!buf) return PLISTKIT_ERR_NOMEM;
        memcpy(buf, data, size);
    }

    free(node->value.data_val.ptr);
    node->value.data_val.ptr = buf;
    node->value.data_val.size = size;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_node_set_date(plistkit_node_t *node, int64_t timestamp)
{
    if (!node) return PLISTKIT_ERR_INVALID_ARG;
    if (node->type != PLISTKIT_TYPE_DATE) return PLISTKIT_ERR_TYPE_MISMATCH;
    node->value.date_val.timestamp = timestamp;
    return PLISTKIT_OK;
}

/* ========================================================================== */
/* Utility                                                                     */
/* ========================================================================== */

const char *
plistkit_error_string(plistkit_error_t error)
{
    switch (error) {
    case PLISTKIT_OK:                  return "Success";
    case PLISTKIT_ERR_NOMEM:           return "Out of memory";
    case PLISTKIT_ERR_INVALID_ARG:     return "Invalid argument";
    case PLISTKIT_ERR_PARSE:           return "Parse error";
    case PLISTKIT_ERR_IO:              return "I/O error";
    case PLISTKIT_ERR_TYPE_MISMATCH:   return "Type mismatch";
    case PLISTKIT_ERR_NOT_FOUND:       return "Not found";
    case PLISTKIT_ERR_LIMIT_EXCEEDED:  return "Limit exceeded";
    case PLISTKIT_ERR_OVERFLOW:        return "Overflow";
    case PLISTKIT_ERR_INDEX_OUT_OF_RANGE: return "Index out of range";
    case PLISTKIT_ERR_INVALID_UTF8:    return "Invalid UTF-8";
    case PLISTKIT_ERR_INVALID_BASE64:  return "Invalid base64";
    case PLISTKIT_ERR_INVALID_DATE:    return "Invalid date";
    case PLISTKIT_ERR_INVALID_XML:     return "Invalid XML";
    case PLISTKIT_ERR_UNSUPPORTED:     return "Unsupported";
    }
    return "Unknown error";
}

const char *
plistkit_type_name(plistkit_type_t type)
{
    switch (type) {
    case PLISTKIT_TYPE_NONE:    return "none";
    case PLISTKIT_TYPE_BOOLEAN: return "boolean";
    case PLISTKIT_TYPE_INTEGER: return "integer";
    case PLISTKIT_TYPE_REAL:    return "real";
    case PLISTKIT_TYPE_STRING:  return "string";
    case PLISTKIT_TYPE_DATA:    return "data";
    case PLISTKIT_TYPE_DATE:    return "date";
    case PLISTKIT_TYPE_ARRAY:   return "array";
    case PLISTKIT_TYPE_DICT:    return "dict";
    }
    return "unknown";
}
