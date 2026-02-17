/**
 * @file plistkit.h
 * @brief Public API for libplistkit — a lightweight C library for XML plist files.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef PLISTKIT_H
#define PLISTKIT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Version                                                                     */
/* ========================================================================== */

/** Get the library version string at runtime (e.g. "0.1.0"). */
const char *plistkit_version(void);

/** Get individual version components at runtime. Any pointer may be NULL. */
void plistkit_version_get(int *major, int *minor, int *patch);

/* ========================================================================== */
/* Types                                                                       */
/* ========================================================================== */

/** Opaque plist node handle. */
typedef struct plistkit_node plistkit_node_t;

/** Node type identifiers. */
typedef enum {
    PLISTKIT_TYPE_NONE = 0,
    PLISTKIT_TYPE_BOOLEAN,
    PLISTKIT_TYPE_INTEGER,
    PLISTKIT_TYPE_REAL,
    PLISTKIT_TYPE_STRING,
    PLISTKIT_TYPE_DATA,
    PLISTKIT_TYPE_DATE,
    PLISTKIT_TYPE_ARRAY,
    PLISTKIT_TYPE_DICT
} plistkit_type_t;

/** Error codes returned by library functions. */
typedef enum {
    PLISTKIT_OK = 0,
    PLISTKIT_ERR_NOMEM,
    PLISTKIT_ERR_INVALID_ARG,
    PLISTKIT_ERR_PARSE,
    PLISTKIT_ERR_IO,
    PLISTKIT_ERR_TYPE_MISMATCH,
    PLISTKIT_ERR_NOT_FOUND,
    PLISTKIT_ERR_LIMIT_EXCEEDED,
    PLISTKIT_ERR_OVERFLOW,
    PLISTKIT_ERR_INDEX_OUT_OF_RANGE,
    PLISTKIT_ERR_INVALID_UTF8,
    PLISTKIT_ERR_INVALID_BASE64,
    PLISTKIT_ERR_INVALID_DATE,
    PLISTKIT_ERR_INVALID_XML,
    PLISTKIT_ERR_UNSUPPORTED
} plistkit_error_t;

/** Configuration flags. */
#define PLISTKIT_NO_LIMITS      0x00000001u  /**< Disable all security limits */
#define PLISTKIT_STRICT_MODE    0x00000002u  /**< Strict XML validation */

/** Parser/writer configuration with security limits. */
typedef struct {
    size_t   max_file_size;      /**< Max input size in bytes (default 100 MB) */
    size_t   max_depth;          /**< Max nesting depth (default 100) */
    size_t   max_string_length;  /**< Max string length in bytes (default 10 MB) */
    size_t   max_data_size;      /**< Max binary data size (default 100 MB) */
    size_t   max_array_items;    /**< Max array elements (default 1 000 000) */
    size_t   max_dict_entries;   /**< Max dictionary entries (default 1 000 000) */
    uint32_t flags;              /**< Combination of PLISTKIT_* flags */
} plistkit_config_t;

/** Detailed error context (optional, for diagnostics). */
typedef struct {
    plistkit_error_t code;
    char             message[256];
    int              line;       /**< Line number in plist input (1-based) */
    int              column;     /**< Column number in plist input (1-based) */
} plistkit_error_ctx_t;

/** Options controlling XML output formatting. */
typedef struct {
    bool pretty_print;   /**< Add indentation and newlines (default true) */
    int  indent_size;    /**< Spaces per indent level (default 2) */
    bool xml_declaration;/**< Emit <?xml ...?> header (default true) */
    bool doctype;        /**< Emit <!DOCTYPE ...> (default true) */
} plistkit_writer_opts_t;

/* ========================================================================== */
/* Configuration defaults                                                      */
/* ========================================================================== */

/** Return a config struct filled with safe defaults. */
plistkit_config_t plistkit_config_default(void);

/** Return a writer_opts struct filled with defaults. */
plistkit_writer_opts_t plistkit_writer_opts_default(void);

/* ========================================================================== */
/* Node creation                                                               */
/* ========================================================================== */

plistkit_node_t *plistkit_node_new_boolean(bool value);
plistkit_node_t *plistkit_node_new_integer(int64_t value);
plistkit_node_t *plistkit_node_new_real(double value);

/** Create a string node. @p value is copied internally. */
plistkit_node_t *plistkit_node_new_string(const char *value);

/** Create a binary data node. @p data is copied internally. */
plistkit_node_t *plistkit_node_new_data(const void *data, size_t size);

/** Create a date node. @p timestamp is seconds since Unix epoch (UTC). */
plistkit_node_t *plistkit_node_new_date(int64_t timestamp);

plistkit_node_t *plistkit_node_new_array(void);
plistkit_node_t *plistkit_node_new_dict(void);

/* ========================================================================== */
/* Node lifecycle                                                              */
/* ========================================================================== */

/** Deep-free a node and all children. NULL is safe. */
void plistkit_node_free(plistkit_node_t *node);

/** Deep-copy a node and all children. Returns NULL on error. */
plistkit_node_t *plistkit_node_copy(const plistkit_node_t *node);

/* ========================================================================== */
/* Node introspection                                                          */
/* ========================================================================== */

plistkit_type_t plistkit_node_get_type(const plistkit_node_t *node);
bool plistkit_node_is_type(const plistkit_node_t *node, plistkit_type_t type);

/** Deep comparison. Returns true if both trees are structurally equal. */
bool plistkit_node_equal(const plistkit_node_t *a, const plistkit_node_t *b);

