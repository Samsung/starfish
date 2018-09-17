{
    # build.gypi defines build options.
    # Update build.gypi if build options need to be modified

    'variables' : {
        'starfish_root': '.',
        'escargot_root': '<(starfish_root)/third_party/escargot',
        'third_party_libs': 'icu-uc icu-i18n',
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
            'STARFISH_ENABLE_HTTPCACHE',

            # 'STARFISH_ENABLE_TEST',
            # 'STARFISH_ENABLE_PROFILING',
            # 'STARFISH_ENABLE_NETWORK_PROFILING',
            # 'STARFISH_ENABLE_SCRIPT_PROFILING',
        ],
        # NOTE: common #defines for all Tizen platform
        'defines_tizen': [
            'STARFISH_TIZEN',
            'STARFISH_TIZEN_OBS',
            'STARFISH_ENABLE_DOMPARSER',
            'STARFISH_IGNORE_CROSS_ORIGIN',
            # 'STARFISH_ENABLE_MULTIMEDIA',
            'TIZEN_DEVICE_API',
            'SIZE_MAX=0xffffffff',
            #'STARFISH_IGNORE_SSL_VERIFYPEER',
            #'STARFISH_ENABLE_INSPECTOR',
            #'STARFISH_ENABLE_TEST',
            #'STARFISH_MEDIAPLAYER_DEBUG',
        ],
        # NOTE: specific #defines for each profile
        # e.g., defines used by the compiler = 'defines_tizen' + 'defines_unified_tv'
        'defines_unified_common': [
        ],
        'defines_unified_mobile': [
            #'STARFISH_ENABLE_MULTIMEDIA',
        ],
        'defines_unified_tv': [
            #'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_TIZEN_TV',
            'STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED',
            #'STARFISH_ENABLE_AVPLAY',
            'STARFISH_ENABLE_TRANSPARENT_WINDOW',
            #'STARFISH_ENABLE_TTS',
            #'STARFISH_ENABLE_BODY_FOCUS_RING',
            #'STARFISH_ENABLE_VIRTUAL_CURSOR',
            #'USE_PRODUCT_FEATURE',
        ],
        'defines_prod_tv': [
            'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_TIZEN_TV',
            'STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED',
            #'STARFISH_ENABLE_AVPLAY',
            'STARFISH_ENABLE_TRANSPARENT_WINDOW',
            'STARFISH_ENABLE_TTS',
            #'STARFISH_ENABLE_BODY_FOCUS_RING',
            #'STARFISH_ENABLE_VIRTUAL_CURSOR',
            'USE_PRODUCT_FEATURE',
        ],
        'defines_prod_tv_dali': [
            #'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_TIZEN_TV',
            'STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED',
            #'STARFISH_ENABLE_AVPLAY',
            'STARFISH_ENABLE_TRANSPARENT_WINDOW',
            'STARFISH_ENABLE_TTS',
            #'STARFISH_ENABLE_BODY_FOCUS_RING',
            #'STARFISH_ENABLE_VIRTUAL_CURSOR',
            'USE_PRODUCT_FEATURE',
        ],
        'defines_unified_wearable': [
            # 'STARFISH_TIZEN_WEARABLE',
            'STARFISH_TIZEN_WEARABLE_WIDGET',
            'STARFISH_TIZEN_TRANSPARENT_BACKGROUND',
            'STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED',
            'STARFISH_DISABLE_OVERFLOW_SCROLL',
            #'STARFISH_ENABLE_MULTIMEDIA',
        ],
        'defines_prod_wearable': [
            'STARFISH_TIZEN_WEARABLE_WIDGET',
            'STARFISH_TIZEN_TRANSPARENT_BACKGROUND',
            'STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED',
            'STARFISH_DISABLE_OVERFLOW_SCROLL',
            #'STARFISH_ENABLE_MULTIMEDIA',
        ],
        'defines_debug': [
            'GC_DEBUG', # bdwgc
            '_GLIBCXX_DEBUG',
            'STARFISH_ENABLE_TEST',
            #'STARFISH_ENABLE_NETWORK_TEST',
        ],
        'defines_release': [
            'NDEBUG',
        ],
        'cflags_default': [
            '-std=c++11',
            '-g3',
            '-Wall',
            '-Wextra',
            '-Werror',
            '-Wno-unused-parameter',
            '-Wno-unused-result',
            '-Wno-unused-variable',
            '-Wno-unused-function',
            '-Wno-deprecated-declarations',
            '-Wno-type-limits',
            '-fno-math-errno',
            '-fdata-sections',
            '-ffunction-sections',
            '-Wno-invalid-offsetof',
            '-fvisibility=hidden',
            '-fno-omit-frame-pointer',
            '-fstack-protector',
            '-fPIC',
        ],
        'cflags_debug': [
            '-O0',
            # '-fsanitize=address',
        ],
        'cflags_release': [
            '-O2',
        ],
        'libraries_default': [
            '<!@(pkg-config --libs-only-l <(third_party_libs))',
            '-Wl,-rpath=/usr/local/lib',
            '-lpthread',
            '-lcurl',
            '-lssl',
            '-lcrypto',
            # '-lasan', # for -fsanitize=address
        ],
        'include_dirs_default': [
           '<(starfish_root)/src',
           '<(starfish_root)/inc',
           #'<(starfish_root)/third_party/rapidxml',
           '<!@(pkg-config --cflags-only-I <(third_party_libs) | sed s/-I//g)',
           '<(escargot_root)/third_party/rapidjson/include',
        ],
        'main_file' : 'src/shell/shell.cpp',
        'test_runner_file' : 'src/shell/testRunner.cpp',
        'variables': {
            'variables': {
                'component%': 'static_library',
                'backend%': 'efl_cairo_gl',
                'platform%': 'linux',
                'profile%': 'none',
                'touchUi%': '1',
                'deplib%': 'shared_library',
                'compiler%': 'gcc',
            },
            'component%':'<(component)',
            'backend%': '<(backend)',
            'platform%': '<(platform)',
            'profile%': '<(profile)',
            'touchUi%': '1',
            'cflags_extra%': [],
            'include_dirs_extra': [],
            'deplib%': '<(deplib)',

            'conditions': [
                ['compiler=="gcc"', {
                    'cflags_compiler': [
                        '-frounding-math',
                        '-fsignaling-nans',
                        '-Wno-unused-but-set-variable',
                        '-Wno-unused-but-set-parameter',
                    ],
                    'libraries_compiler': [
                    ]
                }],
                ['compiler=="clang"', {
                    'cflags_compiler': [
                        '-fno-fast-math',
                        '-fno-unsafe-math-optimizations',
                        '-fdenormal-fp-math=ieee',
                        '-stdlib=libc++',
                        '-Wno-expansion-to-defined',
                        '-Wno-dynamic-class-memaccess',
                    ],
                    'libraries_compiler': [
                        '-stdlib=libc++',
                    ],
                }],
                ['platform=="linux"', {
                    'cflags_extra': [
                    ],
                    'include_dirs_extra': [
                    ],
                    'libraries_extra': [
                    ],
                }],
                ['platform=="tizen"', {
                    'include_dirs_extra': [
                        '<@(include_dirs_extra)',
                        '/usr/include/dlog',
                        '/usr/include/location',
                        'third_party/deviceapi/src',
                    ],
                    'cflags_extra': [
                    ],
                    'libraries_extra': [
                        '-lrt',
                        '-ldl',
                        '-lcapi-location-manager',
                    ],
                }],
                ['touchUi=="1"', {
                    'include_dirs_extra': [
                        '<@(include_dirs_extra)',
                        '/usr/include/location',
                    ],
                    'cflags_extra': [
                    ],
                    'libraries_extra': [
                        '-lcapi-location-manager',
                    ],
                }],
                ['backend=="efl_cairo"', {
                    'cflags_extra': [
                    ],
                    'include_dirs_extra': [
                    ],
                    'libraries_extra': [
                        '-ljpeg',
                        '-lgif',
                    ],
                }],
                ['backend=="efl_cairo_gl"', {
                    'cflags_extra': [
                    ],
                    'include_dirs_extra': [
                    ],
                    'libraries_extra': [
                        '-ljpeg',
                        '-lgif',
                    ],
                }],
                ['backend=="efl_skia"', {
                    'cflags_extra': [
                    ],
                    'include_dirs_extra': [
                    ],
                    'libraries_extra': [
                        '-ljpeg',
                        '-lgif',
                    ],
                }],
                ['backend=="dali"', {
                    'cflags_extra': [
                    ],
                    'include_dirs_extra': [
                    ],
                    'libraries_extra': [
                        '-ljpeg',
                        '-lgif',
                    ],
                }],
                ['backend!="dali"', {
                    'cflags_extra': [
                        # turn off rtti for all backends except dali
                        '-fno-rtti',
                    ],
                }],
                # profile: tv || wearable || mobile
                # Use the following template if build options need to be added
                # for each profile
                ['profile=="wearable"', {
                    'cflags_extra': [
                        '-Os',
                    ],
                    'include_dirs_extra': [
                        #'<@(include_dirs_extra)',
                    ],
                    'libraries_extra': [
                    ],
                }],
            ],
            'include_dirs_extra%': '<(include_dirs_extra)',
        },
        'component%':'<(component)',
        'backend%': '<(backend)',
        'platform%': '<(platform)',
        'profile%': '<(profile)',
        'deplib%': '<(deplib)',
        'libraries_compiler%': '<(libraries_compiler)',
        'cflags_compiler%': '<(cflags_compiler)',
        'include_dirs_extra%': '<(include_dirs_extra)',
        'libraries_extra%': '<(libraries_extra)',
        'cflags_extra%': '<(cflags_extra)',
        'defines_extra%': [],
        'deps_extra%': [],
        'deps_debug_extra%': [],
        'deps_release_extra%': [],
        'conditions': [
            ['platform=="tizen" and backend=="efl_cairo"', {
                'defines_extra': [
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                    '-Wno-format-nonliteral',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-Wl,-soname,liblightweight-web-engine.so.1',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl_cairo.tizen',
                    './build.dep.gyp:skia_matrix',
                ],
            }],
            ['platform=="linux" and backend=="efl_cairo_gl"', {
                'defines_extra': [
                    'STARFISH_EFL_CAIRO',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl_cairo.x64',
                    './build.dep.gyp:skia_matrix',
                ],
            }],
            ['platform=="tizen" and backend=="efl_cairo_gl"', {
                'defines_extra': [
                    'STARFISH_EFL_CAIRO',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                    '-Wno-format-nonliteral',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-Wl,-soname,liblightweight-web-engine.so.1',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl_cairo.tizen',
                    './build.dep.gyp:skia_matrix',
                ],
            }],
            ['platform=="linux" and backend=="efl_skia"', {
                'defines_extra': [
                    'STARFISH_EFL_SKIA',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl_skia.x64',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:libskia.x64.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:libskia.x64.release',
                ],
            }],
            ['platform=="linux" and backend=="glfw_cairo_gl"', {
                'defines_extra': [
                    'STARFISH_GLFW_CAIRO_GL',
                ],
                'cflags_extra': [
                    '<@(cflags_extra)',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:libtuv.x64.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:libtuv.x64.release',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-lGL',
                    '-lGLESv2',
                    '-lglfw',
                ],
                'deps_extra': [
                    './build.dep.gyp:glfw_cairo.x64',
                    './build.dep.gyp:skia_matrix',

                ],
            }],
            ['platform=="linux" and backend=="dali"', {
                'defines_extra': [
                    'STARFISH_DALI'
                ],
                'deps_extra': [
                    './build.dep.gyp:dali.x64',
                    './build.dep.gyp:skia_matrix',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:libtuv.x64.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:libtuv.x64.release',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                ],
            }],
            ['platform=="tizen" and backend=="dali"', {
                'defines_extra': [
                    'STARFISH_DALI'
                ],
                'cflags_extra': [
                    '-Wno-format-nonliteral',
                ],
                'deps_extra': [
                    './build.dep.gyp:dali.tizen',
                    './build.dep.gyp:skia_matrix',
                ],
                'deps_debug_extra': [
                    './build.dep.gyp:libtuv.tizen.debug',
                ],
                'deps_release_extra': [
                    './build.dep.gyp:libtuv.tizen.release',
                ],
                'libraries_extra': [
                    '<@(libraries_extra)',
                    '-lpthread',
                    '-Wl,-soname,liblightweight-web-engine-dali-plugin.so.1',
                ],
            }],
        ],
    },
}
