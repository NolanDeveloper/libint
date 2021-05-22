project(
  'libint',
  'c',
  version: '0.1',
  default_options: ['warning_level=3', 'werror=true', 'c_std=c11']
)

include_directories = include_directories('include')

install_headers('include/libint.h')

cc = meson.get_compiler('c')

libm_dep = cc.find_library('m', required: false)

libis_dep = dependency('is', required: false)
if not libis_dep.found()
  libis_proj = subproject('libis')
  libis_dep = libis_proj.get_variable('libis_dep')
endif

dependencies = [
  libis_dep,
  libm_dep,
]

src = [
  'src/libint_signed.c',
  'src/libint_unsigned.c',
  'src/libint_utils.c',
]

libint = library(
  'int',
  src,
  install: true,
  dependencies: dependencies,
  include_directories: include_directories
)

pkg = import('pkgconfig')
pkg.generate(libint)

libint_dep = declare_dependency(
  include_directories: include_directories,
  link_with: libint,
)

libint_test_unit = executable(
  'libint_test_unit',
  ['test-unit/main.c'],
  link_with: libint,
  dependencies: dependencies,
  include_directories: include_directories
)

test('unit test', libint_test_unit)

