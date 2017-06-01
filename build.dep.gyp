{
    'variables' : {
        'variables': {
            'dep_lib%': 'shared_library',
            'backend%': 'efl',
        },
        'dep_lib%': '<(dep_lib)',
        'conditions': [
            ['backend=="efl"', {
                'cflags_extra': [
                    '-fno-rtti',
                ],
            }],
            ['backend=="dali"', {
                'cflags_extra': [
                ],
            }]
        ],
    },
    'make_global_settings': [
        ['CXX', '/usr/bin/g++'],
    ],
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
                   '-std=c++11',
                   '-fPIC',
                   '-Wall',
                   '-Wno-unused-but-set-variable',
                   '-Wno-unused-but-set-parameter',
                   '-Wno-unused-parameter',
                   '-Wno-unused-result',
                   '-Wno-unused-variable',
                   '-Wno-unused-function',
                   '-Wno-deprecated-declarations',
                   '-Wno-type-limits',
                   '-Wno-invalid-offsetof',
                   '-fno-math-errno',
                   '-fdata-sections',
                   '-ffunction-sections',
                   '-frounding-math',
                   '-fsignaling-nans',
                   '-fno-omit-frame-pointer',
                   '-fstack-protector',
                   '<@(cflags_extra)',
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
                           '_GLIBCXX_DEBUG',
                       ],
                       'cflags': [
                           '-g3',
                       ],
                   },
                   'release': {
                       'defines': [
                           'NDEBUG',
                       ],
                       'cflags': [
                           '-O2',
                           '-funswitch-loops',
                       ],
                   },
               },
           }],
       ],
    },
    'targets': [
        # Libraries built with our gyp
        {
            'target_name': 'clipper.x64',
            'type': '<(dep_lib)',
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
            'target_name': 'skia.x64',
            'type': '<(dep_lib)',
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
            'target_name': 'mp4parse.x64',
            'type': '<(dep_lib)',
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
            'target_name': 'webm.x64',
            'type': '<(dep_lib)',
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
        # Libraries that have only header files
        {
            'target_name': 'cppzmq.x64',
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
            'type': 'none',
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
            'type': 'none',
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
            'target_name': 'efl.x64',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                    '<!@(pkg-config --cflags-only-I elementary ecore ecore-x | sed s/-I//g)',
                ],
                'libraries': [
                    '<!@(pkg-config --libs-only-l elementary ecore ecore-x)',
                ],
            },
        },
        {
            'target_name': 'dali.x64',
            'type': 'none',
            'direct_dependent_settings': {
                'include_dirs': [
                ],
                'libraries': [
                    '-ldali-core -ldali-adaptor -ldali-toolkit -luv',
                ],
            },
        },
    ],
}
