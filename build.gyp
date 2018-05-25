{
    # build.gyp defines building rules
    # Update build.gypi if build targets needs to be modified

    'includes': [
        'build/build.gypi',
        'build/build_vd.gypi'
    ],
    #'make_global_settings': [
    #    ['CXX', '/usr/bin/g++'],
    #],
    'target_defaults' : {
        'default_configuration': 'debug',
        'dependencies': [
            #'./build.dep.gyp:js_binding',
            './build.dep.gyp:clipper',
            './build.dep.gyp:cppzmq',
            './build.dep.gyp:skia',
            '<@(deps_extra)',
        ],
        'direct_dependent_settings': {
            'include_dirs': [
                '<(starfish_root)/src',
                '<(starfish_root)/inc',
                '<(starfish_root)/third_party/escargot/third_party/rapidjson/include',
            ],
            'libraries': [
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
                   '<(test_runner_file)',
               ],
           }],
       ],
       'conditions': [
           ['OS=="linux"', {
               'cflags' : [
                   '<@(cflags_default)',
                   '<@(cflags_compiler)',
                   '<@(cflags_extra)',
               ],
               'ldflags' : [
                   '-Wl,--gc-sections',
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
               'code_gen_results' : ['<!@(python binding_generator/scripts/starfish_code_generator.py src/ src/binding/)',],
               'link_settings': {
                   'ldflags' : [
                       '-L/usr/local/lib',
                   ],
                   'libraries': [
                       '<@(libraries_default)',
                       '<@(libraries_compiler)',
                       '<@(libraries_extra)',
                   ],
                   'configurations': {
                       'debug': {
                           'ldflags': [
                               '-Wl,-rpath=\$$ORIGIN/lib',
                               '-Wl,-rpath-link=lib',
                           ],
                       },
                       'release': {
                           'ldflags': [
                               '-Wl,-rpath=\$$ORIGIN/lib',
                               '-Wl,-rpath-link=lib',
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
                './build.dep.gyp:mp4parse',
                './build.dep.gyp:webm',
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
                './build.dep.gyp:mp4parse',
                './build.dep.gyp:webm',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_x64)',
            ],

        },
        {
            'target_name': 'starfish.tizen.unified.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.unified',
            'dependencies': [
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
                # './build.dep.gyp:zmq.tizen',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                './build.dep.gyp:mp4parse',
                './build.dep.gyp:webm',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
                '<@(defines_custom_unified)',
            ],
        },
        {
            'target_name': 'starfish.tizen.tv.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.tv',
            'dependencies': [
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
                # './build.dep.gyp:zmq.tizen',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                './build.dep.gyp:vconf',
                './build.dep.gyp:mp4parse',
                './build.dep.gyp:webm',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
                '<@(defines_custom_vd)',
            ],
        },
        {
            'target_name': 'starfish.tizen.gear.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.gear',
            'dependencies': [
                './build.dep.gyp:tizen-dlog',
                './build.dep.gyp:tizen-bundle',
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                './build.dep.gyp:mp4parse',
                './build.dep.gyp:webm',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
                '<@(defines_custom_im)',
            ],
        },
        {
            'target_name': 'starfish.tizen.speaker.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.speaker',
            'dependencies': [
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
                # './build.dep.gyp:zmq.tizen',
                './build.dep.gyp:capi-network-connection',
                './build.dep.gyp:capi-media-player',
                './build.dep.gyp:mp4parse',
                './build.dep.gyp:webm',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
                #'<@(defines_custom_im)',
                '<@(defines_tizen_headless)',
            ],
        },

    ],
}
