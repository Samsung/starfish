#!/usr/bin/env python3
"""Instrument native graph/queue code, not the entire browser, with TSan.

Requires a configured Ninja build (default out/release), Clang 18 and xvfb-run.
Uses the build's include/define flags without modifying its objects or binaries.
The harness substitutes a silent device; PulseAudio, DOM and DSP handlers are
outside this test's instrumentation scope.
"""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', default='out/release')
    parser.add_argument('--compiler', default='clang++-18')
    args = parser.parse_args()
    if not os.environ.get('DISPLAY'):
        raise RuntimeError('Run under xvfb-run')
    root = Path(__file__).resolve().parents[2]
    build = (root / args.build_dir).resolve()
    commands = subprocess.check_output(
        ['ninja', '-C', str(build), '-t', 'commands'], text=True)
    command = next(line for line in commands.splitlines()
                   if ' -c ' in line and line.endswith('/render/AudioGraph.cpp'))
    flags = [flag for flag in shlex.split(command)
             if flag.startswith(('-D', '-I'))]
    sources = [root / 'src/core/modules/webaudio/render' / name
               for name in ('AudioGraph.cpp', 'AudioBus.cpp', 'AudioHandler.cpp',
                            'AudioNodeInput.cpp', 'AudioNodeOutput.cpp',
                            'AudioParamTimeline.cpp')]
    sources.append(Path(__file__).with_name('graph_thread_stress.cpp'))
    with tempfile.TemporaryDirectory(prefix='starfish-graph-tsan-') as directory:
        binary = Path(directory) / 'graph-thread'
        subprocess.run([args.compiler, *flags, '-std=c++11', '-g', '-O1',
                        '-fsanitize=thread', '-fno-omit-frame-pointer', '-pthread',
                        *map(str, sources), '-o', str(binary)],
                       cwd=build, check=True, timeout=180)
        subprocess.run([str(binary)], check=True, timeout=60,
                       env=dict(os.environ, TSAN_OPTIONS='halt_on_error=1'))


if __name__ == '__main__':
    main()
