{
    'variables' : {
        'starfish_root': '<!(pwd)',
        'escargot_root': '<(starfish_root)/third_party/escargot',
        'third_party_libs': 'libpng cairo freetype2 fontconfig icu-uc icu-i18n',
        'defines_x64': [
            'STARFISH_ENABLE_MULTIMEDIA',
            'STARFISH_ENABLE_AVPLAY',
            'STARFISH_ENABLE_INSPECTOR',
            'STARFISH_ENABLE_MULTI_PAGE',
            'STARFISH_ENABLE_DOMPARSER',
        ],
        'defines_extra': [
        ],
        'cflags_extra': [
        ],
        'deps_extra': [
        ],
        'main_file' : 'src/shell/shell.cpp',
        'variables': {
            'component%': 'static_library',
            'backend%': 'efl',
        },
        'component%':'<(component)',
        'code_gen_results' : ['<!@(python binding_generator/scripts/starfish_code_generator.py src/ src/binding/)',],
        'backend%': '<(backend)',
        'conditions': [
            ['backend=="efl"', {
                'defines_extra': [
                    'STARFISH_EFL',
                ],
                'cflags_extra': [
                    '-fno-rtti',
                ],
                'deps_extra': [
                    './build.dep.gyp:efl.x64',
                ],
            }],
            ['backend=="dali"', {
                'defines_extra': [
                    'STARFISH_DALI',
                ],
                'deps_extra': [
                    './build.dep.gyp:dali.x64',
                ],
            }]
        ],
    },
    'make_global_settings': [
        ['CXX', '/usr/bin/g++'],
    ],
    'target_defaults' : {
        'default_configuration': 'debug',
        'dependencies': [
            './build.dep.gyp:js_binding',
            './build.dep.gyp:clipper.x64',
            './build.dep.gyp:cppzmq.x64',
            './build.dep.gyp:mp4parse.x64',
            './build.dep.gyp:js_binding',
            './build.dep.gyp:skia.x64',
            './build.dep.gyp:webm.x64',
            '<@(deps_extra)',
        ],
        'direct_dependent_settings': {
            'include_dirs': [
                '<(starfish_root)/src',
                '<(starfish_root)/inc',
                '<(escargot_root)/include',
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
           '<!@(find src -name *.cpp)',
       ],
       'conditions': [
           ['component!="executable"', {
               'sources!' : [
                   '<(main_file)',
               ],
           }],
       ],
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
                   '<@(cflags_extra)',
               ],
               'ldflags' : [
               ],
               'defines': [
                   'ESCARGOT_64=1',
                   'ESCARGOT',
                   'ESCARGOT_ENABLE_TYPEDARRAY=1',
                   'ESCARGOT_ENABLE_PROMISE=1',
                   '<@(defines_extra)',
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
                           '-g3',
                       ],
                   },
               },
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
                   'configurations': {
                       'debug': {
                           'ldflags': [
                               '-Wl,-rpath=\$$ORIGIN/lib/debug',
                               '-Wl,-rpath-link=lib/debug',
                           ],
                       },
                       'release': {
                           'ldflags': [
                               '-Wl,-rpath=\$$ORIGIN/lib/release',
                               '-Wl,-rpath-link=lib/release',
                           ],
                       }
                   }
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
            'defines': [
                '<@(defines_x64)',
            ],
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
            'defines': [
                '<@(defines_x64)',
            ],
        },
    ],
}
