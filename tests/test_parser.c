/**
 * @file test_parser.c
 * @brief Tests for the XML plist parser.
 */

#include "test.h"

TEST(parse_simple_dict)
{
    const char *xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>name</key>\n"
        "  <string>libplistkit</string>\n"
        "  <key>version</key>\n"
        "  <integer>1</integer>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, NULL, &err);
    ASSERT_NOT_NULL(root);
    ASSERT(plistkit_node_is_type(root, PLISTKIT_TYPE_DICT));

    size_t sz;
    ASSERT_OK(plistkit_dict_get_size(root, &sz));
    ASSERT_EQ(sz, (size_t)2);

    plistkit_node_t *name = NULL;
    ASSERT_OK(plistkit_dict_get_value(root, "name", &name));
    const char *str;
    ASSERT_OK(plistkit_node_get_string(name, &str));
    ASSERT_STR_EQ(str, "libplistkit");

    plistkit_node_t *ver = NULL;
    ASSERT_OK(plistkit_dict_get_value(root, "version", &ver));
    int64_t val;
    ASSERT_OK(plistkit_node_get_integer(ver, &val));
    ASSERT_EQ(val, (int64_t)1);

    plistkit_node_free(root);
    DONE();
}

TEST(parse_all_types)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>bool_t</key><true/>\n"
        "  <key>bool_f</key><false/>\n"
        "  <key>int</key><integer>42</integer>\n"
        "  <key>neg</key><integer>-100</integer>\n"
        "  <key>pi</key><real>3.14159</real>\n"
        "  <key>str</key><string>hello</string>\n"
        "  <key>data</key><data>AQID</data>\n"
        "  <key>date</key><date>2026-01-01T00:00:00Z</date>\n"
        "  <key>arr</key>\n"
        "  <array>\n"
        "    <integer>1</integer>\n"
        "    <integer>2</integer>\n"
        "  </array>\n"
        "  <key>nested</key>\n"
        "  <dict>\n"
        "    <key>inner</key>\n"
        "    <string>value</string>\n"
        "  </dict>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);

    /* booleans */
    plistkit_node_t *v = NULL;
    bool bval;
    ASSERT_OK(plistkit_dict_get_value(root, "bool_t", &v));
    ASSERT_OK(plistkit_node_get_boolean(v, &bval));
    ASSERT(bval == true);

    ASSERT_OK(plistkit_dict_get_value(root, "bool_f", &v));
    ASSERT_OK(plistkit_node_get_boolean(v, &bval));
    ASSERT(bval == false);

    /* integers */
    int64_t ival;
    ASSERT_OK(plistkit_dict_get_value(root, "int", &v));
    ASSERT_OK(plistkit_node_get_integer(v, &ival));
    ASSERT_EQ(ival, (int64_t)42);

    ASSERT_OK(plistkit_dict_get_value(root, "neg", &v));
    ASSERT_OK(plistkit_node_get_integer(v, &ival));
    ASSERT_EQ(ival, (int64_t)-100);

    /* real */
    double dval;
    ASSERT_OK(plistkit_dict_get_value(root, "pi", &v));
    ASSERT_OK(plistkit_node_get_real(v, &dval));
    ASSERT(fabs(dval - 3.14159) < 0.0001);

    /* string */
    const char *sval;
    ASSERT_OK(plistkit_dict_get_value(root, "str", &v));
    ASSERT_OK(plistkit_node_get_string(v, &sval));
    ASSERT_STR_EQ(sval, "hello");

    /* data */
    const void *ddata;
    size_t dsz;
    ASSERT_OK(plistkit_dict_get_value(root, "data", &v));
    ASSERT_OK(plistkit_node_get_data(v, &ddata, &dsz));
    ASSERT_EQ(dsz, (size_t)3);
    const uint8_t *bytes = (const uint8_t *)ddata;
    ASSERT_EQ(bytes[0], (uint8_t)1);
    ASSERT_EQ(bytes[1], (uint8_t)2);
    ASSERT_EQ(bytes[2], (uint8_t)3);

    /* date */
    int64_t ts;
    ASSERT_OK(plistkit_dict_get_value(root, "date", &v));
    ASSERT_OK(plistkit_node_get_date(v, &ts));
    ASSERT(ts > 0);

    /* array */
    ASSERT_OK(plistkit_dict_get_value(root, "arr", &v));
    size_t asz;
    ASSERT_OK(plistkit_array_get_size(v, &asz));
    ASSERT_EQ(asz, (size_t)2);

    /* nested dict */
    ASSERT_OK(plistkit_dict_get_value(root, "nested", &v));
    ASSERT(plistkit_node_is_type(v, PLISTKIT_TYPE_DICT));

    plistkit_node_free(root);
    DONE();
}

