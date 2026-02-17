/**
 * @file manipulation.c
 * @brief Example: reading a plist, modifying it, writing it back.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <plistkit/plistkit.h>

int main(int argc, char **argv)
{
    const char *input = (argc > 1) ? argv[1] : "input.plist";
    const char *output = (argc > 2) ? argv[2] : "modified.plist";

    plistkit_error_ctx_t err = {0};
    plistkit_node_t *root = plistkit_parse_file(input, NULL, &err);
    if (!root) {
        fprintf(stderr, "Failed to parse %s: %s\n", input, err.message);
        return 1;
    }

    if (!plistkit_node_is_type(root, PLISTKIT_TYPE_DICT)) {
        fprintf(stderr, "Root is not a dictionary\n");
        plistkit_node_free(root);
        return 1;
    }

    /* add or overwrite a timestamp */
    plistkit_dict_set_value(root, "LastModified",
                            plistkit_node_new_date((int64_t)time(NULL)));

    /* increment a counter if it exists, or create it */
    plistkit_node_t *counter = NULL;
    int64_t count = 0;
    if (plistkit_dict_get_value(root, "EditCount", &counter) == PLISTKIT_OK) {
        plistkit_node_get_integer(counter, &count);
    }
    plistkit_dict_set_value(root, "EditCount",
                            plistkit_node_new_integer(count + 1));

    /* write out */
    if (plistkit_write_file(root, output, NULL, &err) != PLISTKIT_OK) {
        fprintf(stderr, "Failed to write %s: %s\n", output, err.message);
        plistkit_node_free(root);
        return 1;
    }

    printf("Modified plist written to %s\n", output);
    plistkit_node_free(root);
    return 0;
}
