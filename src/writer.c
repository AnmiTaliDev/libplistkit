/**
 * @file writer.c
 * @brief XML plist writer with pretty-print and compact modes.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "internal.h"

#include <inttypes.h>

#define WRITER_INIT_CAPACITY 4096

/* ========================================================================== */
/* Buffer management                                                           */
/* ========================================================================== */

static bool
writer_ensure(writer_ctx_t *ctx, size_t additional)
{
    if (ctx->failed)
        return false;

    size_t needed = ctx->size + additional + 1; /* +1 for null terminator */
    if (needed <= ctx->capacity)
        return true;

    size_t new_cap = ctx->capacity == 0 ? WRITER_INIT_CAPACITY : ctx->capacity;
    while (new_cap < needed)
        new_cap *= 2;

    char *new_buf = (char *)plistkit_realloc(ctx->buffer, new_cap);
    if (!new_buf) {
        ctx->failed = true;
        if (ctx->error) {
            ctx->error->code = PLISTKIT_ERR_NOMEM;
            snprintf(ctx->error->message, sizeof(ctx->error->message),
                     "Out of memory in writer");
        }
        return false;
    }

    ctx->buffer = new_buf;
    ctx->capacity = new_cap;
    return true;
}

static void
writer_append(writer_ctx_t *ctx, const char *str)
{
    size_t len = strlen(str);
    if (!writer_ensure(ctx, len))
        return;
    memcpy(ctx->buffer + ctx->size, str, len);
    ctx->size += len;
    ctx->buffer[ctx->size] = '\0';
}

static void
writer_append_char(writer_ctx_t *ctx, char c)
{
    if (!writer_ensure(ctx, 1))
        return;
    ctx->buffer[ctx->size++] = c;
    ctx->buffer[ctx->size] = '\0';
}

static void
writer_newline(writer_ctx_t *ctx)
{
    if (ctx->opts.pretty_print)
        writer_append_char(ctx, '\n');
}

static void
writer_indent(writer_ctx_t *ctx)
{
    if (!ctx->opts.pretty_print)
        return;
    int spaces = ctx->indent_level * ctx->opts.indent_size;
    for (int i = 0; i < spaces; i++)
        writer_append_char(ctx, ' ');
}

static void
writer_append_escaped(writer_ctx_t *ctx, const char *str)
{
    char *escaped = plistkit_xml_escape(str);
    if (!escaped) {
        ctx->failed = true;
        return;
    }
    writer_append(ctx, escaped);
    free(escaped);
}

/* ========================================================================== */
/* Node writing                                                                */
/* ========================================================================== */

static void write_node(writer_ctx_t *ctx, const plistkit_node_t *node);

static void
write_dict(writer_ctx_t *ctx, const plistkit_node_t *node)
{
    writer_append(ctx, "<dict>");
    writer_newline(ctx);
    ctx->indent_level++;

    for (size_t i = 0; i < node->value.dict_val.count; i++) {
        writer_indent(ctx);
        writer_append(ctx, "<key>");
        writer_append_escaped(ctx, node->value.dict_val.keys[i]);
        writer_append(ctx, "</key>");
        writer_newline(ctx);

        writer_indent(ctx);
        write_node(ctx, node->value.dict_val.values[i]);
        writer_newline(ctx);
    }

    ctx->indent_level--;
    writer_indent(ctx);
    writer_append(ctx, "</dict>");
}

static void
write_array(writer_ctx_t *ctx, const plistkit_node_t *node)
{
    writer_append(ctx, "<array>");
    writer_newline(ctx);
    ctx->indent_level++;

    for (size_t i = 0; i < node->value.array_val.count; i++) {
        writer_indent(ctx);
        write_node(ctx, node->value.array_val.items[i]);
        writer_newline(ctx);
    }

    ctx->indent_level--;
    writer_indent(ctx);
    writer_append(ctx, "</array>");
}

