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
    'src/klatr/platform.hpp',

    # klatr main
    'src/klatr/object.hpp', 'src/klatr/object.cpp',
    'src/klatr/klatr.hpp', 'src/klatr/klatr.cpp',

    # klatr audio headers
    'src/klatr/audio/audio.hpp',
    'src/klatr/audio/enums.hpp',

    # klatr audio
    'src/klatr/audio/instance.hpp', 'src/klatr/audio/instance.cpp',
    'src/klatr/audio/adapter.hpp',
    'src/klatr/audio/device.hpp',
    'src/klatr/audio/buffer.hpp',

    # klatr WASAPI audio implementation
    'src/klatr/audio/wasapi/enums.hpp',
    'src/klatr/audio/wasapi/wasapi.hpp',
    'src/klatr/audio/wasapi/instance.hpp', 'src/klatr/audio/wasapi/instance.cpp',
    'src/klatr/audio/wasapi/adapter.hpp', 'src/klatr/audio/wasapi/adapter.cpp',
    'src/klatr/audio/wasapi/device.hpp', 'src/klatr/audio/wasapi/device.cpp',
    'src/klatr/audio/wasapi/buffer.hpp', 'src/klatr/audio/wasapi/buffer.cpp',

    # klatr CoreAudio audio implementation
    'src/klatr/audio/coreaudio/enums.hpp',
    'src/klatr/audio/coreaudio/coreaudio.hpp',
    'src/klatr/audio/coreaudio/instance.hpp', 'src/klatr/audio/coreaudio/instance.cpp',
    'src/klatr/audio/coreaudio/adapter.hpp', 'src/klatr/audio/coreaudio/adapter.cpp',
    'src/klatr/audio/coreaudio/device.hpp', 'src/klatr/audio/coreaudio/device.cpp',
    'src/klatr/audio/coreaudio/buffer.hpp', 'src/klatr/audio/coreaudio/buffer.cpp',

    # klatr ALSA audio implementation
    'src/klatr/audio/alsa/enums.hpp',
    'src/klatr/audio/alsa/alsa.hpp',
    'src/klatr/audio/alsa/instance.hpp', 'src/klatr/audio/alsa/instance.cpp',
    'src/klatr/audio/alsa/adapter.hpp', 'src/klatr/audio/alsa/adapter.cpp',
    'src/klatr/audio/alsa/device.hpp', 'src/klatr/audio/alsa/device.cpp',
    'src/klatr/audio/alsa/buffer.hpp', 'src/klatr/audio/alsa/buffer.cpp',

    # klatr gpu
    'src/klatr/gpu/gpu.hpp',
    'src/klatr/gpu/context.hpp', 'src/klatr/gpu/context.cpp',

    # klatr gpu audio
    'src/klatr/gpu/audio/endpoint.hpp', 'src/klatr/gpu/audio/endpoint.cpp',
    'src/klatr/gpu/audio/graph.hpp', 'src/klatr/gpu/audio/graph.cpp',
    'src/klatr/gpu/audio/module.hpp', 'src/klatr/gpu/audio/module.cpp',

    # klatr gpu audio sdk
    'src/klatr/gpu/audio/sdk/glsl.hpp',
    'src/klatr/gpu/audio/sdk/hlsl.hpp',

    # klatr gpu graphics
    'src/klatr/gpu/graphics/sprite.hpp', 'src/klatr/gpu/graphics/sprite.cpp',
    'src/klatr/gpu/graphics/text.hpp', 'src/klatr/gpu/graphics/text.cpp',

    # klatr gpu shader
    'src/klatr/gpu/shader/compiler.hpp', 'src/klatr/gpu/shader/compiler.cpp',
    'src/klatr/gpu/shader/shader.hpp', 'src/klatr/gpu/shader/shader.cpp',

    # klatr ui
    'src/klatr/ui/ui.hpp', 'src/klatr/ui/ui.cpp',

    # klatr window
    'src/klatr/window/window.hpp', 'src/klatr/window/window.cpp',
]

klatr_includes = include_directories('src')
klatr_dependencies = [ vkom, kom, sdl3 ]
klatr_override_options = [ 'cpp_std=c++17' ]

cc = meson.get_compiler('c')

if host_machine.system() == 'windows'
    klatr_platform_specific_dependencies = [
        # WASAPI and Windows COM dependencies
        cc.find_library('ole32'),
        cc.find_library('uuid'),
        cc.find_library('winmm'),
    ]
elif host_machine.system() == 'darwin'
    klatr_platform_specific_dependencies = [
        dependency('appleframeworks', modules: [ 'CoreAudio', 'CoreFoundation', 'AudioUnit' ])
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
