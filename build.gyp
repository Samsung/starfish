{
    # build.gyp defines building rules
    # Update build.gypi if build targets needs to be modified

    'includes': [
        'build.gypi',
    ],
    #'make_global_settings': [
    #    ['CXX', '/usr/bin/g++'],
    #],
    'target_defaults' : {
        'default_configuration': 'debug',
        'dependencies': [
            './build.dep.gyp:js_binding',
            './build.dep.gyp:clipper',
            './build.dep.gyp:cppzmq',
            './build.dep.gyp:mp4parse',
            './build.dep.gyp:skia',
            './build.dep.gyp:webm',
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
           '<@(include_dirs_default)',
           '<@(include_dirs_extra)',
       ],
       'sources': [
           '<!@(find src -name *.cpp)',
           '<@(sources_extra)',
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
                   '<@(cflags_default)',
                   '<@(cflags_extra)',
               ],
               'ldflags' : [
                   '-Wl,--gc-sections',
                   #'-flto', # when enable lto, we can get slim binary(-200kb). but I can not sure the result is stable & linking takes all day long!
               ],
               'defines': [
                   '<@(defines_default)',
                   '<@(defines_extra)',
               ],
               'include_dirs': [
               ],
               'configurations': {
                   'debug': {
                       'defines': [
                           '<@(defines_debug)',
                       ],
                       'cflags' : [
                           '<@(cflags_debug)',
                       ],
                   },
                   'release': {
                       'defines': [
                           '<@(defines_release)',
                       ],
                       'cflags' : [
	                       '<@(cflags_release)',
                       ],
                   },
               },
               'link_settings': {
                   'ldflags' : [
                       '-L/usr/local/lib',
                   ],
                   'libraries': [
                       '<@(libraries_default)',
                       '<@(libraries_extra)',
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
                './build.dep.gyp:gc.x64.debug',
                './build.dep.gyp:zmq.x64.debug',
                '<@(deps_debug_extra)',
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
                './build.dep.gyp:gc.x64.release',
                './build.dep.gyp:zmq.x64.release',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_x64)',
            ],
        },
        {
            'target_name': 'starfish.tizen.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.tizen',
            'dependencies': [
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
#                './build.dep.gyp:zmq.tizen',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
            ],
        },
        {
            'target_name': 'starfish.tizen_tv.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.tizen_tv',
            'dependencies': [
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
#                './build.dep.gyp:zmq.tizen',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                './build.dep.gyp:vconf',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen_tv)',
            ],
        },
        {
            'target_name': 'starfish.tizen_headless.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.tizen_headless',
            'dependencies': [
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
#                './build.dep.gyp:zmq.tizen',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen_headless)',
            ],
        },

    ],
}
