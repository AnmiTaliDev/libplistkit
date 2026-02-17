/**
 * @file test_types.c
 * @brief Tests for base64, date, XML escape/unescape.
 */

#include "test.h"
#include <plistkit/plistkit.h>

/* ---- base64 ---- */

TEST(base64_encode_hello)
{
    plistkit_node_t *d = plistkit_node_new_data("Hello", 5);
    ASSERT_NOT_NULL(d);

    const void *out;
    size_t sz;
    ASSERT_OK(plistkit_node_get_data(d, &out, &sz));
    ASSERT_EQ(sz, (size_t)5);
    ASSERT(memcmp(out, "Hello", 5) == 0);

    plistkit_node_free(d);
    DONE();
}

TEST(base64_roundtrip)
{
    /* Build a plist with <data>, write it, parse it back */
    const char *xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>bin</key>\n"
        "  <data>SGVsbG8gV29ybGQ=</data>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);

    plistkit_node_t *val = NULL;
    ASSERT_OK(plistkit_dict_get_value(root, "bin", &val));

    const void *data;
    size_t sz;
    ASSERT_OK(plistkit_node_get_data(val, &data, &sz));
    ASSERT_EQ(sz, (size_t)11);
    ASSERT(memcmp(data, "Hello World", 11) == 0);

    plistkit_node_free(root);
    DONE();
}

/* ---- date ---- */

TEST(date_parse_roundtrip)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>ts</key>\n"
        "  <date>2026-02-16T12:00:00Z</date>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);

    plistkit_node_t *val = NULL;
    ASSERT_OK(plistkit_dict_get_value(root, "ts", &val));

    int64_t ts;
    ASSERT_OK(plistkit_node_get_date(val, &ts));
    ASSERT(ts > 0);

    /* write back and check the date is preserved */
    char *out = plistkit_write_string(root, NULL, NULL);
    ASSERT_NOT_NULL(out);
    ASSERT(strstr(out, "2026-02-16T12:00:00Z") != NULL);

    free(out);
    plistkit_node_free(root);
    DONE();
}

/* ---- xml escape ---- */

TEST(xml_escape_in_string)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>html</key>\n"
        "  <string>&lt;b&gt;bold &amp; italic&lt;/b&gt;</string>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);

    plistkit_node_t *val = NULL;
    ASSERT_OK(plistkit_dict_get_value(root, "html", &val));

    const char *str;
    ASSERT_OK(plistkit_node_get_string(val, &str));
    ASSERT_STR_EQ(str, "<b>bold & italic</b>");

    /* write back — should re-escape */
    char *out = plistkit_write_string(root, NULL, NULL);
    ASSERT_NOT_NULL(out);
    ASSERT(strstr(out, "&lt;b&gt;") != NULL);
    ASSERT(strstr(out, "&amp;") != NULL);

    free(out);
    plistkit_node_free(root);
    DONE();
}

/* ---- version ---- */

TEST(version_string)
{
    const char *v = plistkit_version();
    ASSERT_NOT_NULL(v);
    ASSERT_STR_EQ(v, "0.1.0");

    int major, minor, patch;
    plistkit_version_get(&major, &minor, &patch);
    ASSERT_EQ(major, 0);
    ASSERT_EQ(minor, 1);
    ASSERT_EQ(patch, 0);
    DONE();
}

/* ---- error strings ---- */

TEST(error_strings)
{
    ASSERT_STR_EQ(plistkit_error_string(PLISTKIT_OK), "Success");
    ASSERT_STR_EQ(plistkit_error_string(PLISTKIT_ERR_NOMEM), "Out of memory");
    ASSERT_STR_EQ(plistkit_type_name(PLISTKIT_TYPE_STRING), "string");
    ASSERT_STR_EQ(plistkit_type_name(PLISTKIT_TYPE_DICT), "dict");
    DONE();
}

/* ---- registration ---- */

void register_types_tests(void)
{
    REGISTER(base64_encode_hello);
    REGISTER(base64_roundtrip);
    REGISTER(date_parse_roundtrip);
    REGISTER(xml_escape_in_string);
    REGISTER(version_string);
    REGISTER(error_strings);
}
