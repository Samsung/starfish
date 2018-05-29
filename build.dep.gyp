{
    # build.dep.gyp defines third party libraries.
    # Update build.dep.gyp if third party libraries are to be added

    'includes': [
        'build/build.gypi',
    ],
    'variables' : {
        'variables': {
            'backend%': 'efl',
        },
    },
    #'make_global_settings': [
    #    ['CXX', '/usr/bin/g++'],
    #],
    'target_defaults' : {
       'include_dirs': [
       ],
       'sources': [
       ],
       'link_settings': {
       },
       'conditions': [
           ['OS=="linux"', {
               'cflags' : [
                   '<@(cflags_default)',
                   '<@(cflags_compiler)',
                   '<@(cflags_extra)',
               ],
               'cflags!' : [
                   '-fvisibility=hidden',
               ],
               'ldflags' : [
                   '<@(libraries_compiler)',
               ],
               'defines': [
               ],
               'include_dirs': [
               ],
               'configurations': {
                   'debug' : {
                       'defines': [
                           '<@(defines_debug)',
                       ],
                       'cflags': [
                           '<@(cflags_debug)',
                       ],
                   },
                   'release': {
                       'defines': [
                           '<@(defines_release)',
                       ],
                       'cflags': [
                           '<@(cflags_release)',
                       ],
                   },
               },
           }],
       ],
    },
    'targets': [
        # Libraries built with our gyp
        {
            'target_name': 'clipper',
            'type': '<(deplib)',
            'dependencies': [
            ],
            'include_dirs': [
                'third_party/clipper/cpp',
            ],
            'sources': [
                'third_party/clipper/cpp/clipper.cpp',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/clipper/cpp',
                ],
            },
        },
        {
            'target_name': 'js_binding',
            'type': 'none',
            'dependencies': [
            ],
            'actions': [
                {
                    'action_name': 'generate_code',
                    'inputs': [
                        'binding_generator/scripts/starfish_code_generator.py',
                        #'.git/modules/binding_generator/HEAD',
                        '<!@(find src -name *.idl)',
                    ],
                    'outputs': [
                        '<!@(find src/binding -name *Binding.cpp)'
                    ],
                    'action': ['python', 'binding_generator/scripts/starfish_code_generator.py', 'src/', 'src/binding/'],
                },
            ],
        },
        {
            'target_name': 'skia',
            'type': '<(deplib)',
            'dependencies': [
            ],
            'include_dirs': [
                'third_party/skia_matrix',
            ],
            'sources': [
                'third_party/skia_matrix/SkMath.cpp',
                'third_party/skia_matrix/SkPoint.cpp',
                'third_party/skia_matrix/SkRect.cpp',
                'third_party/skia_matrix/SkMatrix.cpp',
                'third_party/skia_matrix/SkDebug.cpp',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/skia_matrix',
                ],
            },
        },
        {
            'target_name': 'mp4parse',
            'type': '<(deplib)',
            'dependencies': [
            ],
            'include_dirs': [
                'third_party/MP4Parse/source/include',
            ],
            'sources': [
                '<!@(find third_party/MP4Parse/source -name MP4*.cpp)',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/MP4Parse/source/include',
                ],
            },
        },
        {
            'target_name': 'webm',
            'type': '<(deplib)',
            'dependencies': [
            ],
            'include_dirs': [
                'third_party/webm',
            ],
            'sources': [
                'third_party/webm/mkvparser/mkvparser.cc',
                'third_party/webm/webvtt/webvttparser.cc',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/webm',
                ],
            },
        },
        {
            'target_name': 'cppzmq',
            'type': 'none',
            'dependencies': [
            ],
            'include_dirs': [
            ],
            'sources': [
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/cppzmq',
                ],
            },
        },
        # Libraries built with their own Makefiles
        {
            'target_name': 'escargot.x64.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/escargot/out/linux/x64/interpreter/release/libescargot.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib',
                },
            ],
            'direct_dependent_settings': {
                'libraries': [
                    'lib/libescargot.a',
                ],
            },
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/include',
                ],
            },
        },
        {
            'target_name': 'escargot.x64.debug',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/escargot/out/linux/x64/interpreter/debug/libescargot.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../debug/lib',
                },
            ],
            'direct_dependent_settings': {
                'libraries': [
                    'lib/libescargot.a',
                ],
            },
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/include',
                ],
            },
        },
        {
            'target_name': 'escargot.tizen.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/escargot/libescargot.a',
                    ],
                    # To workaround the 'duplicate-target' in ninja
                    'destination': '<(PRODUCT_DIR)/../release/lib/tizen',
                },
            ],
            'direct_dependent_settings': {
                'libraries': [
                    'lib/tizen/libescargot.a',
                ],
            },
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/include',
                ],
            },
        },
        {
            'target_name': 'zmq.x64.debug',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/zeromq/out/linux/x64/debug.shared/.libs/libzmq.so',
                        'third_party/zeromq/out/linux/x64/debug.shared/.libs/libzmq.so.5',
                        'third_party/zeromq/out/linux/x64/debug.shared/.libs/libzmq.so.5.0.1',
                    ],
                    'destination': '<(PRODUCT_DIR)/../debug/lib',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/zeromq/include',
                ],
                'libraries': [
                    'lib/libzmq.so',
                ],
            },
        },
        {
            'target_name': 'zmq.x64.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/zeromq/out/linux/x64/release.shared/.libs/libzmq.so',
                        'third_party/zeromq/out/linux/x64/release.shared/.libs/libzmq.so.5',
                        'third_party/zeromq/out/linux/x64/release.shared/.libs/libzmq.so.5.0.1',
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/zeromq/include',
                ],
                'libraries': [
                    'lib/libzmq.so',
                ],
            },
        },
        {
            'target_name': 'zmq.tizen',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'tizen_dep/arm/libzmq.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib/tizen',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/zeromq/include',
                ],
                'libraries': [
                    'lib/tizen/libzmq.a',
                ],
            },
        },
        {
            'target_name': 'gc.x64.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/escargot/third_party/GCutil/bdwgc/out/linux/x64/release.shared/.libs/libgc.so',
                        'third_party/escargot/third_party/GCutil/bdwgc/out/linux/x64/release.shared/.libs/libgc.so.1',
                        'third_party/escargot/third_party/GCutil/bdwgc/out/linux/x64/release.shared/.libs/libgc.so.1.0.3',
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib',
                },
            ],
            'include_dirs': [
                'third_party/escargot/third_party/GCutil/bdwgc/include/',
            ],
            'sources': [
                '<!@(find third_party/escargot/third_party/GCutil -maxdepth 1 -name *.cpp)',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/third_party/GCutil/bdwgc/include',
                    'third_party/escargot/third_party/GCutil',
                ],
            },
            'direct_dependent_settings': {
                'libraries': [
                    'lib/libgc.so',
                ],
            },
        },
        {
            'target_name': 'gc.x64.debug',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/escargot/third_party/GCutil/bdwgc/out/linux/x64/debug.shared/.libs/libgc.so',
                        'third_party/escargot/third_party/GCutil/bdwgc/out/linux/x64/debug.shared/.libs/libgc.so.1',
                        'third_party/escargot/third_party/GCutil/bdwgc/out/linux/x64/debug.shared/.libs/libgc.so.1.0.3',
                    ],
                    'destination': '<(PRODUCT_DIR)/../debug/lib',
                },
            ],
            'include_dirs': [
                'third_party/escargot/third_party/GCutil/bdwgc/include/',
            ],
            'sources': [
                '<!@(find third_party/escargot/third_party/GCutil -maxdepth 1 -name *.cpp)',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/third_party/GCutil/bdwgc/include',
                    'third_party/escargot/third_party/GCutil',
                ],
            },
            'direct_dependent_settings': {
                'libraries': [
                    'lib/libgc.so',
                ],
            },
        },
        {
            'target_name': 'gc.tizen.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        './third_party/escargot/third_party/GCutil/bdwgc/out/tizen_obs/arm/release.shared/.libs/libgc.a'
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib/tizen',
                },
            ],
            'include_dirs': [
                'third_party/escargot/third_party/GCutil/bdwgc/include/',
            ],
            'sources': [
                '<!@(find third_party/escargot/third_party/GCutil -maxdepth 1 -name *.cpp)',
            ],
            'direct_dependent_settings': {
                'libraries': [
                    'lib/tizen/libgc.a',
                ],
            },
            'all_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/third_party/GCutil/bdwgc/include',
                    'third_party/escargot/third_party/GCutil',
                ],
            },
        },
        {
            'target_name': 'efl.x64',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas) || true)',
                ],
            },
        },
        {
            'target_name': 'efl.tizen',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf-evas efl-extension | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf-evas efl-extension) || true)',
                ],
            },
        },
        {
            'target_name': 'capi-network-connection',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I capi-network-connection | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l capi-network-connection) || true)',
                ],
            },

        },
        {
            'target_name': 'capi-media-player',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I capi-media-player | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l capi-media-player) || true)',
                ],
            },
        },
        {
            'target_name': 'tizen-dlog',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I dlog | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l dlog) || true)',
                ],
            },
        },
        {
            'target_name': 'tizen-bundle',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I bundle | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l bundle) || true)',
                ],
            },
        },
        {
            'target_name': 'vconf',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I vconf vconf-internal-keys-tv vd-win-util | sed s/-I//g) || true)',
                ],

                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l vconf vconf-internal-keys-tv vd-win-util) || true)',
                ],

            },
        },
        {
            'target_name': 'efl_cairo.x64',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas) || true)',
                    '-lturbojpeg -lgif',
                ],
            },
        },
        {
            'target_name': 'efl_cairo.tizen',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf ecore-imf-evas | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf ecore-imf-evas) || true)',
                    '-lturbojpeg -lgif',
                ],
            },
        },
        {
            'target_name': 'efl_headless_cairo.tizen',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I ecore | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l ecore) || true)',
                ],
            },
        },
        {
            'target_name': 'dali.x64',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore | sed s/-I//g) || true)',
                    '/usr/include/dali',
                    'third_party/libtuv/include',
                    'third_party/libtuv/src'
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ) || true)',
                    '-Llib -ltuv',
                    '-ldali-core',
                    '-ldali-adaptor',
                    '-ldali-toolkit',
                ],
            },
        },
        {
            'target_name': 'dali.tizen',
            'type': 'none',
            'all_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore | sed s/-I//g) || true)',
                    '/usr/include/dali',
                    'third_party/libtuv/include',
                    'third_party/libtuv/src'
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ) || true)',
                    '-Llib/tizen -ltuv',
                    '-ldali-core',
                    '-ldali-adaptor',
                    '-ldali-toolkit',
                ],
            },
        },
        {
            'target_name': 'libtuv.x64.debug',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/x86_64-linux/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../debug/lib',
                },
            ],
        },
        {
            'target_name': 'libtuv.x64.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/x86_64-linux/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib',
                },
            ],
        },
        {
            'target_name': 'libtuv.tizen.debug',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/armv7l-linux/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../debug/lib/tizen',
                },
            ],
        },
        {
            'target_name': 'libtuv.tizen.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/armv7l-linux/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/../release/lib/tizen',
                },
            ],
        },
    ],
}
