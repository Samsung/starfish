#!/usr/bin/env python3
"""Compare native audio demuxing with independently decoded reference PCM."""

import argparse
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile


def run(command, **kwargs):
    return subprocess.run(command, check=True, timeout=180, **kwargs)


def replace_box(data, names, transform):
    """Rewrite a generated fixture's moov-at-end metadata without moving mdat."""
    cursor = 0
    output = bytearray()
    while cursor < len(data):
        size, kind = struct.unpack_from('>I4s', data, cursor)
        if size < 8 or cursor + size > len(data):
            raise ValueError('invalid generated fixture box')
        payload = data[cursor + 8:cursor + size]
        if kind == names[0]:
            payload = replace_box(payload, names[1:], transform) if len(names) > 1 else transform(payload)
            output.extend(struct.pack('>I4s', 8 + len(payload), kind) + payload)
        else:
            output.extend(data[cursor:cursor + size])
        cursor += size
    return bytes(output)


def mp4_variant(data, variant):
    stbl = [b'moov', b'trak', b'mdia', b'minf', b'stbl']
    if variant in {'delay', 'cuts'}:
        def edit(payload):
            original = payload[8:20]
            start = struct.unpack_from('>I', original, 4)[0]
            if variant == 'delay':
                entries = struct.pack('>IiI', 250, -1, 0x10000) + original
            else:
                entries = struct.pack('>III', 40, start, 0x10000) + struct.pack('>III', 40, start + 3360, 0x10000)
            return payload[:4] + struct.pack('>I', 2) + entries
        data = replace_box(data, [b'moov', b'trak', b'edts', b'elst'], edit)
        for names, offset in [([b'moov', b'mvhd'], 16), ([b'moov', b'trak', b'tkhd'], 20)]:
            def duration(payload, offset=offset):
                old = struct.unpack_from('>I', payload, offset)[0]
                return payload[:offset] + struct.pack('>I', old + 250 if variant == 'delay' else 80) + payload[offset + 4:]
            data = replace_box(data, names, duration)
    if variant == 'descriptions':
        data = replace_box(data, stbl + [b'stsd'], lambda p: p[:4] + struct.pack('>I', 2) + p[8:] * 2)
        def descriptions(payload):
            p = bytearray(payload)
            for offset in range(8, len(p), 12):
                struct.pack_into('>I', p, offset + 8, 2)
            return bytes(p)
        data = replace_box(data, stbl + [b'stsc'], descriptions)
    if variant.startswith('packed'):
        bits = int(variant[6:])
        def compact(payload):
            size, count = struct.unpack_from('>II', payload, 4)
            sizes = [size] * count if size else list(struct.unpack_from('>' + 'I' * count, payload, 12))
            if any(s >= 1 << bits for s in sizes):
                raise ValueError('generated sample does not fit stz2')
            if bits == 4:
                table = bytes((sizes[i] << 4) | (sizes[i + 1] if i + 1 < count else 0) for i in range(0, count, 2))
            else:
                table = struct.pack('>' + ('B' if bits == 8 else 'H') * count, *sizes)
            return payload[:4] + bytes(3) + bytes([bits]) + struct.pack('>I', count) + table
        data = replace_box(data, stbl, lambda p: replace_box(p, [b'stsz'], compact).replace(b'stsz', b'stz2', 1))
    if variant == 'co64':
        def wide(payload):
            count = struct.unpack_from('>I', payload, 4)[0]
            offsets = struct.unpack_from('>' + 'I' * count, payload, 8)
            return payload[:8] + struct.pack('>' + 'Q' * count, *offsets)
        data = replace_box(data, stbl, lambda p: replace_box(p, [b'stco'], wide).replace(b'stco', b'co64', 1))
    return data


