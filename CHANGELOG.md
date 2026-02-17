# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-02-17

### Added
- Initial release
- XML plist parsing from files, strings, and memory buffers
- XML plist generation with pretty-print and compact modes
- Full type support: string, integer, real, boolean, date, data, array, dict
- Tree-based API: create, free, copy, compare nodes
- Getters and setters for all data types
- Array operations: append, insert, remove, clear
- Dictionary operations: set, get, remove, clear, get_keys
- Configurable security limits (depth, file size, string length, etc.)
- PLISTKIT_NO_LIMITS flag to disable all limits
- Custom lightweight XML parser (zero external dependencies)
- Meson build system with static and shared library support
- pkg-config file generation
- Comprehensive test suite
- Example programs: simple_read, simple_write, manipulation
