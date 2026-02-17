/**
 * @file parser.c
 * @brief Custom lightweight XML parser for the plist format subset.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <stdarg.h>

/* ========================================================================== */
/* Error helpers                                                               */
/* ========================================================================== */

static void
set_error(parser_ctx_t *ctx, plistkit_error_t code, const char *msg)
{
    if (!ctx->error)
        return;
    ctx->error->code = code;
    ctx->error->line = ctx->line;
    ctx->error->column = ctx->column;
    snprintf(ctx->error->message, sizeof(ctx->error->message), "%s", msg);
}

static void
set_errorf(parser_ctx_t *ctx, plistkit_error_t code, const char *fmt, ...)
{
    if (!ctx->error)
        return;
    ctx->error->code = code;
    ctx->error->line = ctx->line;
    ctx->error->column = ctx->column;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(ctx->error->message, sizeof(ctx->error->message), fmt, ap);
    va_end(ap);
}

/* ========================================================================== */
/* Character access                                                            */
/* ========================================================================== */

static inline bool
at_end(const parser_ctx_t *ctx)
{
    return ctx->pos >= ctx->input_size;
}

static inline char
peek(const parser_ctx_t *ctx)
{
    if (at_end(ctx)) return '\0';
    return ctx->input[ctx->pos];
}

static inline char
advance(parser_ctx_t *ctx)
{
    if (at_end(ctx)) return '\0';
    char c = ctx->input[ctx->pos++];
    if (c == '\n') {
        ctx->line++;
        ctx->column = 1;
    } else {
        ctx->column++;
    }
    return c;
}

static void
skip_whitespace(parser_ctx_t *ctx)
{
    while (!at_end(ctx) && isspace((unsigned char)peek(ctx)))
        advance(ctx);
}

static bool
match_str(parser_ctx_t *ctx, const char *s)
{
    size_t len = strlen(s);
    if (ctx->pos + len > ctx->input_size)
        return false;
    if (memcmp(ctx->input + ctx->pos, s, len) != 0)
        return false;
    for (size_t i = 0; i < len; i++)
        advance(ctx);
    return true;
}

static bool
try_match(const parser_ctx_t *ctx, const char *s)
{
    size_t len = strlen(s);
    if (ctx->pos + len > ctx->input_size)
        return false;
    return memcmp(ctx->input + ctx->pos, s, len) == 0;
}

/* ========================================================================== */
/* Read text content between tags                                              */
/* ========================================================================== */

/** Read characters until '<' is encountered. Returns malloc'd string. */
static char *
read_text_content(parser_ctx_t *ctx, size_t *out_len)
{
    size_t start = ctx->pos;
    while (!at_end(ctx) && peek(ctx) != '<')
        advance(ctx);

    size_t raw_len = ctx->pos - start;
    char *unescaped = plistkit_xml_unescape(ctx->input + start, raw_len);
    if (!unescaped) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory reading text content");
        return NULL;
    }
    if (out_len)
        *out_len = strlen(unescaped);
    return unescaped;
}

/* ========================================================================== */
/* Tag reading                                                                 */
/* ========================================================================== */

/** Read a tag name after '<'. Returns malloc'd string or NULL. */
static char *
read_tag_name(parser_ctx_t *ctx)
{
    size_t start = ctx->pos;
    while (!at_end(ctx) && !isspace((unsigned char)peek(ctx)) &&
           peek(ctx) != '>' && peek(ctx) != '/')
        advance(ctx);

    size_t len = ctx->pos - start;
    if (len == 0) {
        set_error(ctx, PLISTKIT_ERR_PARSE, "Empty tag name");
        return NULL;
    }
    return plistkit_strndup(ctx->input + start, len);
}

/** Skip everything until and including '>'. */
static bool
skip_to_close_bracket(parser_ctx_t *ctx)
{
    while (!at_end(ctx) && peek(ctx) != '>')
        advance(ctx);
    if (at_end(ctx)) {
        set_error(ctx, PLISTKIT_ERR_PARSE, "Unexpected end of input looking for '>'");
        return false;
    }
    advance(ctx); /* consume '>' */
    return true;
}

