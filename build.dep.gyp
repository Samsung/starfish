{
    'includes': [
        'build.gypi',
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
                   '<@(cflags_extra)',
               ],
               'cflags!' : [
                   '-fvisibility=hidden',
               ],
               'ldflags' : [
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
            'direct_dependent_settings': {
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
                        '.git/modules/binding_generator/HEAD',
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
            'direct_dependent_settings': {
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
            'direct_dependent_settings': {
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
                'third_party/webm/mkvparser.cpp',
                'third_party/webm/webvttparser.cc',
            ],
            'direct_dependent_settings': {
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
                    'destination': '<(PRODUCT_DIR)/lib/release',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/include',
                ],
                'libraries': [
                    'lib/release/libescargot.a',
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
                    'destination': '<(PRODUCT_DIR)/lib/debug',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/include',
                ],
                'libraries': [
                    'lib/debug/libescargot.a',
                ],
            },
        },
        {
            'target_name': 'escargot.tizen.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        '/usr/lib/web-widget-js/release/libescargot.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/tizen/release',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/escargot/include',
                ],
                'libraries': [
                    'lib/tizen/release/libescargot.a',
                ],
            },
        },
        {
            'target_name': 'av.x64.release',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/libav/out/linux/x64/release/libavcodec/libavcodec.so',
                        'third_party/libav/out/linux/x64/release/libavcodec/libavcodec.so.56',
                        'third_party/libav/out/linux/x64/release/libavformat/libavformat.so',
                        'third_party/libav/out/linux/x64/release/libavformat/libavformat.so.56',
                        'third_party/libav/out/linux/x64/release/libavutil/libavutil.so',
                        'third_party/libav/out/linux/x64/release/libavutil/libavutil.so.54',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/release',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/libav/out/linux/x64/release',
                    'third_party/libav',
                ],
                'libraries': [
                    'lib/release/libavcodec.so lib/release/libavformat.so lib/release/libavutil.so',
                ],
            },
        },
        {
            'target_name': 'av.x64.debug',
            'type': 'none',
            'copies': [
                {
                    'files': [
                        'third_party/libav/out/linux/x64/debug/libavcodec/libavcodec.so',
                        'third_party/libav/out/linux/x64/debug/libavcodec/libavcodec.so.56',
                        'third_party/libav/out/linux/x64/debug/libavformat/libavformat.so',
                        'third_party/libav/out/linux/x64/debug/libavformat/libavformat.so.56',
                        'third_party/libav/out/linux/x64/debug/libavutil/libavutil.so',
                        'third_party/libav/out/linux/x64/debug/libavutil/libavutil.so.54',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/debug',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/libav/out/linux/x64/debug',
                    'third_party/libav',
                ],
                'libraries': [
                    'lib/debug/libavcodec.so lib/debug/libavformat.so lib/debug/libavutil.so',
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
                    'destination': '<(PRODUCT_DIR)/lib/debug',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/zeromq/include',
                ],
                'libraries': [
                    'lib/debug/libzmq.so',
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
                    'destination': '<(PRODUCT_DIR)/lib/release',
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/zeromq/include',
                ],
                'libraries': [
                    'lib/release/libzmq.so',
                ],
            },
        },
        {
            'target_name': 'gc.x64.release',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        'third_party/GCutil/bdwgc/out/linux/x64/release.shared/.libs/libgc.so',
                        'third_party/GCutil/bdwgc/out/linux/x64/release.shared/.libs/libgc.so.1',
                        'third_party/GCutil/bdwgc/out/linux/x64/release.shared/.libs/libgc.so.1.0.3',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/release',
                },
            ],
            'include_dirs': [
                'third_party/GCutil/bdwgc/include/',
            ],
            'sources': [
                '<!@(find third_party/GCutil -maxdepth 1 -name *.cpp)',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/GCutil/bdwgc/include',
                    'third_party/GCutil',
                ],
                'libraries': [
                    'lib/release/libgc.so',
                ],
            },
        },
        {
            'target_name': 'gc.x64.debug',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        'third_party/GCutil/bdwgc/out/linux/x64/debug.shared/.libs/libgc.so',
                        'third_party/GCutil/bdwgc/out/linux/x64/debug.shared/.libs/libgc.so.1',
                        'third_party/GCutil/bdwgc/out/linux/x64/debug.shared/.libs/libgc.so.1.0.3',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/debug',
                },
            ],
            'include_dirs': [
                'third_party/GCutil/bdwgc/include/',
            ],
            'sources': [
                '<!@(find third_party/GCutil -maxdepth 1 -name *.cpp)',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/GCutil/bdwgc/include',
                    'third_party/GCutil',
                ],
                'libraries': [
                    'lib/debug/libgc.so',
                ],
            },
        },
        {
            'target_name': 'gc.tizen.release',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        '/usr/lib/web-widget-js/release/libgc.a'
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/tizen/release',
                },
            ],
            'include_dirs': [
                'third_party/GCutil/bdwgc/include/',
            ],
            'sources': [
                '<!@(find third_party/GCutil -maxdepth 1 -name *.cpp)',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'third_party/GCutil/bdwgc/include',
                    'third_party/GCutil',
                ],
                'libraries': [
                    'lib/tizen/release/libgc.a',
                ],
            },
        },
        {
            'target_name': 'efl.x64',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I elementary ecore ecore-x ecore-imf ecore-imf-evas | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l elementary ecore ecore-x ecore-imf ecore-imf-evas) || true)',
                ],
            },
        },
        {
            'target_name': 'efl.tizen',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I dlog elementary ecore ecore-imf-evas efl-extension | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l dlog elementary ecore ecore-imf-evas efl-extension) || true)',
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
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I capi-media-player | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l capi-media-player) || true)',
                ],
            },
        },
        {
            'target_name': 'vconf',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I vconf vconf-internal-keys-tv | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l vconf vconf-internal-keys-tv) || true)',
                ],
            },
        },
        {
            'target_name': 'efl_cairo.x64',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I elementary ecore ecore-x ecore-imf ecore-imf-evas | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l elementary ecore ecore-x ecore-imf ecore-imf-evas) || true)',
                    '-lturbojpeg -lgif',
                ],
            },
        },
        {
            'target_name': 'efl_cairo.tizen',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I elementary ecore ecore-imf ecore-imf-evas | sed s/-I//g) || true)',
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l elementary ecore ecore-imf ecore-imf-evas) || true)',
                    '-lturbojpeg -lgif',
                ],
            },
        },
        {
            'target_name': 'dali.x64',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I elementary ecore | sed s/-I//g) || true)',
                    '/usr/include/dali',
                    'third_party/libtuv/include',
                    'third_party/libtuv/src'
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l elementary ecore ) || true)',
                    'lib/debug/libtuv.a',
                    '-ldali-core',
                    '-ldali-adaptor',
                    '-ldali-toolkit',
                ],
            },
        },
        {
            'target_name': 'dali.tizen',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@((pkg-config --silence-errors --cflags-only-I dlog elementary ecore | sed s/-I//g) || true)',
                    '/usr/include/dali',
                    'third_party/libtuv/include',
                    'third_party/libtuv/src'
                ],
                'libraries': [
                    '<!@((pkg-config --silence-errors --libs-only-l dlog elementary ecore ) || true)',
                    'lib/tizen/release/libtuv.a',
                    '-ldali-core',
                    '-ldali-adaptor',
                    '-ldali-toolkit',
                ],
            },
        },
        {
            'target_name': 'libtuv.x64.debug',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/x86_64-linux/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/debug',
                },
            ],
        },
        {
            'target_name': 'libtuv.x64.release',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/x86_64-linux/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/debug',
                },
            ],
        },
        {
            'target_name': 'libtuv.tizen.debug',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/arm-tizen/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/tizen/debug',
                },
            ],
        },
        {
            'target_name': 'libtuv.tizen.release',
            'type': '<(deplib)',
            'copies': [
                {
                    'files': [
                        'third_party/libtuv/build/arm-tizen/debug/lib/libtuv.a',
                    ],
                    'destination': '<(PRODUCT_DIR)/lib/tizen/release',
                },
            ],
        },
    ],
}
