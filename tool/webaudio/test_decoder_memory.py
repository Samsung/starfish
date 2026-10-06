#!/usr/bin/env python3
"""Check native decoder success/failure cleanup with Valgrind.

Requires an FFmpeg-enabled Ninja build, its development libraries, ffmpeg,
Valgrind and a C++ compiler. Builds only the native decoder in a temp directory.
"""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', default='out/ffmpeg')
    parser.add_argument('--compiler', default='c++')
    args = parser.parse_args()
    if not os.environ.get('DISPLAY'):
        raise RuntimeError('Run under xvfb-run')
    root = Path(__file__).resolve().parents[2]
    build = (root / args.build_dir).resolve()
    commands = subprocess.check_output(
        ['ninja', '-C', str(build), '-t', 'commands'], text=True)
    command = next(line for line in commands.splitlines()
                   if ' -c ' in line and
                   line.endswith('/render/CompressedAudioDecoder.cpp'))
    flags = [flag for flag in shlex.split(command)
             if flag.startswith(('-D', '-I'))]
    if '-DSTARFISH_USE_FFMPEG_MEDIAPLAYER' not in flags:
        raise RuntimeError('An FFmpeg-enabled build is required')
    libraries = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'libavformat', 'libavcodec',
         'libavutil', 'libswresample'], text=True))
    sources = [root / 'src/core/modules/webaudio/render' / name
               for name in ('CompressedAudioDecoder.cpp', 'AudioDecoder.cpp',
                            'AudioBufferData.cpp')]
    sources.append(Path(__file__).with_name('decoder_memory.cpp'))
    with tempfile.TemporaryDirectory(prefix='starfish-decoder-') as directory:
        path = Path(directory)
        binary = path / 'decoder-memory'
        subprocess.run([args.compiler, *flags, '-std=c++11', '-g', '-O1',
                        *map(str, sources), *libraries, '-o', str(binary)],
                       cwd=build, check=True, timeout=180)
        fixture = path / 'tone.wav'
        subprocess.run([
            'ffmpeg', '-nostdin', '-v', 'error', '-f', 'lavfi',
            '-i', 'sine=frequency=440:sample_rate=48000:duration=0.1',
            '-c:a', 'pcm_mulaw', str(fixture),
        ], check=True, timeout=30)
        result = subprocess.run([
            'valgrind', '--leak-check=full', '--show-leak-kinds=definite',
            '--errors-for-leak-kinds=definite', '--error-exitcode=99',
            str(binary), str(fixture),
        ], cwd=path, capture_output=True, text=True, timeout=120)
        print(result.stdout + result.stderr)
        result.check_returncode()
        if 'DECODER_MEMORY_PASS' not in result.stdout:
            raise RuntimeError('Decoder regression did not complete')


if __name__ == '__main__':
    main()