/** Expect and consume a closing tag: </name> */
static bool
expect_close_tag(parser_ctx_t *ctx, const char *name)
{
    skip_whitespace(ctx);
    if (!match_str(ctx, "</")) {
        set_errorf(ctx, PLISTKIT_ERR_PARSE, "Expected closing tag </%s>", name);
        return false;
    }
    char *tag = read_tag_name(ctx);
    if (!tag) return false;

    if (strcmp(tag, name) != 0) {
        set_errorf(ctx, PLISTKIT_ERR_PARSE,
                   "Mismatched closing tag: expected </%s>, got </%s>", name, tag);
        free(tag);
        return false;
    }
    free(tag);

    skip_whitespace(ctx);
    if (at_end(ctx) || peek(ctx) != '>') {
        set_errorf(ctx, PLISTKIT_ERR_PARSE, "Expected '>' after </%s", name);
        return false;
    }
    advance(ctx); /* consume '>' */
    return true;
}

/* ========================================================================== */
/* Skip XML declaration and DOCTYPE                                            */
/* ========================================================================== */

static void
skip_xml_declaration(parser_ctx_t *ctx)
{
    skip_whitespace(ctx);
    if (try_match(ctx, "<?xml")) {
        while (!at_end(ctx) && !try_match(ctx, "?>"))
            advance(ctx);
        if (!at_end(ctx))
            match_str(ctx, "?>");
    }
}

static void
skip_doctype(parser_ctx_t *ctx)
{
    skip_whitespace(ctx);
    if (try_match(ctx, "<!DOCTYPE")) {
        /* skip until closing >, handle nested [] for internal subset */
        int depth = 0;
        while (!at_end(ctx)) {
            char c = peek(ctx);
            if (c == '[') depth++;
            else if (c == ']') depth--;
            else if (c == '>' && depth <= 0) {
                advance(ctx);
                return;
            }
            advance(ctx);
        }
    }
}

static void
skip_comment(parser_ctx_t *ctx)
{
    /* consume <!-- */
    match_str(ctx, "<!--");
    while (!at_end(ctx)) {
        if (try_match(ctx, "-->")) {
            match_str(ctx, "-->");
            return;
        }
        advance(ctx);
    }
}

/* ========================================================================== */
/* Element parsing                                                             */
/* ========================================================================== */

static plistkit_node_t *parse_element(parser_ctx_t *ctx);

static plistkit_node_t *
parse_string_element(parser_ctx_t *ctx)
{
    size_t len = 0;

    /* handle <string/> (empty self-closing) */
    /* at this point we've already read <string and are past the tag name */
    /* the caller handled '>' already; content is between > and </string> */

    char *text = read_text_content(ctx, &len);
    if (!text) return NULL;

    if (!(ctx->config.flags & PLISTKIT_NO_LIMITS) &&
        len > ctx->config.max_string_length) {
        set_error(ctx, PLISTKIT_ERR_LIMIT_EXCEEDED, "String exceeds max length");
        free(text);
        return NULL;
    }

    plistkit_node_t *node = plistkit_node_new_string(text);
    free(text);
    if (!node) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    if (!expect_close_tag(ctx, "string")) {
        plistkit_node_free(node);
        return NULL;
    }
    return node;
}

static plistkit_node_t *
parse_key_text(parser_ctx_t *ctx, char **out_key)
{
    size_t len = 0;
    char *text = read_text_content(ctx, &len);
    if (!text) return NULL;

    if (!expect_close_tag(ctx, "key")) {
        free(text);
        return NULL;
    }

    *out_key = text;
    return (plistkit_node_t *)1; /* dummy non-NULL to signal success */
}

