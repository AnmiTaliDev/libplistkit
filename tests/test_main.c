/**
 * @file test_main.c
 * @brief Lightweight test framework and runner for libplistkit.
 *
 * Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "test.h"

#include <stdio.h>

/* test registration */
static test_entry_t tests[MAX_TESTS];
static int test_count = 0;

/* global counters */
int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;
int g_assert_count = 0;

void
test_register(const char *name, test_func_t func)
{
    if (test_count < MAX_TESTS) {
        tests[test_count].name = name;
        tests[test_count].func = func;
        test_count++;
    }
}

int
main(void)
{
    printf("=== libplistkit test suite ===\n\n");

    /* call all registration functions */
    register_parser_tests();
    register_writer_tests();
    register_types_tests();
    register_dict_tests();
    register_array_tests();
    register_security_tests();

    /* run all tests */
    for (int i = 0; i < test_count; i++) {
        g_tests_run++;
        printf("  [RUN ] %s\n", tests[i].name);
        tests[i].func();
    }

    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           g_tests_passed, g_tests_failed, g_tests_run);

    return g_tests_failed > 0 ? 1 : 0;
}
