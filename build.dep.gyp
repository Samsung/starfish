{
    'variables' : {
        'variables': {
            'component%': 'static_library',
        },
        'component%':'<(component)',
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
                   '-O2',
                   '-Wall',
                   '-Wextra',
                   '-Werror',
                   '-Wno-unused-but-set-variable',
                   '-Wno-unused-but-set-parameter',
                   '-Wno-unused-parameter',
                   '-Wno-unused-result',
                   '-Wno-unused-variable',
                   '-Wno-unused-function',
                   '-Wno-deprecated-declarations',
                   '-Wno-type-limits',
                   '-fno-rtti',
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
                   '-funswitch-loops',
               ],
               'ldflags' : [
               ],
               'defines': [
               ],
               'include_dirs': [
               ],
               'configurations': {
                   'debug' : {
                   },
                   'release': {
                       'defines': [
                           'NDEBUG',
                       ],
                   },
               },
           }],
       ],
    },
    'targets': [
        {
            'target_name': 'skia.x64.release',
            'type': 'static_library',
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
            'target_name': 'mp4parse.x64.release',
            'type': 'static_library',
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
            'target_name': 'webm.x64.release',
            'type': 'static_library',
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
            'target_name': 'clipper.x64.release',
            'type': 'static_library',
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
    ],
}
