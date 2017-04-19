# To generate ninja config file, type
# GYP_GENERATORS=ninja gyp build.gyp --toplevel-dir=`pwd` --depth=0
#
# To compile, type
# ninja -C out/[debug|release] starfish.x64.[debug|release]
#
# e.g.,
# ninja -C out/debug starfish.x64.debug
# ninja -C out/release starfish.x64.release

{
    'variables' : {
        'starfish_root': '<!(pwd)',
        'escargot_root': '<(starfish_root)/third_party/escargot',
        'libav_root': '<(starfish_root)/third_party/libav/out/linux/x64/release',
        'third_party_libs': 'elementary ecore ecore-x libpng cairo freetype2 fontconfig icu-uc icu-i18n',
        'defines_x64': [
            'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_ENABLE_AVPLAY',
            'STARFISH_ENABLE_INSPECTOR',
            'STARFISH_ENABLE_MULTI_PAGE',
            'STARFISH_ENABLE_DOMPARSER',
        ],
        'libraries_x64_debug': [
            '<(escargot_root)/out/linux/x64/interpreter/debug/libescargot.a',
            '<(escargot_root)/third_party/bdwgc/out/linux/x64/debug.shared/.libs/libgc.a',
            '<(starfish_root)/third_party/zeromq/out/linux/x64/debug.shared/.libs/libzmq.a',
        ],
        'libraries_x64_release': [
            '<(escargot_root)/out/linux/x64/interpreter/release/libescargot.a',
            '<(escargot_root)/third_party/bdwgc/out/linux/x64/release.shared/.libs/libgc.a',
            '<(starfish_root)/third_party/zeromq/out/linux/x64/release.shared/.libs/libzmq.a',
        ],
    },
    'make_global_settings': [
        ['CXX', '/usr/bin/g++'],
    ],
    'target_defaults' : {
       'include_dirs': [
           '<(starfish_root)/src',
           '<(starfish_root)/inc',
           '<(starfish_root)/third_party/skia_matrix',
           '<(starfish_root)/third_party/clipper/cpp',
           '<(starfish_root)/third_party/rapidxml',
           '<(starfish_root)/third_party/cppzmq',
           '<(starfish_root)/third_party/zeromq/include',
           '<(starfish_root)/third_party/webm',
           '<(starfish_root)/third_party/MP4Parse/source/include',
           '<(escargot_root)/src',
           '<(escargot_root)/third_party/bdwgc/include',
           '<(escargot_root)/third_party/checked_arithmetic',
           '<(escargot_root)/third_party/double_conversion',
           '<(escargot_root)/third_party/rapidjson/include',
           '<!@(pkg-config --cflags-only-I <(third_party_libs) | sed s/-I//g)',
       ],
       'sources': [
           'src/StarFish.cpp',
           '<!@(find src/animation -name *.cpp)',
           '<!@(find src/dom -name *.cpp)',
           '<!@(find src/extra -name *.cpp)',
           '<!@(find src/inspector -name *.cpp)',
           '<!@(find src/layout -name *.cpp)',
           '<!@(find src/loader -name *.cpp)',
           '<!@(find src/platform -name *.cpp)',
           '<!@(find src/public -name *.cpp)',
           '<!@(find src/style -name *.cpp)',
           '<!@(find src/util -name *.cpp)',
           'third_party/clipper/cpp/clipper.cpp',
           '<!@(find third_party/MP4Parse/source -name MP4*.cpp)',
           'third_party/skia_matrix/SkMath.cpp',
           'third_party/skia_matrix/SkPoint.cpp',
           'third_party/skia_matrix/SkRect.cpp',
           'third_party/skia_matrix/SkMatrix.cpp',
           'third_party/skia_matrix/SkDebug.cpp',
           'third_party/webm/mkvparser.cpp',
           'third_party/webm/webvttparser.cc',
       ],
       'link_settings': {
           'ldflags' : [
               '-L/usr/local/lib',
               '-L<(libav_root)/libavformat',
               '-L<(libav_root)/libavcodec',
               '-L<(libav_root)/libavutil',
               '-L<(starfish_root)',
           ],
           'libraries': [
               '<!@(pkg-config --libs-only-l <(third_party_libs))',
               '-Wl,-rpath <(libav_root)/libavformat -lavformat',
               '-Wl,-rpath <(libav_root)/libavcodec -lavcodec',
               '-Wl,-rpath <(libav_root)/libavutil -lavutil',
               '-lpthread',
               '-lcurl',
           ],
       },
       'conditions': [
           ['OS=="linux"', {
               'cflags' : [
                   '-std=c++11',
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
               ],
               'ldflags' : [
               ],
               'defines': [
                   'STARFISH_EFL',
                   'ESCARGOT_64=1',
                   'ESCARGOT',
                   'USE_ES6_FEATURE',
               ],
               'include_dirs': [
               ],
               'configurations': {
                   'debug': {
                       'defines': [
                           'GC_DEBUG', # bdwgc
                           '_GLIBCXX_DEBUG',
                           'STARFISH_ENABLE_TEST',
                       ],
                       'cflags' : [
                           '-O0',
                           '-g3',
                       ],
                   },
                   'release': {
                       'defines': [
                           'NDEBUG',
                           'STARFISH_ENABLE_TEST',
                       ],
                       'cflags' : [
                           '-O2',
                           '-funswitch-loops',
                       ],
                   },
               },
           }],
       ],
    },
    'targets': [
        {
            'target_name': 'starfish.x64.debug',
            'type': 'executable',
            'dependencies': [
            ],
            'sources' : [
                'src/shell/shell.cpp',
            ],
            'defines': [
                '<@(defines_x64)',
            ],
            'cflags' : [
            ],
            'libraries': [
                '<@(libraries_x64_debug)',
            ],
        },
        {
            'target_name': 'starfish.x64.lib.debug',
            'type': 'static_library',
            'dependencies': [
            ],
            'defines': [
                '<@(defines_x64)',
            ],
            'cflags' : [
            ],
            'libraries': [
                '<@(libraries_x64_debug)',
            ],
        },
        {
            'target_name': 'starfish.x64.release',
            'type': 'executable',
            'dependencies': [
            ],
            'sources' : [
                'src/shell/shell.cpp',
            ],
            'defines': [
                '<@(defines_x64)',
            ],
            'cflags' : [
            ],
            'libraries': [
                '<@(libraries_x64_release)',
            ],
        },
        {
            'target_name': 'starfish.x64.lib.release',
            'type': 'static_library',
            'dependencies': [
            ],
            'defines': [
                '<@(defines_x64)',
            ],
            'cflags' : [
            ],
            'libraries': [
                '<@(libraries_x64_release)',
            ],
        }
    ],
}