static plistkit_node_t *
parse_integer_element(parser_ctx_t *ctx)
{
    size_t len = 0;
    char *text = read_text_content(ctx, &len);
    if (!text) return NULL;

    /* trim whitespace */
    char *p = text;
    while (*p && isspace((unsigned char)*p)) p++;

    char *endptr = NULL;
    errno = 0;
    long long val = strtoll(p, &endptr, 10);

    if (errno == ERANGE || endptr == p) {
        set_errorf(ctx, PLISTKIT_ERR_PARSE, "Invalid integer value: %s", text);
        free(text);
        return NULL;
    }
    free(text);

    plistkit_node_t *node = plistkit_node_new_integer((int64_t)val);
    if (!node) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    if (!expect_close_tag(ctx, "integer")) {
        plistkit_node_free(node);
        return NULL;
    }
    return node;
}

static plistkit_node_t *
parse_real_element(parser_ctx_t *ctx)
{
    size_t len = 0;
    char *text = read_text_content(ctx, &len);
    if (!text) return NULL;

    char *p = text;
    while (*p && isspace((unsigned char)*p)) p++;

    char *endptr = NULL;
    errno = 0;
    double val = strtod(p, &endptr);

    if (errno == ERANGE || endptr == p) {
        set_errorf(ctx, PLISTKIT_ERR_PARSE, "Invalid real value: %s", text);
        free(text);
        return NULL;
    }
    free(text);

    plistkit_node_t *node = plistkit_node_new_real(val);
    if (!node) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    if (!expect_close_tag(ctx, "real")) {
        plistkit_node_free(node);
        return NULL;
    }
    return node;
}

static plistkit_node_t *
parse_data_element(parser_ctx_t *ctx)
{
    size_t len = 0;
    char *text = read_text_content(ctx, &len);
    if (!text) return NULL;

    size_t decoded_max = plistkit_base64_decoded_size(text, len);

    if (!(ctx->config.flags & PLISTKIT_NO_LIMITS) &&
        decoded_max > ctx->config.max_data_size) {
        set_error(ctx, PLISTKIT_ERR_LIMIT_EXCEEDED, "Data exceeds max size");
        free(text);
        return NULL;
    }

    uint8_t *decoded = NULL;
    size_t decoded_size = 0;

    if (decoded_max > 0) {
        decoded = (uint8_t *)malloc(decoded_max);
        if (!decoded) {
            set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
            free(text);
            return NULL;
        }

        plistkit_error_t err = plistkit_base64_decode(
            text, len, decoded, decoded_max, &decoded_size);
        if (err != PLISTKIT_OK) {
            set_error(ctx, PLISTKIT_ERR_INVALID_BASE64, "Invalid base64 in <data>");
            free(decoded);
            free(text);
            return NULL;
        }
    }
    free(text);

    plistkit_node_t *node = plistkit_node_new_data(decoded, decoded_size);
    free(decoded);
    if (!node) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    if (!expect_close_tag(ctx, "data")) {
        plistkit_node_free(node);
        return NULL;
    }
    return node;
}

static plistkit_node_t *
parse_date_element(parser_ctx_t *ctx)
{
    size_t len = 0;
    char *text = read_text_content(ctx, &len);
    if (!text) return NULL;

    /* trim */
    char *p = text;
    while (*p && isspace((unsigned char)*p)) p++;
    size_t plen = strlen(p);
    while (plen > 0 && isspace((unsigned char)p[plen - 1])) plen--;
    p[plen] = '\0';

    int64_t ts;
    plistkit_error_t err = plistkit_date_parse(p, &ts);
    free(text);

    if (err != PLISTKIT_OK) {
        set_error(ctx, PLISTKIT_ERR_INVALID_DATE, "Invalid date format in <date>");
        return NULL;
    }

    plistkit_node_t *node = plistkit_node_new_date(ts);
    if (!node) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    if (!expect_close_tag(ctx, "date")) {
        plistkit_node_free(node);
        return NULL;
    }
    return node;
}

