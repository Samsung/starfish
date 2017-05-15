{
    'variables' : {
        'starfish_root': '<!(pwd)',
        'escargot_root': '<(starfish_root)/third_party/escargot',
        'third_party_libs': 'elementary ecore ecore-x libpng cairo freetype2 fontconfig icu-uc icu-i18n',
        'defines_x64': [
            'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_ENABLE_AVPLAY',
            'STARFISH_ENABLE_INSPECTOR',
            'STARFISH_ENABLE_MULTI_PAGE',
            'STARFISH_ENABLE_DOMPARSER',
        ],
        'main_file' : 'src/shell/shell.cpp',
        'variables': {
            'component%': 'static_library',
        },
        'component%':'<(component)',
        'code_gen_results' : ['<!@(python binding_generator/scripts/starfish_code_generator.py src/)',]
    },
    'make_global_settings': [
        ['CXX', '/usr/bin/g++'],
    ],
    'target_defaults' : {
        'dependencies': [
            './build.dep.gyp:clipper.x64',
            './build.dep.gyp:cppzmq.x64',
            './build.dep.gyp:mp4parse.x64',
            './build.dep.gyp:js_binding',
            './build.dep.gyp:skia.x64',
            './build.dep.gyp:webm.x64',
        ],
        'direct_dependent_settings': {
            'include_dirs': [
                '<(starfish_root)/src',
                '<(starfish_root)/inc',
                '<(escargot_root)/src',
                'third_party/clipper/cpp',
                'third_party/MP4Parse/source/include',
                'third_party/skia_matrix',
                'third_party/webm',
            ],
        },
       'include_dirs': [
           '<(starfish_root)/src',
           '<(starfish_root)/inc',
           #'<(starfish_root)/third_party/rapidxml',
           '<!@(pkg-config --cflags-only-I <(third_party_libs) | sed s/-I//g)',
       ],
       'sources': [
           'src/StarFish.cpp',
           '<!@(find src/binding -name *.cpp)',
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
       ],
       'link_settings': {
           'ldflags' : [
               '-L/usr/local/lib',
           ],
           'libraries': [
               '<!@(pkg-config --libs-only-l <(third_party_libs))',
               '-Wl,-rpath=/usr/local/lib',
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
                           '-Werror',
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
            'type': '<(component)',
            'product_name': 'StarFish.x64.debug',
            'dependencies': [
                './build.dep.gyp:escargot.x64.debug',
                './build.dep.gyp:av.x64.debug',
                './build.dep.gyp:gc.x64.debug',
                './build.dep.gyp:zmq.x64.debug',
            ],
            'conditions': [
                ['component=="executable"', {
                    'sources' : [
                        '<(main_file)',
                    ]
                }],
            ],
            'defines': [
                '<@(defines_x64)',
            ],
            'cflags' : [
            ],
            'link_settings': {
                'libraries': [
                    '-Wl,-rpath=\$$ORIGIN/lib/debug',
                    '-Wl,-rpath-link=lib/debug',
                ],
            },
        },
        {
            'target_name': 'starfish.x64.release',
            'type': '<(component)',
            'product_name': 'StarFish.x64.release',
            'dependencies': [
                './build.dep.gyp:escargot.x64.release',
                './build.dep.gyp:av.x64.release',
                './build.dep.gyp:gc.x64.release',
                './build.dep.gyp:zmq.x64.release',
            ],
            'conditions': [
                ['component=="executable"', {
                    'sources' : [
                        '<(main_file)',
                    ]
                }],
            ],
            'defines': [
                '<@(defines_x64)',
            ],
            'cflags' : [
            ],
            'link_settings': {
                'libraries': [
                    '-Wl,-rpath=\$$ORIGIN/lib/release',
                    '-Wl,-rpath-link=lib/release',
                ],
            },
        },
    ],
}
