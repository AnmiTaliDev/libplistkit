/**
 * @file test_dict.c
 * @brief Tests for dictionary operations.
 */

#include "test.h"

TEST(dict_create_empty)
{
    plistkit_node_t *d = plistkit_node_new_dict();
    ASSERT_NOT_NULL(d);
    ASSERT(plistkit_node_is_type(d, PLISTKIT_TYPE_DICT));

    size_t sz;
    ASSERT_OK(plistkit_dict_get_size(d, &sz));
    ASSERT_EQ(sz, (size_t)0);

    plistkit_node_free(d);
    DONE();
}

TEST(dict_set_get)
{
    plistkit_node_t *d = plistkit_node_new_dict();

    ASSERT_OK(plistkit_dict_set_value(d, "key1",
              plistkit_node_new_string("val1")));
    ASSERT_OK(plistkit_dict_set_value(d, "key2",
              plistkit_node_new_integer(42)));

    size_t sz;
    ASSERT_OK(plistkit_dict_get_size(d, &sz));
    ASSERT_EQ(sz, (size_t)2);

    ASSERT(plistkit_dict_has_key(d, "key1"));
    ASSERT(plistkit_dict_has_key(d, "key2"));
    ASSERT(!plistkit_dict_has_key(d, "key3"));

    plistkit_node_t *v = NULL;
    ASSERT_OK(plistkit_dict_get_value(d, "key1", &v));
    const char *str;
    ASSERT_OK(plistkit_node_get_string(v, &str));
    ASSERT_STR_EQ(str, "val1");

    plistkit_node_free(d);
    DONE();
}

TEST(dict_overwrite)
{
    plistkit_node_t *d = plistkit_node_new_dict();

    ASSERT_OK(plistkit_dict_set_value(d, "k",
              plistkit_node_new_string("old")));
    ASSERT_OK(plistkit_dict_set_value(d, "k",
              plistkit_node_new_string("new")));

    size_t sz;
    ASSERT_OK(plistkit_dict_get_size(d, &sz));
    ASSERT_EQ(sz, (size_t)1);

    plistkit_node_t *v = NULL;
    ASSERT_OK(plistkit_dict_get_value(d, "k", &v));
    const char *str;
    ASSERT_OK(plistkit_node_get_string(v, &str));
    ASSERT_STR_EQ(str, "new");

    plistkit_node_free(d);
    DONE();
}

TEST(dict_remove)
{
    plistkit_node_t *d = plistkit_node_new_dict();

    ASSERT_OK(plistkit_dict_set_value(d, "a",
              plistkit_node_new_integer(1)));
    ASSERT_OK(plistkit_dict_set_value(d, "b",
              plistkit_node_new_integer(2)));
    ASSERT_OK(plistkit_dict_set_value(d, "c",
              plistkit_node_new_integer(3)));

    ASSERT_OK(plistkit_dict_remove_key(d, "b"));

    size_t sz;
    ASSERT_OK(plistkit_dict_get_size(d, &sz));
    ASSERT_EQ(sz, (size_t)2);

    ASSERT(!plistkit_dict_has_key(d, "b"));
    ASSERT(plistkit_dict_has_key(d, "a"));
    ASSERT(plistkit_dict_has_key(d, "c"));

    /* remove non-existent */
    ASSERT_EQ(plistkit_dict_remove_key(d, "zzz"), PLISTKIT_ERR_NOT_FOUND);

    plistkit_node_free(d);
    DONE();
}

TEST(dict_clear)
{
    plistkit_node_t *d = plistkit_node_new_dict();

    ASSERT_OK(plistkit_dict_set_value(d, "x",
              plistkit_node_new_string("y")));
    ASSERT_OK(plistkit_dict_clear(d));

    size_t sz;
    ASSERT_OK(plistkit_dict_get_size(d, &sz));
    ASSERT_EQ(sz, (size_t)0);

    plistkit_node_free(d);
    DONE();
}

TEST(dict_get_keys)
{
    plistkit_node_t *d = plistkit_node_new_dict();

    ASSERT_OK(plistkit_dict_set_value(d, "alpha",
              plistkit_node_new_integer(1)));
    ASSERT_OK(plistkit_dict_set_value(d, "beta",
              plistkit_node_new_integer(2)));

    char **keys = NULL;
    size_t count = 0;
    ASSERT_OK(plistkit_dict_get_keys(d, &keys, &count));
    ASSERT_EQ(count, (size_t)2);
    ASSERT_NOT_NULL(keys);

    /* order is insertion order */
    ASSERT_STR_EQ(keys[0], "alpha");
    ASSERT_STR_EQ(keys[1], "beta");

    for (size_t i = 0; i < count; i++)
        free(keys[i]);
    free(keys);

    plistkit_node_free(d);
    DONE();
}

TEST(dict_type_mismatch)
{
    plistkit_node_t *s = plistkit_node_new_string("not a dict");
    size_t sz;
    ASSERT_EQ(plistkit_dict_get_size(s, &sz), PLISTKIT_ERR_TYPE_MISMATCH);
    plistkit_node_free(s);
    DONE();
}

void register_dict_tests(void)
{
    REGISTER(dict_create_empty);
    REGISTER(dict_set_get);
    REGISTER(dict_overwrite);
    REGISTER(dict_remove);
    REGISTER(dict_clear);
    REGISTER(dict_get_keys);
    REGISTER(dict_type_mismatch);
}
