{
    'target_defaults' : {
       'conditions': [
           ['custom=="vd"', {
               'sources!' : [
                   'src/platform/tts/TTSMock.cpp',
               ],
           }],
       ],
    },
}
