/**
 * @file simple_write.c
 * @brief Example: creating a plist programmatically and writing it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <plistkit/plistkit.h>

int main(void)
{
    plistkit_node_t *root = plistkit_node_new_dict();

    plistkit_dict_set_value(root, "AppName",
                            plistkit_node_new_string("MyApp"));
    plistkit_dict_set_value(root, "Version",
                            plistkit_node_new_integer(1));
    plistkit_dict_set_value(root, "Debug",
                            plistkit_node_new_boolean(false));
    plistkit_dict_set_value(root, "Scale",
                            plistkit_node_new_real(1.5));

    plistkit_node_t *features = plistkit_node_new_array();
    plistkit_array_append(features, plistkit_node_new_string("dark-mode"));
    plistkit_array_append(features, plistkit_node_new_string("auto-save"));
    plistkit_dict_set_value(root, "Features", features);

    /* print to stdout */
    char *xml = plistkit_write_string(root, NULL, NULL);
    if (xml) {
        printf("%s", xml);
        free(xml);
    }

    /* write to file */
    plistkit_error_ctx_t err = {0};
    if (plistkit_write_file(root, "output.plist", NULL, &err) != PLISTKIT_OK)
        fprintf(stderr, "Write error: %s\n", err.message);
    else
        printf("Written to output.plist\n");

    plistkit_node_free(root);
    return 0;
}
