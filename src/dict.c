/**
 * @file dict.c
 * @brief Dictionary node operations.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"

#define DICT_INIT_CAPACITY 4

static plistkit_error_t
dict_grow(plistkit_node_t *dict)
{
    size_t new_cap = dict->value.dict_val.capacity == 0
                         ? DICT_INIT_CAPACITY
                         : dict->value.dict_val.capacity * 2;

    char **new_keys = (char **)plistkit_realloc(
        dict->value.dict_val.keys, new_cap * sizeof(char *));
    if (!new_keys)
        return PLISTKIT_ERR_NOMEM;
    dict->value.dict_val.keys = new_keys;

    plistkit_node_t **new_vals = (plistkit_node_t **)plistkit_realloc(
        dict->value.dict_val.values, new_cap * sizeof(plistkit_node_t *));
    if (!new_vals)
        return PLISTKIT_ERR_NOMEM;
    dict->value.dict_val.values = new_vals;

    dict->value.dict_val.capacity = new_cap;
    return PLISTKIT_OK;
}

static int
dict_find(const plistkit_node_t *dict, const char *key)
{
    for (size_t i = 0; i < dict->value.dict_val.count; i++) {
        if (strcmp(dict->value.dict_val.keys[i], key) == 0)
            return (int)i;
    }
    return -1;
}

plistkit_error_t
plistkit_dict_get_size(const plistkit_node_t *dict, size_t *out)
{
    if (!dict || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (dict->type != PLISTKIT_TYPE_DICT) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = dict->value.dict_val.count;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_dict_get_value(const plistkit_node_t *dict, const char *key,
                        plistkit_node_t **out)
{
    if (!dict || !key || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (dict->type != PLISTKIT_TYPE_DICT) return PLISTKIT_ERR_TYPE_MISMATCH;

    int idx = dict_find(dict, key);
    if (idx < 0) return PLISTKIT_ERR_NOT_FOUND;

    *out = dict->value.dict_val.values[idx];
    return PLISTKIT_OK;
}

bool
plistkit_dict_has_key(const plistkit_node_t *dict, const char *key)
{
    if (!dict || !key || dict->type != PLISTKIT_TYPE_DICT)
        return false;
    return dict_find(dict, key) >= 0;
}

plistkit_error_t
plistkit_dict_set_value(plistkit_node_t *dict, const char *key,
                        plistkit_node_t *value)
{
    if (!dict || !key || !value) return PLISTKIT_ERR_INVALID_ARG;
    if (dict->type != PLISTKIT_TYPE_DICT) return PLISTKIT_ERR_TYPE_MISMATCH;

    /* check if key already exists */
    int idx = dict_find(dict, key);
    if (idx >= 0) {
        plistkit_node_free(dict->value.dict_val.values[idx]);
        dict->value.dict_val.values[idx] = value;
        value->parent = dict;
        return PLISTKIT_OK;
    }

    /* new key */
    if (dict->value.dict_val.count >= dict->value.dict_val.capacity) {
        plistkit_error_t err = dict_grow(dict);
        if (err != PLISTKIT_OK) return err;
    }

    char *key_dup = plistkit_strdup(key);
    if (!key_dup) return PLISTKIT_ERR_NOMEM;

    size_t i = dict->value.dict_val.count;
    dict->value.dict_val.keys[i] = key_dup;
    dict->value.dict_val.values[i] = value;
    dict->value.dict_val.count++;
    value->parent = dict;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_dict_remove_key(plistkit_node_t *dict, const char *key)
{
    if (!dict || !key) return PLISTKIT_ERR_INVALID_ARG;
    if (dict->type != PLISTKIT_TYPE_DICT) return PLISTKIT_ERR_TYPE_MISMATCH;

    int idx = dict_find(dict, key);
    if (idx < 0) return PLISTKIT_ERR_NOT_FOUND;

    free(dict->value.dict_val.keys[idx]);
    plistkit_node_free(dict->value.dict_val.values[idx]);

    /* shift elements left */
    for (size_t i = (size_t)idx; i + 1 < dict->value.dict_val.count; i++) {
        dict->value.dict_val.keys[i] = dict->value.dict_val.keys[i + 1];
        dict->value.dict_val.values[i] = dict->value.dict_val.values[i + 1];
    }

    dict->value.dict_val.count--;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_dict_clear(plistkit_node_t *dict)
{
    if (!dict) return PLISTKIT_ERR_INVALID_ARG;
    if (dict->type != PLISTKIT_TYPE_DICT) return PLISTKIT_ERR_TYPE_MISMATCH;

    for (size_t i = 0; i < dict->value.dict_val.count; i++) {
        free(dict->value.dict_val.keys[i]);
        plistkit_node_free(dict->value.dict_val.values[i]);
    }

    dict->value.dict_val.count = 0;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_dict_get_keys(const plistkit_node_t *dict, char ***keys,
                       size_t *count)
{
    if (!dict || !keys || !count) return PLISTKIT_ERR_INVALID_ARG;
    if (dict->type != PLISTKIT_TYPE_DICT) return PLISTKIT_ERR_TYPE_MISMATCH;

    size_t n = dict->value.dict_val.count;
    *count = n;

    if (n == 0) {
        *keys = NULL;
        return PLISTKIT_OK;
    }

    char **arr = (char **)malloc(n * sizeof(char *));
    if (!arr) return PLISTKIT_ERR_NOMEM;

    for (size_t i = 0; i < n; i++) {
        arr[i] = plistkit_strdup(dict->value.dict_val.keys[i]);
        if (!arr[i]) {
            for (size_t j = 0; j < i; j++)
                free(arr[j]);
            free(arr);
            return PLISTKIT_ERR_NOMEM;
        }
    }

    *keys = arr;
    return PLISTKIT_OK;
}