static void
write_node(writer_ctx_t *ctx, const plistkit_node_t *node)
{
    if (ctx->failed || !node)
        return;

    switch (node->type) {
    case PLISTKIT_TYPE_BOOLEAN:
        writer_append(ctx, node->value.boolean_val ? "<true/>" : "<false/>");
        break;

    case PLISTKIT_TYPE_INTEGER: {
        char buf[32];
        snprintf(buf, sizeof(buf), "<integer>%" PRId64 "</integer>",
                 node->value.integer_val);
        writer_append(ctx, buf);
        break;
    }

    case PLISTKIT_TYPE_REAL: {
        char buf[64];
        /* use %g to avoid trailing zeros, but ensure enough precision */
        snprintf(buf, sizeof(buf), "<real>%.17g</real>", node->value.real_val);
        writer_append(ctx, buf);
        break;
    }

    case PLISTKIT_TYPE_STRING:
        writer_append(ctx, "<string>");
        writer_append_escaped(ctx, node->value.string_val.ptr);
        writer_append(ctx, "</string>");
        break;

    case PLISTKIT_TYPE_DATA: {
        writer_append(ctx, "<data>");
        if (node->value.data_val.size > 0) {
            size_t enc_size = plistkit_base64_encoded_size(
                node->value.data_val.size);
            char *enc = (char *)malloc(enc_size + 1);
            if (!enc) {
                ctx->failed = true;
                return;
            }
            plistkit_base64_encode(node->value.data_val.ptr,
                                   node->value.data_val.size,
                                   enc, enc_size + 1);
            writer_append(ctx, enc);
            free(enc);
        }
        writer_append(ctx, "</data>");
        break;
    }

    case PLISTKIT_TYPE_DATE: {
        char buf[32];
        plistkit_error_t err = plistkit_date_format(
            node->value.date_val.timestamp, buf, sizeof(buf));
        if (err != PLISTKIT_OK) {
            ctx->failed = true;
            return;
        }
        writer_append(ctx, "<date>");
        writer_append(ctx, buf);
        writer_append(ctx, "</date>");
        break;
    }

    case PLISTKIT_TYPE_ARRAY:
        write_array(ctx, node);
        break;

    case PLISTKIT_TYPE_DICT:
        write_dict(ctx, node);
        break;

    default:
        break;
    }
}

/* ========================================================================== */
/* Public API                                                                  */
/* ========================================================================== */

char *
plistkit_write_string(const plistkit_node_t *node,
                      const plistkit_writer_opts_t *opts,
                      plistkit_error_ctx_t *error)
{
    if (!node) {
        if (error) {
            error->code = PLISTKIT_ERR_INVALID_ARG;
            snprintf(error->message, sizeof(error->message), "NULL node");
        }
        return NULL;
    }

    writer_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.error = error;

    if (opts) {
        ctx.opts = *opts;
    } else {
        ctx.opts = plistkit_writer_opts_default();
    }

    /* XML declaration */
    if (ctx.opts.xml_declaration) {
        writer_append(&ctx, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        writer_newline(&ctx);
    }

    /* DOCTYPE */
    if (ctx.opts.doctype) {
        writer_append(&ctx,
            "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
            "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">");
        writer_newline(&ctx);
    }

    /* <plist> */
    writer_append(&ctx, "<plist version=\"1.0\">");
    writer_newline(&ctx);

    /* root node */
    write_node(&ctx, node);
    writer_newline(&ctx);

    /* </plist> */
    writer_append(&ctx, "</plist>");
    writer_newline(&ctx);

    if (ctx.failed) {
        free(ctx.buffer);
        return NULL;
    }

    return ctx.buffer;
}

plistkit_error_t
plistkit_write_file(const plistkit_node_t *node, const char *path,
                    const plistkit_writer_opts_t *opts,
                    plistkit_error_ctx_t *error)
{
    if (!node || !path) {
        if (error) {
            error->code = PLISTKIT_ERR_INVALID_ARG;
            snprintf(error->message, sizeof(error->message),
                     "NULL node or path");
        }
        return PLISTKIT_ERR_INVALID_ARG;
    }

    char *xml = plistkit_write_string(node, opts, error);
    if (!xml)
        return error ? error->code : PLISTKIT_ERR_NOMEM;

    plistkit_error_t err = plistkit_file_write(path, xml, strlen(xml));
    free(xml);

    if (err != PLISTKIT_OK && error) {
        error->code = err;
        snprintf(error->message, sizeof(error->message),
                 "Failed to write file: %s", path);
    }

    return err;
}
