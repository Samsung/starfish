{
    'target_defaults' : {
       'conditions': [
           ['custom=="vd"', {
               'sources!' : [
                   'src/platform/tts/TTSBase.cpp',
                   'src/platform/multimedia/MediaPlayerTizenBase.cpp',
               ],
           }],
       ],
    },
}
