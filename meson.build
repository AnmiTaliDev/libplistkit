project('libplistkit', 'c',
  version: '0.1.0',
  license: 'LGPL-3.0-or-later',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'default_library=both',
  ],
)

version_arr = meson.project_version().split('.')
version_major = version_arr[0].to_int()
version_minor = version_arr[1].to_int()
version_patch = version_arr[2].to_int()

conf = configuration_data()
conf.set('VERSION', meson.project_version())
conf.set('VERSION_MAJOR', version_major)
conf.set('VERSION_MINOR', version_minor)
conf.set('VERSION_PATCH', version_patch)

version_h = configure_file(
  input: 'include/plistkit/version.h.in',
  output: 'version.h',
  configuration: conf,
  install: true,
  install_dir: get_option('includedir') / 'plistkit',
)

inc = include_directories('include')

sources = files(
  'src/plist.c',
  'src/array.c',
  'src/dict.c',
  'src/parser.c',
  'src/writer.c',
  'src/types.c',
  'src/utf8.c',
  'src/platform.c',
)

# Math library needed on some systems for ceil/floor
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

libplistkit = library('plistkit',
  sources,
  include_directories: inc,
  dependencies: [m_dep],
  version: meson.project_version(),
  soversion: version_major,
  install: true,
)

install_headers(
  'include/plistkit/plistkit.h',
  subdir: 'plistkit',
)

pkg = import('pkgconfig')
pkg.generate(
  libplistkit,
  name: 'libplistkit',
  description: 'Lightweight C library for XML plist files',
  url: 'https://github.com/AnmiTaliDev/libplistkit',
  filebase: 'plistkit',
  subdirs: 'plistkit',
)

libplistkit_dep = declare_dependency(
  link_with: libplistkit,
  include_directories: inc,
)

if get_option('tests')
  subdir('tests')
endif

if get_option('examples')
  subdir('examples')
endif
