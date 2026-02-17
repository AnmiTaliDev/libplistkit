/**
 * @file test_security.c
 * @brief Security-related tests: limits, malformed input.
 */

#include "test.h"

TEST(security_depth_limit)
{
    /* Create a deeply nested array: <array><array>...<integer>1</integer>...</array></array> */
    /* depth of 5 with limit of 3 should fail */
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<array><array><array><array><array>"
        "<integer>1</integer>"
        "</array></array></array></array></array>\n"
        "</plist>\n";

    plistkit_config_t cfg = plistkit_config_default();
    cfg.max_depth = 3;

    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, &cfg, &err);
    ASSERT_NULL(root);
    ASSERT_EQ(err.code, PLISTKIT_ERR_LIMIT_EXCEEDED);
    DONE();
}

TEST(security_depth_ok)
{
    /* depth of 2 with limit of 100 should pass */
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<array><array><integer>1</integer></array></array>\n"
        "</plist>\n";

    plistkit_node_t *root = plistkit_parse_string(xml, NULL, NULL);
    ASSERT_NOT_NULL(root);
    plistkit_node_free(root);
    DONE();
}

TEST(security_file_size_limit)
{
    /* simulate a large file by setting a very small limit */
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict><key>x</key><string>value</string></dict>\n"
        "</plist>\n";

    plistkit_config_t cfg = plistkit_config_default();
    cfg.max_file_size = 10; /* 10 bytes — way too small */

    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, &cfg, &err);
    ASSERT_NULL(root);
    ASSERT_EQ(err.code, PLISTKIT_ERR_LIMIT_EXCEEDED);
    DONE();
}

TEST(security_no_limits_flag)
{
    /* with PLISTKIT_NO_LIMITS, even small max_depth should be ignored */
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<array><array><array><array><array>"
        "<integer>1</integer>"
        "</array></array></array></array></array>\n"
        "</plist>\n";

    plistkit_config_t cfg = plistkit_config_default();
    cfg.max_depth = 2;
    cfg.flags = PLISTKIT_NO_LIMITS;

    plistkit_node_t *root = plistkit_parse_string(xml, &cfg, NULL);
    ASSERT_NOT_NULL(root);
    plistkit_node_free(root);
    DONE();
}

TEST(security_unknown_tag)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict><key>x</key><foobar>y</foobar></dict>\n"
        "</plist>\n";

    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, NULL, &err);
    ASSERT_NULL(root);
    ASSERT_EQ(err.code, PLISTKIT_ERR_PARSE);
    DONE();
}

TEST(security_mismatched_tags)
{
    const char *xml =
        "<plist version=\"1.0\">\n"
        "<dict><key>x</key><string>y</integer></dict>\n"
        "</plist>\n";

    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_string(xml, NULL, &err);
    ASSERT_NULL(root);
    DONE();
}

void register_security_tests(void)
{
    REGISTER(security_depth_limit);
    REGISTER(security_depth_ok);
    REGISTER(security_file_size_limit);
    REGISTER(security_no_limits_flag);
    REGISTER(security_unknown_tag);
    REGISTER(security_mismatched_tags);
}
