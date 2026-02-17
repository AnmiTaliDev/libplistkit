/**
 * @file test_array.c
 * @brief Tests for array operations.
 */

#include "test.h"

TEST(array_create_empty)
{
    plistkit_node_t *a = plistkit_node_new_array();
    ASSERT_NOT_NULL(a);
    ASSERT(plistkit_node_is_type(a, PLISTKIT_TYPE_ARRAY));

    size_t sz;
    ASSERT_OK(plistkit_array_get_size(a, &sz));
    ASSERT_EQ(sz, (size_t)0);

    plistkit_node_free(a);
    DONE();
}

TEST(array_append_get)
{
    plistkit_node_t *a = plistkit_node_new_array();

    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_string("first")));
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_string("second")));
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_integer(3)));

    size_t sz;
    ASSERT_OK(plistkit_array_get_size(a, &sz));
    ASSERT_EQ(sz, (size_t)3);

    plistkit_node_t *item = NULL;
    ASSERT_OK(plistkit_array_get_item(a, 0, &item));
    const char *str;
    ASSERT_OK(plistkit_node_get_string(item, &str));
    ASSERT_STR_EQ(str, "first");

    ASSERT_OK(plistkit_array_get_item(a, 2, &item));
    int64_t val;
    ASSERT_OK(plistkit_node_get_integer(item, &val));
    ASSERT_EQ(val, (int64_t)3);

    plistkit_node_free(a);
    DONE();
}

TEST(array_insert)
{
    plistkit_node_t *a = plistkit_node_new_array();

    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_integer(1)));
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_integer(3)));
    ASSERT_OK(plistkit_array_insert(a, 1, plistkit_node_new_integer(2)));

    size_t sz;
    ASSERT_OK(plistkit_array_get_size(a, &sz));
    ASSERT_EQ(sz, (size_t)3);

    plistkit_node_t *item = NULL;
    int64_t val;

    ASSERT_OK(plistkit_array_get_item(a, 0, &item));
    ASSERT_OK(plistkit_node_get_integer(item, &val));
    ASSERT_EQ(val, (int64_t)1);

    ASSERT_OK(plistkit_array_get_item(a, 1, &item));
    ASSERT_OK(plistkit_node_get_integer(item, &val));
    ASSERT_EQ(val, (int64_t)2);

    ASSERT_OK(plistkit_array_get_item(a, 2, &item));
    ASSERT_OK(plistkit_node_get_integer(item, &val));
    ASSERT_EQ(val, (int64_t)3);

    plistkit_node_free(a);
    DONE();
}

TEST(array_remove)
{
    plistkit_node_t *a = plistkit_node_new_array();

    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_string("a")));
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_string("b")));
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_string("c")));

    ASSERT_OK(plistkit_array_remove(a, 1));

    size_t sz;
    ASSERT_OK(plistkit_array_get_size(a, &sz));
    ASSERT_EQ(sz, (size_t)2);

    plistkit_node_t *item = NULL;
    const char *str;

    ASSERT_OK(plistkit_array_get_item(a, 0, &item));
    ASSERT_OK(plistkit_node_get_string(item, &str));
    ASSERT_STR_EQ(str, "a");

    ASSERT_OK(plistkit_array_get_item(a, 1, &item));
    ASSERT_OK(plistkit_node_get_string(item, &str));
    ASSERT_STR_EQ(str, "c");

    plistkit_node_free(a);
    DONE();
}

TEST(array_clear)
{
    plistkit_node_t *a = plistkit_node_new_array();
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_integer(1)));
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_integer(2)));
    ASSERT_OK(plistkit_array_clear(a));

    size_t sz;
    ASSERT_OK(plistkit_array_get_size(a, &sz));
    ASSERT_EQ(sz, (size_t)0);

    plistkit_node_free(a);
    DONE();
}

TEST(array_out_of_bounds)
{
    plistkit_node_t *a = plistkit_node_new_array();
    ASSERT_OK(plistkit_array_append(a, plistkit_node_new_integer(1)));

    plistkit_node_t *item = NULL;
    ASSERT_EQ(plistkit_array_get_item(a, 5, &item),
              PLISTKIT_ERR_INDEX_OUT_OF_RANGE);
    ASSERT_EQ(plistkit_array_remove(a, 5),
              PLISTKIT_ERR_INDEX_OUT_OF_RANGE);

    plistkit_node_free(a);
    DONE();
}

TEST(array_type_mismatch)
{
    plistkit_node_t *s = plistkit_node_new_string("not an array");
    size_t sz;
    ASSERT_EQ(plistkit_array_get_size(s, &sz), PLISTKIT_ERR_TYPE_MISMATCH);
    plistkit_node_free(s);
    DONE();
}

TEST(node_deep_copy)
{
    plistkit_node_t *dict = plistkit_node_new_dict();
    ASSERT_OK(plistkit_dict_set_value(dict, "a",
              plistkit_node_new_string("hello")));

    plistkit_node_t *arr = plistkit_node_new_array();
    ASSERT_OK(plistkit_array_append(arr, plistkit_node_new_integer(1)));
    ASSERT_OK(plistkit_array_append(arr, plistkit_node_new_integer(2)));
    ASSERT_OK(plistkit_dict_set_value(dict, "arr", arr));

    plistkit_node_t *copy = plistkit_node_copy(dict);
    ASSERT_NOT_NULL(copy);
    ASSERT(plistkit_node_equal(dict, copy));

    /* mutate original, copy should be unaffected */
    ASSERT_OK(plistkit_dict_set_value(dict, "a",
              plistkit_node_new_string("modified")));
    ASSERT(!plistkit_node_equal(dict, copy));

    plistkit_node_free(dict);
    plistkit_node_free(copy);
    DONE();
}

void register_array_tests(void)
{
    REGISTER(array_create_empty);
    REGISTER(array_append_get);
    REGISTER(array_insert);
    REGISTER(array_remove);
    REGISTER(array_clear);
    REGISTER(array_out_of_bounds);
    REGISTER(array_type_mismatch);
    REGISTER(node_deep_copy);
}
