test_sources = files(
  'test_main.c',
  'test_parser.c',
  'test_writer.c',
  'test_types.c',
  'test_dict.c',
  'test_array.c',
  'test_security.c',
)

test_inc = include_directories('.')

test_exe = executable('plistkit_tests',
  test_sources,
  include_directories: test_inc,
  dependencies: libplistkit_dep,
)

test('libplistkit-tests', test_exe)
