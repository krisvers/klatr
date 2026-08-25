project('klatr',
    [ 'c', 'cpp' ],
    license: 'MIT',
    version: '0.0.2',
    default_options: [ 'cpp_std=c++17' ]
)

vkom_subproject = subproject('vkom')
kom_subproject = subproject('kom')
sdl3_subproject = subproject('sdl3')

vkom = dependency('vkom', version: '0.1.2', required: true)
kom = dependency('kom', version: '1.0.1', required: true)
sdl3 = dependency('sdl3', version: '>=3.2.0', required: true)

klatr_sources = [
    # bootstrap
    'src/main.cpp',

    # klatr headers
    'src/klatr/enums.hpp',
    'src/klatr/object.hpp',
    'src/klatr/platform.hpp',

    # klatr main
    'src/klatr/klatr.hpp', 'src/klatr/klatr.cpp',

    # klatr audio headers
    'src/klatr/audio/audio.hpp',
    'src/klatr/audio/enums.hpp',

    # klatr audio
    'src/klatr/audio/instance.hpp', 'src/klatr/audio/instance.cpp',

    # WASPI audio implementation
    'src/klatr/audio/internal/wasapi/instance.hpp', 'src/klatr/audio/internal/wasapi/instance.cpp',

    # CoreAudio audio implementation
    'src/klatr/audio/internal/coreaudio/instance.hpp', 'src/klatr/audio/internal/coreaudio/instance.cpp',

    # ALSA audio implementation
    'src/klatr/audio/internal/alsa/instance.hpp', 'src/klatr/audio/internal/alsa/instance.cpp',
]

klatr_includes = include_directories('src')
klatr_dependencies = [ vkom, kom, sdl3 ]
klatr_override_options = [ 'cpp_std=c++17' ]

cc = meson.get_compiler('c')

if host_machine.system() == 'windows'
    klatr_platform_specific_dependencies = [
        # WASAPI and Windows COM dependencies
        cc.find_library('ole32'),
        cc.find_library('uuid')
    ]
elif host_machine.system() == 'darwin'
    klatr_platform_specific_dependencies = [
        dependency('appleframeworks', modules: [ 'CoreAudio' ])
    ]
elif host_machine.system() == 'linux'
    klatr_platform_specific_dependencies = [
        dependency('alsa', version: '>=1.0.0')
    ]
else
    error('Unsupported platform')
endif

klatr = executable('klatr',
    sources: klatr_sources,
    include_directories: klatr_includes,
    dependencies: klatr_dependencies + klatr_platform_specific_dependencies,
    override_options: klatr_override_options,
)