static plistkit_node_t *
parse_dict_element(parser_ctx_t *ctx)
{
    plistkit_node_t *dict = plistkit_node_new_dict();
    if (!dict) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    while (1) {
        skip_whitespace(ctx);

        if (at_end(ctx)) {
            set_error(ctx, PLISTKIT_ERR_PARSE, "Unexpected end inside <dict>");
            plistkit_node_free(dict);
            return NULL;
        }

        /* check for </dict> */
        if (try_match(ctx, "</dict>")) {
            match_str(ctx, "</dict>");
            return dict;
        }

        /* skip comments */
        if (try_match(ctx, "<!--")) {
            skip_comment(ctx);
            continue;
        }

        /* expect <key>...</key> */
        if (!match_str(ctx, "<key")) {
            set_error(ctx, PLISTKIT_ERR_PARSE, "Expected <key> inside <dict>");
            plistkit_node_free(dict);
            return NULL;
        }

        /* skip attributes of <key> and consume '>' */
        if (!skip_to_close_bracket(ctx)) {
            plistkit_node_free(dict);
            return NULL;
        }

        char *key = NULL;
        plistkit_node_t *dummy = parse_key_text(ctx, &key);
        if (!dummy) {
            plistkit_node_free(dict);
            return NULL;
        }

        /* check dict entries limit */
        size_t dict_size = 0;
        plistkit_dict_get_size(dict, &dict_size);
        if (!(ctx->config.flags & PLISTKIT_NO_LIMITS) &&
            dict_size >= ctx->config.max_dict_entries) {
            set_error(ctx, PLISTKIT_ERR_LIMIT_EXCEEDED, "Dictionary entry limit exceeded");
            free(key);
            plistkit_node_free(dict);
            return NULL;
        }

        /* parse value element */
        plistkit_node_t *value = parse_element(ctx);
        if (!value) {
            free(key);
            plistkit_node_free(dict);
            return NULL;
        }

        plistkit_error_t err = plistkit_dict_set_value(dict, key, value);
        free(key);
        if (err != PLISTKIT_OK) {
            plistkit_node_free(value);
            plistkit_node_free(dict);
            set_error(ctx, err, "Failed to add dict entry");
            return NULL;
        }
    }
}

static plistkit_node_t *
parse_array_element(parser_ctx_t *ctx)
{
    plistkit_node_t *array = plistkit_node_new_array();
    if (!array) {
        set_error(ctx, PLISTKIT_ERR_NOMEM, "Out of memory");
        return NULL;
    }

    while (1) {
        skip_whitespace(ctx);

        if (at_end(ctx)) {
            set_error(ctx, PLISTKIT_ERR_PARSE, "Unexpected end inside <array>");
            plistkit_node_free(array);
            return NULL;
        }

        /* check for </array> */
        if (try_match(ctx, "</array>")) {
            match_str(ctx, "</array>");
            return array;
        }

        /* skip comments */
        if (try_match(ctx, "<!--")) {
            skip_comment(ctx);
            continue;
        }

        /* check array size limit */
        size_t arr_size = 0;
        plistkit_array_get_size(array, &arr_size);
        if (!(ctx->config.flags & PLISTKIT_NO_LIMITS) &&
            arr_size >= ctx->config.max_array_items) {
            set_error(ctx, PLISTKIT_ERR_LIMIT_EXCEEDED, "Array item limit exceeded");
            plistkit_node_free(array);
            return NULL;
        }

        plistkit_node_t *item = parse_element(ctx);
        if (!item) {
            plistkit_node_free(array);
            return NULL;
        }

        plistkit_error_t err = plistkit_array_append(array, item);
        if (err != PLISTKIT_OK) {
            plistkit_node_free(item);
            plistkit_node_free(array);
            set_error(ctx, err, "Failed to append array item");
            return NULL;
        }
    }
}

/**
 * Parse a single plist element. Expects to be positioned at '<'.
 */
