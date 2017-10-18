{
    # build.gypi defines build options.
    # Update build.gypi if build options need to be modified

    'variables' : {
        'starfish_root': '.',
        'escargot_root': '<(starfish_root)/third_party/escargot',
        'third_party_libs': 'libpng cairo freetype2 fontconfig icu-uc icu-i18n harfbuzz harfbuzz-icu',
        'defines_default': [
            'ESCARGOT_ENABLE_TYPEDARRAY=1',
            'ESCARGOT_ENABLE_PROMISE=1',
        ],
        'defines_x64': [
            'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_ENABLE_INSPECTOR',
            'STARFISH_ENABLE_DOMPARSER',
            'STARFISH_ENABLE_TTS',
            'STARFISH_IGNORE_CROSS_ORIGIN',
        ],
        'defines_tizen': [
            'STARFISH_TIZEN',
            'STARFISH_TIZEN_OBS',
            'STARFISH_TIZEN_TV',
            'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_ENABLE_DOMPARSER',
            'STARFISH_ENABLE_TTS',
            'STARFISH_ENABLE_TEST',
            'STARFISH_ENABLE_BODY_FOCUS_RING',
            #'STARFISH_ENABLE_VIRTUAL_CURSOR',
            'STARFISH_IGNORE_CROSS_ORIGIN',
            'STARFISH_FRAME_REPLACED_VIDEO_NEEDS_GRAPHICS_BUFFER=false',
            'TIZEN_DEVICE_API',
            'USE_PRODUCT_FEATURE',
            'SIZE_MAX=0xffffffff',
        ],
        'defines_debug': [
            'GC_DEBUG', # bdwgc
            '_GLIBCXX_DEBUG',
            'STARFISH_ENABLE_TEST',
            #'STARFISH_ENABLE_NETWORK_TEST',
        ],
        'defines_release': [
            'NDEBUG',
            'STARFISH_ENABLE_TEST',
        ],
        'cflags_default': [
            '-std=c++11',
            '-Wall',
            '-Wextra',
            '-Wno-unused-but-set-variable',
            '-Wno-unused-but-set-parameter',
            '-Wno-unused-parameter',
            '-Wno-unused-result',
            '-Wno-unused-variable',
            '-Wno-unused-function',
            '-Wno-deprecated-declarations',
            '-Wno-type-limits',
            '-fno-math-errno',
            '-fdata-sections',
            '-ffunction-sections',
            '-frounding-math',
            '-fsignaling-nans',
            '-Wno-invalid-offsetof',
            '-fvisibility=hidden',
            '-fno-omit-frame-pointer',
            '-fstack-protector',
            '-fPIC',
            #'-fuse-ld=gold', # for fast linking
        ],
        'cflags_debug': [
            '-O0',
            '-g3',
            '-Werror',
        ],
        'cflags_release': [
            '-O2',
            '-g3',
        ],
        'libraries_default': [
            '<!@(pkg-config --libs-only-l <(third_party_libs))',
            '-Wl,-rpath=/usr/local/lib',
            '-lpthread',
            '-lcurl',
            '-lssl',
            '-lcrypto',
        ],
        'include_dirs_default': [
           '<(starfish_root)/src',
           '<(starfish_root)/inc',
           #'<(starfish_root)/third_party/rapidxml',
           '<!@(pkg-config --cflags-only-I <(third_party_libs) | sed s/-I//g)',
        ],
        'code_gen_results' : ['<!@(python binding_generator/scripts/starfish_code_generator.py src/ src/binding/)',],
        'main_file' : 'src/shell/shell.cpp',
        'variables': {
            'variables': {
                'component%': 'static_library',
                'backend%': 'efl',
                'enable_ffmpeg_demuxer%': 'false',
                'platform%': 'linux',
                'deplib%': 'shared_library',
            },
            'component%':'<(component)',
            'backend%': '<(backend)',
            'enable_ffmpeg_demuxer%': '<(enable_ffmpeg_demuxer)',
            'platform%': '<(platform)',
            'cflags_extra%': [],
            'deplib%': '<(deplib)',

            'conditions': [
                ['platform=="tizen"', {
                    'include_dirs_extra': [
                        'third_party/deviceapi/src',
                        '/usr/include/dlog',
                        '/usr/include/location',
                    ],
                    'sources_extra': [
                        '<!@(find third_party/deviceapi/src -name *.cpp)',
                    ],
                    'libraries_extra': [
                        '-lrt',
                        '-ldl',
                        '-lavformat -lavcodec -lavutil',
                        '-ldivxdrm',
                        '-lcapi-location-manager',
                    ],
                }],
                ['platform=="linux"', {
                    'cflags_extra': [
                    ],
                    'include_dirs_extra': [
                    ],
                    'sources_extra': [
                    ],
                    'libraries_extra': [
                    ],
                }],
            ],
        },
        'component%':'<(component)',
        'backend%': '<(backend)',
        'platform%': '<(platform)',
        'deplib%': '<(deplib)',
        'enable_ffmpeg_demuxer%': '<(enable_ffmpeg_demuxer)',
        'include_dirs_extra%': '<(include_dirs_extra)',
        'sources_extra%': '<(sources_extra)',
        'libraries_extra%': '<(libraries_extra)',
        'cflags_extra%': '<(cflags_extra)',
        'defines_extra%': [],
        'deps_extra%': [],
        'deps_debug_extra%': [],
        'deps_release_extra%': [],
        'conditions': [
            ['platform=="linux" and backend=="efl"', {
                'defines_extra': [
                    'STARFISH_EFL',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                    '-fno-rtti',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl.x64',
                ],
            }],
            ['platform=="tizen" and backend=="efl"', {
                'defines_extra': [
                    'STARFISH_EFL',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                    '-fno-rtti',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl.tizen',
                ],
            }],
            ['platform=="linux" and backend=="efl_cairo"', {
                'defines_extra': [
                    'STARFISH_EFL_CAIRO',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                    '-fno-rtti',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-lturbojpeg',
                    '-lgif',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl_cairo.x64',
                ],
            }],
            ['platform=="tizen" and backend=="efl_cairo"', {
                'defines_extra': [
                    'STARFISH_EFL_CAIRO',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                    '-fno-rtti',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-lturbojpeg',
                    '-lgif',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl_cairo.tizen',
                ],
            }],
            ['platform=="linux" and backend=="dali"', {
                'defines_extra': [
                    'STARFISH_DALI','GC_THREADS'
                ],
                'deps_extra': [
                    './build.dep.gyp:dali.x64',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:libtuv.x64.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:libtuv.x64.release',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-lturbojpeg',
                    '-lgif',
                ],
            }],
            ['platform=="tizen" and backend=="dali"', {
                'defines_extra': [
                    'STARFISH_DALI','GC_THREADS'
                ],
                'deps_extra': [
                    './build.dep.gyp:dali.tizen',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:libtuv.tizen.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:libtuv.tizen.release',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-lturbojpeg',
                    '-lgif',
                    '-pthread',
                ],
            }],
            ['enable_ffmpeg_demuxer=="true"', {
                'defines_extra': [
                    'STARFISH_ENABLE_FFMPEG_DEMUXER',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:av.x64.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:av.x64.release',
                ],
            }],
        ],
    },
}
