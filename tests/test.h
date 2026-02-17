/**
 * @file test.h
 * @brief Lightweight test framework macros for libplistkit.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef PLISTKIT_TEST_H
#define PLISTKIT_TEST_H

#include <plistkit/plistkit.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_TESTS 512

typedef void (*test_func_t)(void);

typedef struct {
    const char  *name;
    test_func_t  func;
} test_entry_t;

/* global counters (defined in test_main.c) */
extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;
extern int g_assert_count;

void test_register(const char *name, test_func_t func);

/* registration functions for each test file */
void register_parser_tests(void);
void register_writer_tests(void);
void register_types_tests(void);
void register_dict_tests(void);
void register_array_tests(void);
void register_security_tests(void);

/* ========================================================================== */
/* Macros                                                                      */
/* ========================================================================== */

#define TEST(name) \
    static void test_##name(void)

#define REGISTER(name) \
    test_register(#name, test_##name)

#define PASS() \
    do { g_tests_passed++; printf("  [PASS]\n"); return; } while (0)

#define FAIL(msg) \
    do { \
        g_tests_failed++; \
        printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
        return; \
    } while (0)

#define ASSERT(expr) \
    do { \
        g_assert_count++; \
        if (!(expr)) { FAIL("Assertion failed: " #expr); } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        g_assert_count++; \
        if ((a) != (b)) { FAIL("Expected " #a " == " #b); } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        g_assert_count++; \
        if ((a) == (b)) { FAIL("Expected " #a " != " #b); } \
    } while (0)

#define ASSERT_NULL(p) \
    do { \
        g_assert_count++; \
        if ((p) != NULL) { FAIL("Expected NULL: " #p); } \
    } while (0)

#define ASSERT_NOT_NULL(p) \
    do { \
        g_assert_count++; \
        if ((p) == NULL) { FAIL("Expected non-NULL: " #p); } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        g_assert_count++; \
        if (strcmp((a), (b)) != 0) { \
            printf("  [FAIL] %s:%d: \"%s\" != \"%s\"\n", \
                   __FILE__, __LINE__, (a), (b)); \
            g_tests_failed++; return; \
        } \
    } while (0)

#define ASSERT_OK(err) \
    do { \
        g_assert_count++; \
        if ((err) != PLISTKIT_OK) { \
            printf("  [FAIL] %s:%d: expected OK, got %s\n", \
                   __FILE__, __LINE__, plistkit_error_string(err)); \
            g_tests_failed++; return; \
        } \
    } while (0)

/* must be called at the end of every test that didn't FAIL */
#define DONE() PASS()

#endif /* PLISTKIT_TEST_H */