/* ========================================================================== */
/* Value getters (return error on type mismatch or NULL node)                  */
/* ========================================================================== */

plistkit_error_t plistkit_node_get_boolean(const plistkit_node_t *node, bool *out);
plistkit_error_t plistkit_node_get_integer(const plistkit_node_t *node, int64_t *out);
plistkit_error_t plistkit_node_get_real(const plistkit_node_t *node, double *out);

/** Get pointer to internal string buffer. Do not free. Valid until node is modified/freed. */
plistkit_error_t plistkit_node_get_string(const plistkit_node_t *node, const char **out);

/** Get pointer to internal data buffer. Do not free. */
plistkit_error_t plistkit_node_get_data(const plistkit_node_t *node,
                                        const void **out, size_t *size);

plistkit_error_t plistkit_node_get_date(const plistkit_node_t *node, int64_t *out);

/* ========================================================================== */
/* Value setters (return error on type mismatch or NULL node)                  */
/* ========================================================================== */

plistkit_error_t plistkit_node_set_boolean(plistkit_node_t *node, bool value);
plistkit_error_t plistkit_node_set_integer(plistkit_node_t *node, int64_t value);
plistkit_error_t plistkit_node_set_real(plistkit_node_t *node, double value);
plistkit_error_t plistkit_node_set_string(plistkit_node_t *node, const char *value);
plistkit_error_t plistkit_node_set_data(plistkit_node_t *node,
                                        const void *data, size_t size);
plistkit_error_t plistkit_node_set_date(plistkit_node_t *node, int64_t timestamp);

/* ========================================================================== */
/* Array operations (node must be PLISTKIT_TYPE_ARRAY)                         */
/* ========================================================================== */

plistkit_error_t plistkit_array_get_size(const plistkit_node_t *array, size_t *out);

/** Get item at index. Returns a borrowed reference — do NOT free. */
plistkit_error_t plistkit_array_get_item(const plistkit_node_t *array,
                                         size_t index,
                                         plistkit_node_t **out);

/** Append item. Ownership of @p item transfers to @p array on success. */
plistkit_error_t plistkit_array_append(plistkit_node_t *array,
                                       plistkit_node_t *item);

plistkit_error_t plistkit_array_insert(plistkit_node_t *array,
                                       size_t index,
                                       plistkit_node_t *item);

/** Remove and free item at index. */
plistkit_error_t plistkit_array_remove(plistkit_node_t *array, size_t index);

/** Remove and free all items. */
plistkit_error_t plistkit_array_clear(plistkit_node_t *array);

/* ========================================================================== */
/* Dictionary operations (node must be PLISTKIT_TYPE_DICT)                     */
/* ========================================================================== */

plistkit_error_t plistkit_dict_get_size(const plistkit_node_t *dict, size_t *out);

/** Get value for key. Returns a borrowed reference — do NOT free. */
plistkit_error_t plistkit_dict_get_value(const plistkit_node_t *dict,
                                         const char *key,
                                         plistkit_node_t **out);

bool plistkit_dict_has_key(const plistkit_node_t *dict, const char *key);

/** Set value for key. Key is copied. Ownership of @p value transfers on success.
 *  If the key already exists, old value is freed. */
plistkit_error_t plistkit_dict_set_value(plistkit_node_t *dict,
                                         const char *key,
                                         plistkit_node_t *value);

/** Remove key and free its value. */
plistkit_error_t plistkit_dict_remove_key(plistkit_node_t *dict, const char *key);

/** Remove and free all entries. */
plistkit_error_t plistkit_dict_clear(plistkit_node_t *dict);

/** Get all keys as a newly allocated array of strings.
 *  Caller must free each string and the array itself. */
plistkit_error_t plistkit_dict_get_keys(const plistkit_node_t *dict,
                                        char ***keys, size_t *count);

/* ========================================================================== */
/* Parsing                                                                     */
/* ========================================================================== */

/** Parse XML plist from memory buffer.
 *  @param config  NULL for defaults.
 *  @param error   NULL if diagnostics not needed. */
plistkit_node_t *plistkit_parse_mem(const void *data, size_t size,
                                    const plistkit_config_t *config,
                                    plistkit_error_ctx_t *error);

/** Parse XML plist from null-terminated string. */
plistkit_node_t *plistkit_parse_string(const char *str,
                                       const plistkit_config_t *config,
                                       plistkit_error_ctx_t *error);

/** Parse XML plist from file path. */
plistkit_node_t *plistkit_parse_file(const char *path,
                                     const plistkit_config_t *config,
                                     plistkit_error_ctx_t *error);

/* ========================================================================== */
/* Writing                                                                     */
/* ========================================================================== */

/** Write node tree as XML plist string. Caller must free() the result.
 *  @param opts  NULL for defaults. */
char *plistkit_write_string(const plistkit_node_t *node,
                            const plistkit_writer_opts_t *opts,
                            plistkit_error_ctx_t *error);

/** Write node tree as XML plist to file. */
plistkit_error_t plistkit_write_file(const plistkit_node_t *node,
                                     const char *path,
                                     const plistkit_writer_opts_t *opts,
                                     plistkit_error_ctx_t *error);

/* ========================================================================== */
/* Utility                                                                     */
/* ========================================================================== */

/** Human-readable name for an error code. */
const char *plistkit_error_string(plistkit_error_t error);

/** Human-readable name for a node type. */
const char *plistkit_type_name(plistkit_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* PLISTKIT_H */