static plistkit_node_t *
parse_element(parser_ctx_t *ctx)
{
    skip_whitespace(ctx);

    /* skip comments */
    while (try_match(ctx, "<!--")) {
        skip_comment(ctx);
        skip_whitespace(ctx);
    }

    if (at_end(ctx) || peek(ctx) != '<') {
        set_error(ctx, PLISTKIT_ERR_PARSE, "Expected '<' for element start");
        return NULL;
    }

    advance(ctx); /* consume '<' */

    /* check for self-closing boolean tags */
    /* <true/> or <false/> */
    if (try_match(ctx, "true")) {
        match_str(ctx, "true");
        skip_whitespace(ctx);
        if (peek(ctx) == '/') {
            advance(ctx); /* '/' */
            if (peek(ctx) == '>') {
                advance(ctx); /* '>' */
                return plistkit_node_new_boolean(true);
            }
        }
        /* <true> ... </true> form (unusual but valid) */
        if (peek(ctx) == '>') {
            advance(ctx);
            skip_whitespace(ctx);
            if (!expect_close_tag(ctx, "true"))
                return NULL;
            return plistkit_node_new_boolean(true);
        }
        set_error(ctx, PLISTKIT_ERR_PARSE, "Malformed <true> element");
        return NULL;
    }

    if (try_match(ctx, "false")) {
        match_str(ctx, "false");
        skip_whitespace(ctx);
        if (peek(ctx) == '/') {
            advance(ctx);
            if (peek(ctx) == '>') {
                advance(ctx);
                return plistkit_node_new_boolean(false);
            }
        }
        if (peek(ctx) == '>') {
            advance(ctx);
            skip_whitespace(ctx);
            if (!expect_close_tag(ctx, "false"))
                return NULL;
            return plistkit_node_new_boolean(false);
        }
        set_error(ctx, PLISTKIT_ERR_PARSE, "Malformed <false> element");
        return NULL;
    }

    /* read tag name */
    char *tag = read_tag_name(ctx);
    if (!tag) return NULL;

    /* check for self-closing empty elements like <string/> */
    skip_whitespace(ctx);
    bool self_closing = false;
    if (peek(ctx) == '/') {
        advance(ctx);
        self_closing = true;
    }

    if (peek(ctx) != '>') {
        /* there might be attributes — skip them */
        if (!skip_to_close_bracket(ctx)) {
            free(tag);
            return NULL;
        }
        /* check if there was a '/' just before '>' */
        if (ctx->pos >= 2 && ctx->input[ctx->pos - 2] == '/')
            self_closing = true;
    } else {
        advance(ctx); /* consume '>' */
    }

    /* check depth */
    ctx->depth++;
    if (!(ctx->config.flags & PLISTKIT_NO_LIMITS) &&
        ctx->depth > ctx->config.max_depth) {
        set_error(ctx, PLISTKIT_ERR_LIMIT_EXCEEDED, "Nesting depth limit exceeded");
        free(tag);
        ctx->depth--;
        return NULL;
    }

    plistkit_node_t *node = NULL;

    if (strcmp(tag, "dict") == 0) {
        if (self_closing) {
            node = plistkit_node_new_dict();
        } else {
            node = parse_dict_element(ctx);
        }
    } else if (strcmp(tag, "array") == 0) {
        if (self_closing) {
            node = plistkit_node_new_array();
        } else {
            node = parse_array_element(ctx);
        }
    } else if (strcmp(tag, "string") == 0) {
        if (self_closing) {
            node = plistkit_node_new_string("");
        } else {
            node = parse_string_element(ctx);
        }
    } else if (strcmp(tag, "integer") == 0) {
        if (self_closing) {
            set_error(ctx, PLISTKIT_ERR_PARSE, "Self-closing <integer/> is invalid");
        } else {
            node = parse_integer_element(ctx);
        }
    } else if (strcmp(tag, "real") == 0) {
        if (self_closing) {
            set_error(ctx, PLISTKIT_ERR_PARSE, "Self-closing <real/> is invalid");
        } else {
            node = parse_real_element(ctx);
        }
    } else if (strcmp(tag, "data") == 0) {
        if (self_closing) {
            node = plistkit_node_new_data(NULL, 0);
        } else {
            node = parse_data_element(ctx);
        }
    } else if (strcmp(tag, "date") == 0) {
        if (self_closing) {
            set_error(ctx, PLISTKIT_ERR_PARSE, "Self-closing <date/> is invalid");
        } else {
            node = parse_date_element(ctx);
        }
    } else {
        set_errorf(ctx, PLISTKIT_ERR_PARSE, "Unknown element: <%s>", tag);
    }

    free(tag);
    ctx->depth--;
    return node;
}

