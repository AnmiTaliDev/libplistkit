/**
 * @file test_writer.c
 * @brief Tests for the XML plist writer.
 */

#include "test.h"

TEST(write_simple_dict)
{
    plistkit_node_t *dict = plistkit_node_new_dict();
    ASSERT_NOT_NULL(dict);

    ASSERT_OK(plistkit_dict_set_value(dict, "name",
              plistkit_node_new_string("test")));
    ASSERT_OK(plistkit_dict_set_value(dict, "count",
              plistkit_node_new_integer(42)));

    char *xml = plistkit_write_string(dict, NULL, NULL);
    ASSERT_NOT_NULL(xml);

    ASSERT(strstr(xml, "<plist version=\"1.0\">") != NULL);
    ASSERT(strstr(xml, "<key>name</key>") != NULL);
    ASSERT(strstr(xml, "<string>test</string>") != NULL);
    ASSERT(strstr(xml, "<key>count</key>") != NULL);
    ASSERT(strstr(xml, "<integer>42</integer>") != NULL);
    ASSERT(strstr(xml, "</dict>") != NULL);
    ASSERT(strstr(xml, "</plist>") != NULL);

    free(xml);
    plistkit_node_free(dict);
    DONE();
}

TEST(write_compact_mode)
{
    plistkit_node_t *dict = plistkit_node_new_dict();
    ASSERT_OK(plistkit_dict_set_value(dict, "k",
              plistkit_node_new_string("v")));

    plistkit_writer_opts_t opts = plistkit_writer_opts_default();
    opts.pretty_print = false;
    opts.xml_declaration = false;
    opts.doctype = false;

    char *xml = plistkit_write_string(dict, &opts, NULL);
    ASSERT_NOT_NULL(xml);

    /* should be a single line, no newlines between elements */
    ASSERT(strstr(xml, "<plist version=\"1.0\"><dict>") != NULL);

    free(xml);
    plistkit_node_free(dict);
    DONE();
}

TEST(write_all_types)
{
    plistkit_node_t *dict = plistkit_node_new_dict();

    ASSERT_OK(plistkit_dict_set_value(dict, "bool",
              plistkit_node_new_boolean(true)));
    ASSERT_OK(plistkit_dict_set_value(dict, "int",
              plistkit_node_new_integer(-99)));
    ASSERT_OK(plistkit_dict_set_value(dict, "real",
              plistkit_node_new_real(2.718)));
    ASSERT_OK(plistkit_dict_set_value(dict, "str",
              plistkit_node_new_string("hello")));
    ASSERT_OK(plistkit_dict_set_value(dict, "data",
              plistkit_node_new_data("abc", 3)));
    ASSERT_OK(plistkit_dict_set_value(dict, "date",
              plistkit_node_new_date(1000000000)));

    plistkit_node_t *arr = plistkit_node_new_array();
    ASSERT_OK(plistkit_array_append(arr, plistkit_node_new_integer(1)));
    ASSERT_OK(plistkit_dict_set_value(dict, "arr", arr));

    char *xml = plistkit_write_string(dict, NULL, NULL);
    ASSERT_NOT_NULL(xml);

    ASSERT(strstr(xml, "<true/>") != NULL);
    ASSERT(strstr(xml, "<integer>-99</integer>") != NULL);
    ASSERT(strstr(xml, "<real>") != NULL);
    ASSERT(strstr(xml, "<string>hello</string>") != NULL);
    ASSERT(strstr(xml, "<data>") != NULL);
    ASSERT(strstr(xml, "<date>") != NULL);
    ASSERT(strstr(xml, "<array>") != NULL);

    free(xml);
    plistkit_node_free(dict);
    DONE();
}

TEST(roundtrip_parse_write_parse)
{
    const char *xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>name</key>\n"
        "  <string>roundtrip</string>\n"
        "  <key>count</key>\n"
        "  <integer>7</integer>\n"
        "  <key>flag</key>\n"
        "  <true/>\n"
        "  <key>items</key>\n"
        "  <array>\n"
        "    <string>a</string>\n"
        "    <string>b</string>\n"
        "  </array>\n"
        "</dict>\n"
        "</plist>\n";

    /* parse */
    plistkit_node_t *root1 = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root1);

    /* write */
    char *xml2 = plistkit_write_string(root1, NULL, NULL);
    ASSERT_NOT_NULL(xml2);

    /* parse again */
    plistkit_node_t *root2 = plistkit_parse_string(xml2, NULL, NULL);
    ASSERT_NOT_NULL(root2);

    /* compare */
    ASSERT(plistkit_node_equal(root1, root2));

    free(xml2);
    plistkit_node_free(root1);
    plistkit_node_free(root2);
    DONE();
}

TEST(write_null_node)
{
    plistkit_error_ctx_t err = {0};
    char *xml = plistkit_write_string(NULL, NULL, &err);
    ASSERT_NULL(xml);
    ASSERT_EQ(err.code, PLISTKIT_ERR_INVALID_ARG);
    DONE();
}

void register_writer_tests(void)
{
    REGISTER(write_simple_dict);
    REGISTER(write_compact_mode);
    REGISTER(write_all_types);
    REGISTER(roundtrip_parse_write_parse);
    REGISTER(write_null_node);
}
