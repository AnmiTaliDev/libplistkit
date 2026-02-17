/**
 * @file simple_read.c
 * @brief Example: reading and inspecting a plist file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <plistkit/plistkit.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.plist>\n", argv[0]);
        return 1;
    }

    plistkit_error_ctx_t error = {0};
    plistkit_node_t *root = plistkit_parse_file(argv[1], NULL, &error);

    if (!root) {
        fprintf(stderr, "Parse error: %s (line %d, col %d)\n",
                error.message, error.line, error.column);
        return 1;
    }

    printf("Successfully parsed: %s\n", argv[1]);
    printf("Root type: %s\n", plistkit_type_name(plistkit_node_get_type(root)));

    if (plistkit_node_is_type(root, PLISTKIT_TYPE_DICT)) {
        char **keys = NULL;
        size_t count = 0;

        if (plistkit_dict_get_keys(root, &keys, &count) == PLISTKIT_OK) {
            printf("Keys (%zu):\n", count);
            for (size_t i = 0; i < count; i++) {
                plistkit_node_t *val = NULL;
                plistkit_dict_get_value(root, keys[i], &val);
                printf("  %s : %s\n", keys[i],
                       plistkit_type_name(plistkit_node_get_type(val)));
                free(keys[i]);
            }
            free(keys);
        }
    }

    plistkit_node_free(root);
    return 0;
}