/* ========================================================================== */
/* Top-level parse                                                             */
/* ========================================================================== */

static plistkit_node_t *
parse_plist(parser_ctx_t *ctx)
{
    skip_xml_declaration(ctx);
    skip_doctype(ctx);

    skip_whitespace(ctx);

    /* skip comments before <plist> */
    while (try_match(ctx, "<!--")) {
        skip_comment(ctx);
        skip_whitespace(ctx);
    }

    /* expect <plist ...> */
    if (!match_str(ctx, "<plist")) {
        set_error(ctx, PLISTKIT_ERR_PARSE, "Expected <plist> root element");
        return NULL;
    }

    /* skip attributes (version="1.0" etc.) and close bracket */
    if (!skip_to_close_bracket(ctx))
        return NULL;

    /* parse root value element */
    skip_whitespace(ctx);

    /* handle empty <plist></plist> */
    if (try_match(ctx, "</plist>")) {
        match_str(ctx, "</plist>");
        /* empty plist — return empty dict as convention */
        return plistkit_node_new_dict();
    }

    plistkit_node_t *root = parse_element(ctx);
    if (!root)
        return NULL;

    /* expect </plist> */
    skip_whitespace(ctx);
    if (!match_str(ctx, "</plist>")) {
        set_error(ctx, PLISTKIT_ERR_PARSE, "Expected </plist> closing tag");
        plistkit_node_free(root);
        return NULL;
    }

    return root;
}

/* ========================================================================== */
/* Public API                                                                  */
/* ========================================================================== */

plistkit_node_t *
plistkit_parse_mem(const void *data, size_t size,
                   const plistkit_config_t *config,
                   plistkit_error_ctx_t *error)
{
    if (!data || size == 0) {
        if (error) {
            error->code = PLISTKIT_ERR_INVALID_ARG;
            snprintf(error->message, sizeof(error->message),
                     "NULL or empty input");
            error->line = 0;
            error->column = 0;
        }
        return NULL;
    }

    parser_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.input = (const char *)data;
    ctx.input_size = size;
    ctx.pos = 0;
    ctx.line = 1;
    ctx.column = 1;
    ctx.depth = 0;
    ctx.error = error;

    if (config) {
        ctx.config = *config;
    } else {
        ctx.config = plistkit_config_default();
    }

    /* check file size limit */
    if (!(ctx.config.flags & PLISTKIT_NO_LIMITS) &&
        size > ctx.config.max_file_size) {
        if (error) {
            error->code = PLISTKIT_ERR_LIMIT_EXCEEDED;
            snprintf(error->message, sizeof(error->message),
                     "Input size %zu exceeds limit %zu",
                     size, ctx.config.max_file_size);
        }
        return NULL;
    }

    if (error)
        memset(error, 0, sizeof(*error));

    return parse_plist(&ctx);
}

plistkit_node_t *
plistkit_parse_string(const char *str,
                      const plistkit_config_t *config,
                      plistkit_error_ctx_t *error)
{
    if (!str) {
        if (error) {
            error->code = PLISTKIT_ERR_INVALID_ARG;
            snprintf(error->message, sizeof(error->message), "NULL string");
        }
        return NULL;
    }
    return plistkit_parse_mem(str, strlen(str), config, error);
}

plistkit_node_t *
plistkit_parse_file(const char *path,
                    const plistkit_config_t *config,
                    plistkit_error_ctx_t *error)
{
    if (!path) {
        if (error) {
            error->code = PLISTKIT_ERR_INVALID_ARG;
            snprintf(error->message, sizeof(error->message), "NULL path");
        }
        return NULL;
    }

    void *data = NULL;
    size_t size = 0;
    plistkit_error_t err = plistkit_file_read(path, &data, &size);
    if (err != PLISTKIT_OK) {
        if (error) {
            error->code = err;
            snprintf(error->message, sizeof(error->message),
                     "Failed to read file: %s", path);
        }
        return NULL;
    }

    plistkit_node_t *root = plistkit_parse_mem(data, size, config, error);
    free(data);
    return root;
}
