project('klatr', 'cpp', default_options: [ 'cpp_std=c++17' ])

vkom_subproject = subproject('vkom')
kom_subproject = subproject('kom')
sdl3_subproject = subproject('sdl3')

vkom = dependency('vkom', version: '0.1.0', required: true)
kom = dependency('kom', version: '1.0.0', required: true)
sdl3 = dependency('sdl3', version: '>=3.2.0', required: true)

klatr_sources = [
    'src/main.cpp'
]

klatr_includes = include_directories('src')
klatr_dependencies = [ vkom, kom, sdl3 ]
klatr_override_options = [ 'cpp_std=c++17' ]

klatr = executable('klatr',
    sources: klatr_sources,
    include_directories: klatr_includes,
    dependencies: klatr_dependencies,
    override_options: klatr_override_options,
)