TEST(parse_empty_plist)
{
    const char *xml = "<plist version=\"1.0\"></plist>";
    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);
    ASSERT(plistkit_node_is_type(root, PLISTKIT_TYPE_DICT));
    plistkit_node_free(root);
    DONE();
}

TEST(parse_empty_containers)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>empty_arr</key><array></array>\n"
        "  <key>empty_dict</key><dict></dict>\n"
        "  <key>empty_str</key><string></string>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);

    plistkit_node_t *v = NULL;
    size_t sz;

    ASSERT_OK(plistkit_dict_get_value(root, "empty_arr", &v));
    ASSERT_OK(plistkit_array_get_size(v, &sz));
    ASSERT_EQ(sz, (size_t)0);

    ASSERT_OK(plistkit_dict_get_value(root, "empty_dict", &v));
    ASSERT_OK(plistkit_dict_get_size(v, &sz));
    ASSERT_EQ(sz, (size_t)0);

    ASSERT_OK(plistkit_dict_get_value(root, "empty_str", &v));
    const char *str;
    ASSERT_OK(plistkit_node_get_string(v, &str));
    ASSERT_STR_EQ(str, "");

    plistkit_node_free(root);
    DONE();
}

TEST(parse_malformed_no_plist)
{
    const char *xml = "<dict><key>x</key><string>y</string></dict>";
    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, NULL, &err);
    ASSERT_NULL(root);
    ASSERT_EQ(err.code, PLISTKIT_ERR_PARSE);
    DONE();
}

TEST(parse_malformed_unclosed)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>x</key>\n"
        "  <string>y\n";
    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, NULL, &err);
    ASSERT_NULL(root);
    DONE();
}

TEST(parse_null_input)
{
    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(NULL, NULL, &err);
    ASSERT_NULL(root);
    ASSERT_EQ(err.code, PLISTKIT_ERR_INVALID_ARG);

    root = plistkit_parse_mem(NULL, 0, NULL, &err);
    ASSERT_NULL(root);
    DONE();
}

TEST(parse_array_root)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<array>\n"
        "  <string>a</string>\n"
        "  <string>b</string>\n"
        "</array>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);
    ASSERT(plistkit_node_is_type(root, PLISTKIT_TYPE_ARRAY));

    size_t sz;
    ASSERT_OK(plistkit_array_get_size(root, &sz));
    ASSERT_EQ(sz, (size_t)2);

    plistkit_node_free(root);
    DONE();
}

TEST(parse_with_comments)
{
    const char *xml =
        "<!-- header comment -->\n"
        "<plist version=\"1.0\">\n"
        "<!-- inside -->\n"
        "<dict>\n"
        "  <!-- key comment -->\n"
        "  <key>x</key>\n"
        "  <integer>1</integer>\n"
        "</dict>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);

    int64_t val;
    plistkit_node_t *v = NULL;
    ASSERT_OK(plistkit_dict_get_value(root, "x", &v));
    ASSERT_OK(plistkit_node_get_integer(v, &val));
    ASSERT_EQ(val, (int64_t)1);

    plistkit_node_free(root);
    DONE();
}

void register_parser_tests(void)
{
    REGISTER(parse_simple_dict);
    REGISTER(parse_all_types);
    REGISTER(parse_empty_plist);
    REGISTER(parse_empty_containers);
    REGISTER(parse_malformed_no_plist);
    REGISTER(parse_malformed_unclosed);
    REGISTER(parse_null_input);
    REGISTER(parse_array_root);
    REGISTER(parse_with_comments);
}
