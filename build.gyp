{
    # build.gyp defines building rules
    # Update build.gypi if build targets needs to be modified

    'includes': [
        'build/build.gypi',
    ],
    #'make_global_settings': [
    #    ['CXX', '/usr/bin/g++'],
    #],
    'target_defaults' : {
        'default_configuration': 'debug',
        'dependencies': [
            #'./build.dep.gyp:js_binding',
            './build.dep.gyp:clipper',
            './build.dep.gyp:earcut.hpp',
            './build.dep.gyp:cppzmq',
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
       'conditions': [
           ['component!="executable"', {
               'sources!' : [
                   '<(main_file)',
                   '<(test_runner_file)',
               ],
           }],
       ],
       'variables' : {
           'sources_extra%': [],
           'conditions' : [
               ['platform=="tizen"', {
                   'sources_extra' : [
                       '<!@(find third_party/deviceapi/src -name *.cpp)',
                   ],
               }],
           ],
       },
       'sources': [
           '<!@(find src -name *.cpp)',
           '<@(sources_extra)',
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
            'target_name': 'lwe.tizen.unified_common.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.common',
            'dependencies': [
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
                '<@(defines_unified_common)',
            ],
        },
        {
            'target_name': 'lwe.tizen.unified_mobile.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.mobile',
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
                '<@(defines_unified_mobile)',
            ],
        },
        {
            'target_name': 'lwe.tizen.unified_tv.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.tv',
            #'soname_version': '0.0.1',
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
                '<@(defines_unified_tv)',
            ],
        },
        {
            # This rule should only be called by lwe_vd
            'target_name': 'lwe.tizen.prod_tv.release',
            'type': 'static_library',
            'product_name': 'lightweight-web-engine.prod_tv',
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
                '<@(defines_prod_tv)',
            ],
            'sources!' : [
                'src/platform/tts/TTSBase.cpp',
                'src/platform/multimedia/MediaPlayerTizenBase.cpp',
            ],
        },
        {
            # This rule should only be called by lwe_vd
            'target_name': 'lwe.tizen.prod_tv_dali.release',
            'type': 'static_library',
            'product_name': 'lightweight-web-engine.prod_tv_dali',
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
                '<@(defines_prod_tv_dali)',
            ],
            'sources!' : [
                'src/platform/tts/TTSBase.cpp',
                'src/platform/multimedia/MediaPlayerTizenBase.cpp',
            ],
        },
        {
            'target_name': 'lwe.tizen.unified_wearable.release',
            'type': '<(component)',
            'product_name': 'lightweight-web-engine.wearable',
            'dependencies': [
                './build.dep.gyp:tizen-dlog',
                './build.dep.gyp:tizen-bundle',
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
                './build.dep.gyp:capi-network-connection',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
                '<@(defines_unified_wearable)',
            ],
        },
        {
            # This rule should only be called by lwe_im
            'target_name': 'lwe.tizen.prod_wearable.release',
            'type': 'static_library',
            'product_name': 'lightweight-web-engine.prod_wearable',
            'dependencies': [
                './build.dep.gyp:tizen-dlog',
                './build.dep.gyp:tizen-bundle',
                './build.dep.gyp:escargot.tizen.release',
                './build.dep.gyp:gc.tizen.release',
                './build.dep.gyp:capi-network-connection',
                '<@(deps_release_extra)',
            ],
            'defines': [
                '<@(defines_tizen)',
                '<@(defines_prod_wearable)',
            ],
        },
    ],
}