def wave_variant(data, variant):
    output = bytearray(b'RIFX' if variant == 'rifx' else b'RF64')
    output.extend(bytes(4) + b'WAVE')
    if variant == 'rf64':
        output.extend(b'ds64' + struct.pack('<IQQQI', 28, len(data) + 28, 0, 0, 0))
    cursor = 12
    align = 2
    while cursor < len(data):
        kind = data[cursor:cursor + 4]
        size = struct.unpack_from('<I', data, cursor + 4)[0]
        payload = bytearray(data[cursor + 8:cursor + 8 + size])
        if kind == b'fmt ': align = struct.unpack_from('<H', payload, 12)[0]
        if variant == 'rifx':
            if kind == b'fmt ':
                fields = struct.unpack_from('<HHIIHH', payload)
                payload[:16] = struct.pack('>HHIIHH', *fields)
            elif kind == b'data':
                payload[::2], payload[1::2] = payload[1::2], payload[::2]
            elif kind == b'LIST':
                offset = 4
                while offset + 8 <= len(payload):
                    length = struct.unpack_from('<I', payload, offset + 4)[0]
                    struct.pack_into('>I', payload, offset + 4, length)
                    offset += 8 + length + (length & 1)
        output.extend(kind)
        output.extend(struct.pack('>I' if variant == 'rifx' else '<I', 0xffffffff if variant == 'rf64' and kind == b'data' else size))
        output.extend(payload)
        if size & 1: output.append(0)
        if variant == 'rf64' and kind == b'data':
            struct.pack_into('<QQ', output, 28, size, size // align)
        cursor += 8 + size + (size & 1)
    struct.pack_into('>I' if variant == 'rifx' else '<I', output, 4, len(output) - 8 if variant == 'rifx' else 0xffffffff)
    return bytes(output)


def ogg_audio_stream(data):
    """Isolate the generated Vorbis stream for Xiph's single-stream API."""
    serial = None
    output = bytearray()
    cursor = 0
    while cursor < len(data):
        segments = data[cursor + 26]
        header = 27 + segments
        size = header + sum(data[cursor + 27:cursor + header])
        payload = data[cursor + header:cursor + size]
        current = data[cursor + 14:cursor + 18]
        if serial is None and payload.startswith(b'\x01vorbis'):
            serial = current
        if current == serial:
            output.extend(data[cursor:cursor + size])
        cursor += size
    return bytes(output)


def offset_ogg(data, offset):
    result = bytearray(data)
    cursor = 0
    while cursor < len(result):
        header = 27 + result[cursor + 26]
        size = header + sum(result[cursor + 27:cursor + header])
        granule = struct.unpack_from('<Q', result, cursor + 6)[0]
        if granule not in (0, 0xffffffffffffffff):
            struct.pack_into('<Q', result, cursor + 6, granule + offset)
            result[cursor + 22:cursor + 26] = bytes(4)
            crc = 0
            for value in result[cursor:cursor + size]:
                crc ^= value << 24
                for _ in range(8):
                    crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
            struct.pack_into('<I', result, cursor + 22, crc)
        cursor += size
    return bytes(result)


def _matroska_delay(data):
    """Read the first audio track's CodecDelay for reference normalization."""
    if not data.startswith(b'\x1aE\xdf\xa3'):
        return 0
    def elements(start, end):
        def vint(cursor, identifier=False):
            first = data[cursor]
            width = next(w for w in range(1, 9) if first & (128 >> (w - 1)))
            value = first if identifier else first & ((128 >> (width - 1)) - 1)
            for byte in data[cursor + 1:cursor + width]: value = (value << 8) | byte
            return value, cursor + width, width
        while start < end:
            tag, cursor, _ = vint(start, True)
            size, cursor, width = vint(cursor)
            if size == (1 << (width * 7)) - 1: size = end - cursor
            yield tag, cursor, cursor + size
            start = cursor + size
    for tag, start, end in elements(0, len(data)):
        if tag != 0x18538067: continue
        for tag, start, end in elements(start, end):
            if tag != 0x1654ae6b: continue
            for tag, start, end in elements(start, end):
                if tag != 0xae: continue
                fields = {tag: data[start:end] for tag, start, end in elements(start, end)}
                if int.from_bytes(fields.get(0x83, b''), 'big') == 2:
                    return int.from_bytes(fields.get(0x56aa, b''), 'big')
    return 0


def matroska_delay(data):
    try:
        return _matroska_delay(data)
    except (IndexError, StopIteration):
        return 0


def corpus_test(binary, vorbis, directories, path, report):
    extensions = {'.mp3', '.ogg', '.opus', '.oga', '.ogv', '.flac', '.m4a', '.mp4',
                  '.mov', '.webm', '.weba', '.mkv', '.mka', '.wav', '.aac', '.3gp'}
    codecs = {'aac', 'alac', 'mp3', 'flac', 'vorbis', 'opus', 'adpcm_ms', 'adpcm_ima_wav'}
    results = []
    files = sorted({p.resolve() for directory in directories
                    for p in Path(directory).rglob('*')
                    if p.suffix.lower() in extensions and p.is_file()})
    for track in files:
        if track.stat().st_size > 16 * 1024 * 1024:
            continue
        try:
            probe = subprocess.run(['ffprobe', '-v', 'error', '-show_streams',
                                    '-show_format', '-of', 'json', str(track)],
                                   capture_output=True, text=True, timeout=20)
            info = json.loads(probe.stdout)
            audio = next((s for s in info.get('streams', [])
                          if s.get('codec_type') == 'audio'), None)
            if not audio:
                continue
            codec = audio.get('codec_name', '')
            if codec not in codecs and not codec.startswith('pcm_'):
                continue
            duration = float(audio.get('duration', info.get('format', {}).get('duration', '0')))
            channels = int(audio.get('channels', 0))
            if not 0 < duration <= 30 or not 0 < channels <= 32:
                continue
            if duration * 48000 * channels * 4 > 128 * 1024 * 1024:
                continue
            reference = path / 'corpus-reference.f32'
            decoded = subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y',
                                      '-i', str(track), '-map', '0:a:0', '-ar', '48000',
                                      '-f', 'f32le', str(reference)], capture_output=True,
                                     text=True, timeout=30)
            if decoded.returncode or not reference.stat().st_size:
                continue
            if codec == 'vorbis' and track.suffix in {'.ogg', '.oga', '.ogv'}:
                raw = path / 'vorbis-reference.f32'
                vorbis_track = path / 'corpus-vorbis.ogg'
                vorbis_track.write_bytes(ogg_audio_stream(track.read_bytes()))
                with raw.open('wb') as output:
                    run([str(vorbis), str(vorbis_track)], stdout=output)
                run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-f', 'f32le',
                     '-ar', audio['sample_rate'], '-ac', str(channels), '-i', str(raw),
                     '-ar', '48000', '-f', 'f32le', str(reference)])
            data = track.read_bytes()
            delay = matroska_delay(data)
            skip = (delay * int(audio['sample_rate']) + 500000000) // 1000000000
            matroska_opus = codec == 'opus' and data.startswith(b'\x1aE\xdf\xa3')
            if skip or matroska_opus:
                raw = path / 'matroska-reference.f32'
                options = ['-flags2', '+skip_manual'] if matroska_opus else []
                run(['ffmpeg', '-nostdin', '-v', 'error', '-y', *options, '-i', str(track),
                     '-map', '0:a:0', '-f', 'f32le', str(raw)])
                pcm = raw.read_bytes()
                if matroska_opus:
                    # Compare container delay against independently demuxed
                    # packets. FFmpeg's automatic trim uses OpusHead even
                    # when the Matroska CodecDelay overrides that value.
                    packets = json.loads(subprocess.check_output([
                        'ffprobe', '-v', 'error', '-select_streams', 'a:0',
                        '-show_packets', '-of', 'json', str(track)]))
                    discard = sum(side.get('discard_padding', 0)
                                  for packet in packets['packets']
                                  for side in packet.get('side_data_list', []))
                    if discard:
                        pcm = pcm[:-discard * channels * 4]
                raw.write_bytes(pcm[skip * channels * 4:])
                run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-f', 'f32le',
                     '-ar', audio['sample_rate'], '-ac', str(channels), '-i', str(raw),
                     '-ar', '48000', '-f', 'f32le', str(reference)])
            checked = subprocess.run([str(binary), str(track), str(reference),
                                      '48000', str(channels), 'corpus'],
                                     capture_output=True, text=True, timeout=30)
            result = {'file': str(track), 'codec': codec,
                      'ok': checked.returncode == 0, 'exit': checked.returncode,
                      'detail': checked.stderr[-3000:]}
            results.append(result)
            print(('CORPUS_PASS ' if result['ok'] else 'CORPUS_FAIL ') +
                  str(track) + (' ' + result['detail'] if not result['ok'] else ''), flush=True)
        except (ValueError, subprocess.TimeoutExpired) as error:
            print('CORPUS_SKIP', track, error, flush=True)
    Path(report).write_text(json.dumps(results, indent=2))
    passed = sum(r['ok'] for r in results)
    print(f'Corpus: {passed}/{len(results)} PCM comparisons passed; report: {report}', flush=True)
    return passed == len(results) and bool(results)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', default='build/out_linux')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--container-filter', help='Only matching codec/container pairs')
    parser.add_argument('--corpus', action='append', default=[], help='Scan a media directory')
    parser.add_argument('--corpus-only', action='store_true')
    parser.add_argument('--report', default='/tmp/starfish-audio-corpus.json')
    parser.add_argument('--channels', type=int, nargs='+', default=[1, 2])
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
        ['pkg-config', '--cflags', '--libs', 'libavcodec',
         'libavutil', 'libswresample'], text=True))
    sources = [root / 'src/core/modules/webaudio/render' / name
               for name in ('CompressedAudioDecoder.cpp', 'AudioDecoder.cpp',
                            'AudioBufferData.cpp')]
    sources.append(Path(__file__).with_name('native_audio.cpp'))
    with tempfile.TemporaryDirectory(prefix='starfish-native-audio-') as directory:
        path = Path(directory)
        binary = path / 'native-audio'
        vorbis = path / 'vorbis-reference'
        vorbis_libs = shlex.split(subprocess.check_output(
            ['pkg-config', '--cflags', '--libs', 'vorbisfile'], text=True))
        run(['c++', '-O2', str(Path(__file__).with_name('vorbis_reference.cpp')),
             *vorbis_libs, '-o', str(vorbis)])
        sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] \
            if args.sanitize else []
        run(['c++', *flags, '-std=c++11', '-g', '-O1', *sanitize,
             *map(str, sources), *libraries, '-o', str(binary)], cwd=build)
        imports = subprocess.check_output(['nm', '-u', str(binary)], text=True)
        if 'avformat_' in imports or 'avio_' in imports:
            raise RuntimeError('Web Audio decoder still imports FFmpeg demuxing')
        corpus_ok = True
        if args.corpus:
            corpus_ok = corpus_test(binary, vorbis, args.corpus, path, args.report)
        if args.corpus_only:
            if not corpus_ok:
                raise SystemExit(1)
            return
        cases = 0
        for rate in (() if args.container_filter and args.container_filter != 'mp3' else (44100, 22050, 11025)):
            for channels in (c for c in args.channels if c <= 2):
                for mode in ('cbr', 'vbr', 'untagged', 'tags', 'ape', 'preamble', 'stale'):
                    track = path / 'tone.mp3'
                    options = ['-q:a', '4'] if mode == 'vbr' else ['-b:a', '32k']
                    run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-f', 'lavfi',
                         '-i', f'sine=frequency=440:sample_rate={rate}:duration=0.13',
                         '-ac', str(channels), '-c:a', 'libmp3lame', *options,
                         '-write_xing', '0' if mode == 'untagged' else '1',
                         str(track)])
                    reference_track = path / 'reference.mp3'
                    reference_track.write_bytes(track.read_bytes())
                    if mode == 'tags':
                        # Replace metadata without changing audio packets.
                        # A footer tag contains frames, never padding (ID3v2.4).
                        original = track.read_bytes()
                        tag_size = 0
                        for value in original[6:10]:
                            tag_size = (tag_size << 7) | value
                        tag = b'ID3\x04\x00\x10\x00\x00\x08\x00'
                        title = b'TIT2\x00\x00\x07\x76\x00\x00' + b'\x03' + b'x' * 1013
                        footer = b'3DI' + tag[3:]
                        track.write_bytes(tag + title + footer +
                                          original[10 + tag_size:] +
                                          b'TAG' + bytes(125))
                    if mode == 'ape':
                        value = b'1.23 dB'
                        item = struct.pack('<II', len(value), 0) + b'REPLAYGAIN_TRACK_GAIN\0' + value
                        def ape(flags):
                            return b'APETAGEX' + struct.pack('<IIII', 2000, len(item) + 32, 1, flags) + bytes(8)
                        track.write_bytes(track.read_bytes() + ape(0xa0000000) + item + ape(0x80000000) + b'TAG' + bytes(125))
                    if mode == 'preamble':
                        track.write_bytes(b'legacy encoder preamble\0' + track.read_bytes())
                    if mode == 'stale':
                        data = bytearray(track.read_bytes())
                        position = data.find(b'Xing')
                        if position < 0: position = data.find(b'Info')
                        count = struct.unpack_from('>I', data, position + 8)[0]
                        struct.pack_into('>I', data, position + 8, count + 1)
                        track.write_bytes(data)
                        reference_track.write_bytes(data)
                    for target in (rate, 48000):
                        reference = path / 'reference.f32'
                        run(['ffmpeg', '-nostdin', '-v', 'error', '-y',
                             '-i', str(reference_track), '-ar', str(target), '-f', 'f32le',
                             str(reference)])
                        run([str(binary), str(track), str(reference),
                             str(target), str(channels)])
                        cases += 1
        print(f'{cases} native MP3 comparisons passed')
        for codec, suffix in [('flac', 'flac'), ('flac', 'ogg'), ('aac', 'aac'), ('aac', 'tag.aac'), ('aac', 'm4a'),
                              ('aac', 'frag.mp4'), ('aac', 'mka'), ('flac', 'mka'),
                              ('libmp3lame', 'mka'), ('libopus', 'mp4'),
                              ('libmp3lame', 'mp4'), ('libmp3lame', 'fast.mp4'), ('libmp3lame', 'wav'), ('pcm_s16le', 'mov'),
                              ('pcm_s16be', 'mov'),
                              ('alac', 'm4a'), ('alac', 'mov'),
                              ('alac', 'highres.m4a'), ('pcm_f64le', 'highres.mov'),
                              ('flac', 'highres.flac'),
                              ('aac', 'delay.m4a'), ('aac', 'cuts.m4a'),
                              ('aac', 'descriptions.m4a'), ('aac', 'packed16.m4a'),
                              ('aac', 'co64.m4a'), ('aac', 'fast.m4a'),
                              ('pcm_s16le', 'packed4.mov'), ('pcm_s16le', 'packed8.mov'),
                              ('pcm_s16le', 'rf64.wav'), ('pcm_s16le', 'rifx.wav'),
                              ('libvorbis', 'ogg'), ('libopus', 'opus'),
                              ('libvorbis', 'mux.ogg'), ('libopus', 'mux.ogg'),
                              ('libvorbis', 'video.ogg'),
                              ('libvorbis', 'offset.ogg'), ('libopus', 'offset.ogg'),
                              ('libvorbis', 'tags.ogg'), ('libopus', 'tags.ogg'), ('flac', 'tags.ogg'),
                              ('libvorbis', 'webm'), ('libopus', 'webm'),
                              ('libopus', 'codec-delay.webm'),
                              ('libvorbis', 'chain.ogg'), ('libopus', 'chain.ogg'),
                              ('pcm_mulaw', 'wav'), ('pcm_alaw', 'wav'),
                              ('adpcm_ima_wav', 'wav'), ('adpcm_ms', 'wav')]:
            if args.container_filter and args.container_filter not in f'{codec}:{suffix}':
                continue
            for channels in args.channels:
                track = path / f'tone.{suffix}'
                reference_track = track
                source_rate = 96000 if suffix.startswith('highres.') else 48000
                options = ['-movflags', 'empty_moov+frag_keyframe+default_base_moof'] if suffix == 'frag.mp4' else []
                if suffix.startswith('fast.'): options = ['-movflags', 'faststart']
                if suffix == 'codec-delay.webm': options = ['-write_crc32', '0']
                inputs = []
                if suffix == 'mux.ogg':
                    options += ['-map', '0:a:0', '-map', '0:a:0']
                if suffix == 'video.ogg':
                    inputs = ['-f', 'lavfi', '-i', 'color=c=black:size=32x32:rate=10:duration=0.13']
                    options += ['-map', '1:v:0', '-map', '0:a:0', '-c:v', 'libtheora', '-shortest']
                if suffix == 'tags.ogg':
                    tags = path / 'tags.txt'
                    tags.write_text(';FFMETADATA1\ncomment=' + 'x' * (1024 * 1024 + 512) + '\n')
                    inputs = ['-f', 'ffmetadata', '-i', str(tags)]
                    options += ['-map_metadata', '1']
                signal = f'sine=frequency=440:sample_rate={source_rate}:duration=0.13'
                if suffix == 'offset.ogg':
                    signal = signal.replace('duration=0.13', 'duration=1.13')
                if channels > 2:
                    expressions = '|'.join(f'0.05*sin(2*PI*{220 + c * 110}*t)' for c in range(channels))
                    duration = 1.13 if suffix == 'offset.ogg' else 0.13
                    signal = f'aevalsrc={expressions}:s={source_rate}:d={duration}'
                run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-f', 'lavfi',
                     '-i', signal,
                     *inputs, '-ac', str(channels), '-c:a', codec, *options, str(track)])
                variant = suffix.split('.')[0]
                delay = matroska_delay(track.read_bytes())
                if delay and codec != 'libopus':
                    # Matroska requires preroll to be discarded even where
                    # FFmpeg CLI preserves it in a raw PCM output.
                    raw = path / 'matroska-reference.f32'
                    run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-i', str(track), '-f', 'f32le', str(raw)])
                    skip = (delay * source_rate + 500000000) // 1000000000
                    raw.write_bytes(raw.read_bytes()[skip * channels * 4:])
                if variant == 'codec-delay':
                    # CodecDelay remains authoritative when OpusHead has zero
                    # pre-skip. Compare with the original equivalent file;
                    # FFmpeg 6's native Opus decoder otherwise keeps preroll.
                    reference_track = path / 'codec-delay-reference.webm'
                    reference_track.write_bytes(track.read_bytes())
                    data = bytearray(track.read_bytes())
                    head = data.index(b'OpusHead')
                    data[head + 10:head + 12] = bytes(2)
                    track.write_bytes(data)
                if variant == 'offset':
                    reference_track = path / 'offset-original.ogg'
                    reference_track.write_bytes(track.read_bytes())
                    track.write_bytes(offset_ogg(track.read_bytes(), source_rate))
                if variant in {'rf64', 'rifx'}:
                    if variant == 'rifx':
                        # FFmpeg 6's WAV demuxer interprets RIFX PCM as LE;
                        # the original LE fixture is the endian-independent oracle.
                        reference_track = path / 'rifx-original.wav'
                        reference_track.write_bytes(track.read_bytes())
                    track.write_bytes(wave_variant(track.read_bytes(), variant))
                if variant in {'delay', 'cuts', 'descriptions', 'packed4', 'packed8', 'packed16', 'co64'}:
                    if variant == 'cuts':
                        raw = path / 'cuts.f32'
                        run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-i', str(track), '-f', 'f32le', str(raw)])
                        pcm = raw.read_bytes()
                        stride = channels * 4
                        raw.write_bytes(pcm[:1920 * stride] + pcm[3360 * stride:5280 * stride])
                    track.write_bytes(mp4_variant(track.read_bytes(), variant))
                if suffix == 'tag.aac':
                    tag = b'ID3\x04\x00\x00\x00\x00\x00\x0cTIT2\x00\x00\x00\x02\x00\x00\x03x'
                    track.write_bytes(tag + track.read_bytes())
                if suffix == 'chain.ogg':
                    # Each logical stream has its own decoder overlap/skip.
                    # Concatenate independently decoded PCM before resampling.
                    raw = path / 'chain.f32'
                    if codec == 'libvorbis':
                        with raw.open('wb') as output:
                            run([str(vorbis), str(track)], stdout=output)
                    else:
                        run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-i', str(track),
                             '-f', 'f32le', str(raw)])
                    raw.write_bytes(raw.read_bytes() * 2)
                    track.write_bytes(track.read_bytes() * 2)
                for target in (source_rate, 44100):
                    reference = path / 'reference.f32'
                    if codec == 'libvorbis' and suffix in {'ogg', 'mux.ogg', 'video.ogg', 'tags.ogg', 'offset.ogg'}:
                        raw = path / 'vorbis-reference.f32'
                        vorbis_track = reference_track
                        if suffix in {'mux.ogg', 'video.ogg'}:
                            vorbis_track = path / 'vorbis-isolated.ogg'
                            vorbis_track.write_bytes(ogg_audio_stream(track.read_bytes()))
                        with raw.open('wb') as output:
                            run([str(vorbis), str(vorbis_track)], stdout=output)
                    source = ['-f', 'f32le', '-ar', str(source_rate), '-ac', str(channels),
                              '-i', str(raw)] if (delay and codec != 'libopus') or variant == 'cuts' or suffix == 'chain.ogg' or (codec == 'libvorbis' and suffix in {'ogg', 'mux.ogg', 'video.ogg', 'tags.ogg', 'offset.ogg'}) else ['-i', str(reference_track)]
                    run(['ffmpeg', '-nostdin', '-v', 'error', '-y', *source,
                         '-ar', str(target), '-f', 'f32le', str(reference)])
                    print(f'Comparing {codec} channels={channels} rate={target}', flush=True)
                    run([str(binary), str(track), str(reference),
                         str(target), str(channels), 'container'])
                    cases += 1
        print(f'{cases} native audio comparisons passed (without libavformat)')
        if not corpus_ok:
            raise SystemExit(1)


if __name__ == '__main__':
    main()
