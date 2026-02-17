examples = ['simple_read', 'simple_write', 'manipulation']

foreach name : examples
  executable(name, name + '.c',
    dependencies: libplistkit_dep,
    build_by_default: true,
    install: false,
  )
endforeach
