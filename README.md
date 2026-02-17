# libplistkit

A lightweight C11 library for parsing and generating Apple XML property list (plist) files.

**Zero external dependencies.** Custom built-in XML parser handles the plist subset natively.

## Features

- Parse XML plist from files, strings, or memory buffers
- Generate pretty-printed or compact XML plist output
- Full plist type support: string, integer, real, boolean, date, data, array, dict
- Tree-based API with getters, setters, deep copy, and deep comparison
- Configurable security limits (max depth, max file size, max string length)
- Cross-platform: Linux, macOS, Windows (POSIX + Win32 abstractions)
- LGPL-3.0-or-later license

## Building

Requires: [Meson](https://mesonbuild.com/) >= 0.56, a C11 compiler, Ninja.

```sh
meson setup build
meson compile -C build
meson test -C build
```

To install system-wide:

```sh
sudo meson install -C build
```

### Build options

| Option     | Default | Description           |
|------------|---------|-----------------------|
| `tests`    | `true`  | Build test suite      |
| `examples` | `true`  | Build example programs|

```sh
meson setup build -Dtests=false -Dexamples=false
```

## Quick start

### Reading a plist

```c
#include <plistkit/plistkit.h>

plistkit_error_ctx_t err = {0};
plistkit_node_t *root = plistkit_parse_file("Info.plist", NULL, &err);
if (!root) {
    fprintf(stderr, "Error: %s (line %d)\n", err.message, err.line);
    return 1;
}

plistkit_node_t *name = NULL;
plistkit_dict_get_value(root, "CFBundleName", &name);

const char *str;
plistkit_node_get_string(name, &str);
printf("Name: %s\n", str);

plistkit_node_free(root);
```

### Creating a plist

```c
#include <plistkit/plistkit.h>

plistkit_node_t *root = plistkit_node_new_dict();
plistkit_dict_set_value(root, "AppName", plistkit_node_new_string("MyApp"));
plistkit_dict_set_value(root, "Version", plistkit_node_new_integer(1));

plistkit_write_file(root, "output.plist", NULL, NULL);
plistkit_node_free(root);
```

## Security

The parser enforces configurable limits by default:

| Limit             | Default   |
|-------------------|-----------|
| Max file size     | 100 MB    |
| Max nesting depth | 100       |
| Max string length | 10 MB     |
| Max data size     | 100 MB    |
| Max array items   | 1,000,000 |
| Max dict entries  | 1,000,000 |

Override via `plistkit_config_t`, or disable all limits with `PLISTKIT_NO_LIMITS` flag.

XML entity expansion is not supported, providing protection against billion laughs attacks.

## API Reference

See `include/plistkit/plistkit.h` for the complete API with Doxygen-compatible documentation.

## License

LGPL-3.0-or-later. See [LICENSE](LICENSE) for details.

## Author

AnmiTaliDev <anmitalidev@nuros.org>
