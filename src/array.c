/**
 * @file array.c
 * @brief Array node operations.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"

#define ARRAY_INIT_CAPACITY 4

static plistkit_error_t
array_grow(plistkit_node_t *array)
{
    size_t new_cap = array->value.array_val.capacity == 0
                         ? ARRAY_INIT_CAPACITY
                         : array->value.array_val.capacity * 2;

    plistkit_node_t **new_items = (plistkit_node_t **)plistkit_realloc(
        array->value.array_val.items, new_cap * sizeof(plistkit_node_t *));

    if (!new_items)
        return PLISTKIT_ERR_NOMEM;

    array->value.array_val.items = new_items;
    array->value.array_val.capacity = new_cap;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_array_get_size(const plistkit_node_t *array, size_t *out)
{
    if (!array || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (array->type != PLISTKIT_TYPE_ARRAY) return PLISTKIT_ERR_TYPE_MISMATCH;
    *out = array->value.array_val.count;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_array_get_item(const plistkit_node_t *array, size_t index,
                        plistkit_node_t **out)
{
    if (!array || !out) return PLISTKIT_ERR_INVALID_ARG;
    if (array->type != PLISTKIT_TYPE_ARRAY) return PLISTKIT_ERR_TYPE_MISMATCH;
    if (index >= array->value.array_val.count) return PLISTKIT_ERR_INDEX_OUT_OF_RANGE;
    *out = array->value.array_val.items[index];
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_array_append(plistkit_node_t *array, plistkit_node_t *item)
{
    if (!array || !item) return PLISTKIT_ERR_INVALID_ARG;
    if (array->type != PLISTKIT_TYPE_ARRAY) return PLISTKIT_ERR_TYPE_MISMATCH;

    if (array->value.array_val.count >= array->value.array_val.capacity) {
        plistkit_error_t err = array_grow(array);
        if (err != PLISTKIT_OK) return err;
    }

    array->value.array_val.items[array->value.array_val.count++] = item;
    item->parent = array;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_array_insert(plistkit_node_t *array, size_t index,
                      plistkit_node_t *item)
{
    if (!array || !item) return PLISTKIT_ERR_INVALID_ARG;
    if (array->type != PLISTKIT_TYPE_ARRAY) return PLISTKIT_ERR_TYPE_MISMATCH;
    if (index > array->value.array_val.count) return PLISTKIT_ERR_INDEX_OUT_OF_RANGE;

    if (array->value.array_val.count >= array->value.array_val.capacity) {
        plistkit_error_t err = array_grow(array);
        if (err != PLISTKIT_OK) return err;
    }

    /* shift elements right */
    for (size_t i = array->value.array_val.count; i > index; i--)
        array->value.array_val.items[i] = array->value.array_val.items[i - 1];

    array->value.array_val.items[index] = item;
    array->value.array_val.count++;
    item->parent = array;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_array_remove(plistkit_node_t *array, size_t index)
{
    if (!array) return PLISTKIT_ERR_INVALID_ARG;
    if (array->type != PLISTKIT_TYPE_ARRAY) return PLISTKIT_ERR_TYPE_MISMATCH;
    if (index >= array->value.array_val.count) return PLISTKIT_ERR_INDEX_OUT_OF_RANGE;

    plistkit_node_free(array->value.array_val.items[index]);

    /* shift elements left */
    for (size_t i = index; i + 1 < array->value.array_val.count; i++)
        array->value.array_val.items[i] = array->value.array_val.items[i + 1];

    array->value.array_val.count--;
    return PLISTKIT_OK;
}

plistkit_error_t
plistkit_array_clear(plistkit_node_t *array)
{
    if (!array) return PLISTKIT_ERR_INVALID_ARG;
    if (array->type != PLISTKIT_TYPE_ARRAY) return PLISTKIT_ERR_TYPE_MISMATCH;

    for (size_t i = 0; i < array->value.array_val.count; i++)
        plistkit_node_free(array->value.array_val.items[i]);

    array->value.array_val.count = 0;
    return PLISTKIT_OK;
}
