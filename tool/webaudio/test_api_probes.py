#!/usr/bin/env python3
"""Run the Web Audio API probes in api_probes.html against a Starfish build.

Covers behaviour no runnable WPT gates on the CI build. Run under xvfb-run.
STARFISH_BIN selects the browser executable (default ./Starfish). Only a
loopback HTTP server is used; no audio device or external network.
"""

import ctypes
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import resource
import shutil
import socket
import struct
import subprocess
import tempfile
import threading
import unittest


def wave(seconds=4, rate=48000, value=8192):
    """Mono 16-bit PCM WAVE holding one constant sample value."""
    data = struct.pack('<h', value) * (rate * seconds)
    return (b'RIFF' + struct.pack('<I', 36 + len(data)) + b'WAVE' + b'fmt ' +
            struct.pack('<IHHIIHH', 16, 1, 1, rate, rate * 2, 2, 16) +
            b'data' + struct.pack('<I', len(data)) + data)


# Media of the page's origin reads 0.25 in the graph; the other origin's
# reads 0.5, so a probe can tell whose samples it hears.
TONE = wave()
OTHER = wave(value=16384)
SHORT = wave(seconds=1, value=16384)

# Served at the other origin (localhost): makes a Blob URL of its own media
# and hands it to the page that framed it.
BLOB_MAKER = b"""<!doctype html><script>
fetch('/media/tone.wav').then((r) => r.blob()).then(
    (blob) => parent.postMessage(URL.createObjectURL(blob), '*'));
</script>"""


class ProbeServer(ThreadingHTTPServer):
    """Serves the probe page and limits the browser's address space on
    request.

    A limit measured on another run of the browser does not leave the same
    room in this one: its threads' malloc arenas and other mappings differ
    by hundreds of MB from run to run. The page therefore asks once its
    setup is done, and the limit is set relative to what this browser
    process uses at that moment.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.margin = None
        self.pid = None
        self.started = threading.Event()
        self.limited = None
        self.error = None
        self.redirect_seeks = False
        self.stalled = 0
        self.release_stalled = threading.Event()
        self.blocked_targets = None
        self.blocked_responses = {}
        self.split_methods = set()
        self.unanswered = []
        self.unanswered_at_fetch = None

    def expect(self, margin):
        self.margin = margin
        self.pid = None
        self.started.clear()
        self.limited = None
        self.error = None
        self.redirect_seeks = False

    def attach(self, pid):
        self.pid = pid
        self.started.set()

    def detach(self):
        self.margin = None
        self.pid = None
        self.started.clear()

    def limit_address_space(self):
        if not self.margin:
            return
        if not self.started.wait(10):
            self.error = 'the browser was not started'
            return
        try:
            with open(f'/proc/{self.pid}/status') as status:
                size = next(int(line.split()[1]) * 1024 for line in status
                            if line.startswith('VmSize:'))
            limit = size + self.margin
            resource.prlimit(self.pid, resource.RLIMIT_AS, (limit, limit))
            self.limited = limit
        except (OSError, StopIteration) as error:
            self.error = repr(error)

    def dead_thread_stacks(self):
        """Thread stacks the browser maps beyond its live threads: exited
        threads nobody joined, and the few freed stacks glibc caches."""
        if not self.started.wait(10):
            return None
        size = resource.getrlimit(resource.RLIMIT_STACK)[0]
        page = resource.getpagesize()
        stacks = 0
        with open(f'/proc/{self.pid}/maps') as maps:
            for line in maps:
                fields = line.split()
                # glibc maps RLIMIT_STACK plus a guard page per thread; the
                # guard is a separate PROT_NONE mapping or, with
                # MADV_GUARD_INSTALL, part of the rw-p one, and adjacent
                # stacks then merge.
                if len(fields) == 5 and fields[1] == 'rw-p':
                    start, end = (int(x, 16) for x in fields[0].split('-'))
                    stacks += (end - start + page) // size
        with open(f'/proc/{self.pid}/status') as status:
            threads = next(int(line.split()[1]) for line in status
                           if line.startswith('Threads:'))
        return stacks - threads


class ProbeRequestHandler(SimpleHTTPRequestHandler):
    def do_HEAD(self):
        if self.path.startswith('/media/'):
            self.media()
        else:
            super().do_HEAD()

    def do_GET(self):
        if self.path.startswith('/media/'):
            self.media()
            return
        if self.path == '/thread-stacks':
            stacks = self.server.dead_thread_stacks()
            if stacks is None:
                self.reply(503, b'')
            else:
                self.reply(200, str(stacks).encode())
            return
        if self.path != '/address-space-limit':
            super().do_GET()
            return
        self.server.limit_address_space()
        self.reply(200, b'')

    def reply(self, code, body, headers=()):
        self.send_response(code)
        for header in headers:
            self.send_header(*header)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        if self.command != 'HEAD':
            self.wfile.write(body)

    def media(self):
        """Media for the MediaElementAudioSourceNode CORS probes. The same
        server reached as localhost is another origin."""
        port = self.server.server_port
        elsewhere = f'http://localhost:{port}/media/other.wav'
        ranges = self.headers.get('Range', 'bytes=0-')
        start = int(ranges.split('=')[1].split('-')[0])
        if self.path == '/media/redirect-seeks':
            self.server.redirect_seeks = True
            self.reply(200, b'')
        elif self.path == '/media/list.ffconcat':
            self.reply(200, b"ffconcat version 1.0\nfile 'redirect.wav'\n")
        elif self.path == '/media/redirect.wav' or (
                self.path in ('/media/seek-redirect.wav',
                              '/media/cors-seek-redirect.wav') and
                self.server.redirect_seeks and start):
            self.reply(302, b'', [('Location', elsewhere)])
        elif self.path == '/media/cors-redirect.wav':
            self.reply(302, b'', [
                ('Location', f'http://localhost:{port}/media/cors-other.wav'),
                ('Access-Control-Allow-Origin', '*')])
        elif self.path == '/media/cors-split.wav':
            # The engine's CORS check (HEAD) and the player (GET) end up apart.
            self.server.split_methods.add(self.command)
            target = 'cors-other' if self.command == 'HEAD' else 'cors-elsewhere'
            self.reply(302, b'', [
                ('Location', f'http://localhost:{port}/media/{target}.wav'),
                ('Access-Control-Allow-Origin', '*')])
        elif self.path == '/media/cors-nohead.wav' and self.command == 'HEAD':
            self.reply(405, b'')
        elif (self.path == '/media/cors-unanswered.wav' and
              self.command == 'HEAD'):
            # Takes the CORS check and never answers it.
            self.server.unanswered.append(self.path)
            self.server.release_stalled.wait(30)
        elif self.path == '/media/fetched-while-unanswered':
            self.server.unanswered_at_fetch = len(self.server.unanswered)
            self.reply(200, b'')
        elif self.path == '/media/cors-hls':
            # A playlist that its CORS check (HEAD) takes for plain media.
            self.reply(200, b'#EXTM3U\n#EXT-X-TARGETDURATION:2\n'
                       b'#EXTINF:2.0,\n/hls/tone.mp4\n#EXT-X-ENDLIST\n', [
                ('Content-Type', 'audio/wav' if self.command == 'HEAD'
                 else 'application/vnd.apple.mpegurl'),
                ('Access-Control-Allow-Origin', '*')])
        elif self.path in ('/media/tone.wav', '/media/seek-redirect.wav',
                           '/media/other.wav', '/media/short.wav',
                           '/media/cors-other.wav', '/media/cors-elsewhere.wav',
                           '/media/cors-nohead.wav',
                           '/media/cors-seek-redirect.wav',
                           '/media/cors-unanswered.wav'):
            body = {'/media/tone.wav': TONE, '/media/seek-redirect.wav': TONE,
                    '/media/cors-seek-redirect.wav': TONE,
                    '/media/short.wav': SHORT}.get(self.path, OTHER)
            if start >= len(body):
                self.reply(416, b'', [('Content-Range', f'bytes */{len(body)}')])
                return
            cors = ([('Access-Control-Allow-Origin', '*')]
                    if self.path.startswith('/media/cors-') else [])
            self.reply(206, body[start:], [
                ('Content-Type', 'audio/wav'), ('Accept-Ranges', 'bytes'),
                ('Content-Range',
                 f'bytes {start}-{len(body) - 1}/{len(body)}')] + cors)
        elif self.path == '/media/blocked-targets':
            self.reply(200, json.dumps(self.server.blocked_targets).encode(),
                       [('Content-Type', 'application/json')])
        elif self.path in self.server.blocked_responses:
            self.reply(*self.server.blocked_responses[self.path])
        elif self.path == '/media/stall':
            # Takes the request and never answers.
            self.server.stalled += 1
            self.server.release_stalled.wait(30)
        elif self.path == '/media/stall-seek.wav':
            # 10 s of 8 kHz media of which 6 s are sent; a request for a
            # later part (a seek) is never answered.
            body = wave(seconds=10, rate=8000)
            if start > 1000:
                self.server.release_stalled.wait(30)
                return
            self.send_response(206 if start else 200)
            self.send_header('Accept-Ranges', 'bytes')
            self.send_header('Content-Range',
                             f'bytes {start}-{len(body) - 1}/{len(body)}')
            self.send_header('Content-Length', str(len(body) - start))
            self.end_headers()
            self.wfile.write(body[start:44 + 8000 * 2 * 6])
            self.wfile.flush()
            self.server.release_stalled.wait(30)
        elif self.path == '/media/stall-midway.wav':
            # Sends 5.5 s of 10 s of media, then nothing: more than the
            # player reads before metadata, so it stalls during playback.
            body = wave(seconds=10)
            self.send_response(200)
            self.send_header('Content-Type', 'audio/wav')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body[:44 + 48000 * 11])
            self.wfile.flush()
            self.server.release_stalled.wait(30)
        elif self.path == '/media/blob-maker.html':
            self.reply(200, BLOB_MAKER, [('Content-Type', 'text/html')])
        else:
            self.reply(404, b'')


class APIProbesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.environ.get('DISPLAY'):
            raise RuntimeError('Run this test under xvfb-run')
        root = Path(__file__).resolve().parents[2]
        cls.browser = Path(os.environ.get('STARFISH_BIN',
                                          root / 'Starfish')).resolve()
        if not cls.browser.is_file():
            raise RuntimeError(f'Missing browser: {cls.browser}')
        cls.directory = tempfile.TemporaryDirectory(prefix='starfish-api-probes-')
        cls.addClassCleanup(cls.directory.cleanup)
        cls.path = Path(cls.directory.name)
        shutil.copyfile(Path(__file__).with_name('api_probes.html'),
                        cls.path / 'index.html')
        # A file a page must not hear: the browser runs in this directory.
        (cls.path / 'local.wav').write_bytes(TONE)
        # HLS of one MP3 segment (1 s of silent 44.1 kHz mono frames; HLS
        # takes no WAV), alone and with a WebVTT subtitles rendition.
        hls = cls.path / 'hls'
        hls.mkdir()
        playlist = ('#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXTINF:1.0,\n{}\n'
                    '#EXT-X-ENDLIST\n')
        (hls / 'audio.m3u8').write_text(playlist.format('segment.mp3'))
        (hls / 'segment.mp3').write_bytes(
            (bytes([0xFF, 0xFB, 0x90, 0xC4]) + bytes(413)) * 39)
        (hls / 'subtitles.m3u8').write_text(playlist.format('subtitles.vtt'))
        (hls / 'subtitles.vtt').write_text(
            'WEBVTT\n\n00:00.000 --> 00:01.000\nTone\n')
        (hls / 'master.m3u8').write_text(
            '#EXTM3U\n#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="subs",'
            'NAME="English",LANGUAGE="en",AUTOSELECT=YES,'
            'URI="subtitles.m3u8"\n'
            '#EXT-X-STREAM-INF:BANDWIDTH=800000,SUBTITLES="subs"\n'
            'audio.m3u8\n')
        # An audible segment (AAC in fragmented MP4).
        shutil.copyfile(
            root / 'tests/assertion/webaudio/resources/tone-steps.mp4',
            hls / 'tone.mp4')
        cls.server = ProbeServer(('127.0.0.1', 0),
            partial(ProbeRequestHandler, directory=str(cls.path)))
        thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        thread.start()
        cls.addClassCleanup(cls.server.server_close)
        cls.addClassCleanup(thread.join)
        cls.addClassCleanup(cls.server.shutdown)
        # Media of other schemes, and what containers web media doesn't
        # play name, must never reach these listeners (tcp:, gopher:, unix:,
        # a concat entry, an image) or open this FIFO (a DASH segment; an
        # open would block). Nothing accepts, so a connection stays queued.
        cls.blocked = [socket.create_server(('127.0.0.1', 0)),
                       socket.socket(socket.AF_UNIX)]
        cls.blocked[1].bind(str(cls.path / 'blocked.sock'))
        cls.blocked[1].listen()
        for listener in cls.blocked:
            listener.setblocking(False)
            cls.addClassCleanup(listener.close)
        os.mkfifo(cls.path / 'blocked.mp4')
        port = cls.blocked[0].getsockname()[1]
        segment = (cls.path / 'blocked.mp4').as_uri()
        cls.server.blocked_targets = {
            'file': (cls.path / 'local.wav').as_uri(),
            'tcp': f'tcp://127.0.0.1:{port}',
            'gopher': f'gopher://127.0.0.1:{port}/0x',
            'unix': f'unix://{cls.path / "blocked.sock"}',
            'redirect to tcp': '/media/redirect-to-tcp.wav',
            'concat list': '/media/blocked.ffconcat',
            'DASH': '/media/blocked.mpd',
            'image sequence': f'http://127.0.0.1:{port}/?x.jpg',
            'HLS variant': '/media/blocked-variant.m3u8',
        }
        cls.server.blocked_responses = {
            # Not to a listener: libavformat requests a redirect's host over
            # http whatever its scheme, so the redirect connects.
            '/media/redirect-to-tcp.wav': (302, b'', [(
                'Location', f'tcp://127.0.0.1:{cls.server.server_port}'
                            '/media/tone.wav')]),
            '/media/blocked.ffconcat':
                (200, b"ffconcat version 1.0\nfile 'blocked-entry.wav'\n"),
            '/media/blocked-entry.wav': (302, b'', [(
                'Location', f'http://127.0.0.1:{port}/x.wav')]),
            # HLS opens playlists (variants, renditions) without the scheme
            # check it gives segments.
            '/media/blocked-variant.m3u8': (200, (
                '#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=800000\n'
                f'tcp://127.0.0.1:{port}/v.m3u8\n').encode()),
            '/media/blocked-subtitles.m3u8': (200, (
                '#EXTM3U\n#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="subs",'
                'NAME="English",LANGUAGE="en",AUTOSELECT=YES,'
                f'URI="tcp://127.0.0.1:{port}/s.m3u8"\n'
                '#EXT-X-STREAM-INF:BANDWIDTH=800000,SUBTITLES="subs"\n'
                '/hls/audio.m3u8\n').encode()),
            '/media/blocked.mpd': (200, f'''<?xml version="1.0"?>
<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" type="static"
     mediaPresentationDuration="PT1S" minBufferTime="PT1S"
     profiles="urn:mpeg:dash:profile:isoff-on-demand:2011"><Period>
 <AdaptationSet mimeType="audio/mp4"><Representation id="a"
   bandwidth="64000" codecs="mp4a.40.2"><SegmentList duration="1">
  <Initialization sourceURL="{segment}"/><SegmentURL media="{segment}"/>
 </SegmentList></Representation></AdaptationSet>
</Period></MPD>'''.encode(), [('Content-Type', 'application/dash+xml')]),
        }
        cls.addClassCleanup(cls.server.release_stalled.set)

    def command(self, group, seconds, options):
        return [
            str(self.browser),
            f'http://127.0.0.1:{self.server.server_port}/?{group}',
            '--hide-window', f'--timeout={seconds}',
            f'--storage-dir={self.path / "storage"}', *options,
        ]

    def environment(self):
        return dict(os.environ,
                    PULSE_SERVER=f'unix:{self.path}/no-audio-device')

    def run_group(self, group, address_space_margin=None, seconds=20,
                  options=()):
        """Runs one group for at most |seconds|, with the browser's command
        line |options|. With |address_space_margin| (bytes), the page's
        limitAddressSpace() leaves the browser that much more address space
        than it uses at that point."""
        self.server.expect(address_space_margin)
        process = subprocess.Popen(
            self.command(group, seconds, options), env=self.environment(),
            cwd=self.path, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True)
        self.server.attach(process.pid)
        try:
            stdout, stderr = process.communicate(timeout=seconds + 20)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise
        finally:
            self.server.detach()
        output = stdout + stderr
        if address_space_margin:
            self.assertIsNone(self.server.error, self.server.error)
            self.assertIsNotNone(self.server.limited,
                                 f'{group} never asked for the limit')
        self.assertEqual(process.returncode, 0, output)
        self.assertNotIn('API_PROBE_FAIL', output, output)
        self.assertNotIn('API_PROBE_SKIP', output, output)
        self.assertIn('API_PROBE_PASS', output, output)

    def test_shared_array_buffer_views_are_rejected(self):
        self.run_group('shared-views')

    def test_media_element_source_constructor(self):
        self.run_group('media-element-source')

    def test_media_blob_duration(self):
        self.run_group('media-blob-duration')

    def test_media_blob_invalid(self):
        self.run_group('media-blob-invalid')

    def test_media_blob_revoked(self):
        self.run_group('media-blob-revoked')

    def test_media_source_cors(self):
        self.run_group('media-source-same-origin')
        self.run_group('media-source-other-origin')
        self.run_group('media-source-other-origin-ended')
        self.run_group('media-source-nested-redirect')
        self.run_group('media-source-blob-nested-file')
        self.run_group('media-source-seek-redirect')

    def test_media_source_crossorigin(self):
        self.run_group('media-source-crossorigin')
        # The split case tested what it says only if both requests came.
        self.assertEqual(self.server.split_methods, {'HEAD', 'GET'})
        self.run_group('media-source-crossorigin-webgl')

    def test_media_crossorigin_unanswered(self):
        self.run_group('media-source-crossorigin-unanswered')
        # The fetch was made while all six checks were held.
        self.assertEqual(self.server.unanswered_at_fetch, 6)

    def test_failed_output_writers_are_joined(self):
        # Writer threads exist only when libpulse-simple loads; each then
        # fails to connect to the missing socket.
        try:
            ctypes.CDLL('libpulse-simple.so.0')
        except OSError:
            print('::warning::libpulse-simple.so.0 is unavailable; failed '
                  'audio output writers were not covered')
            self.skipTest('libpulse-simple.so.0 is unavailable')
        if (resource.getrlimit(resource.RLIMIT_STACK)[0] ==
                resource.RLIM_INFINITY):
            print('::warning::the thread stack size is unlimited; failed '
                  'audio output writers were not covered')
            self.skipTest('thread stack size is unknown')
        self.run_group('failed-output-writers')

    def test_media_fetch_schemes(self):
        self.run_group('media-source-blocked-schemes')
        for listener in self.blocked:
            with self.assertRaises(BlockingIOError):
                listener.accept()

    def test_media_other_origin_blob(self):
        self.run_group('media-source-other-origin-blob')
        self.run_group('media-source-other-origin-blob-insecure',
                       options=['--disable-web-security'])

    def test_media_stalled_server(self):
        self.run_group('media-source-stalled-server')
        self.assertEqual(self.server.stalled, 1)

    def test_media_stalled_playback(self):
        # 5.5 s of playback, then the 10 s read timeout.
        self.run_group('media-source-stalled-playback', seconds=30)

    def test_media_replaced_during_stalled_seek(self):
        # The dropped player gives up after two 10 s read timeouts.
        self.run_group('media-source-replaced-stall', seconds=40)

    def test_media_hls_subtitles(self):
        self.run_group('media-source-hls-subtitles')
        for listener in self.blocked:
            with self.assertRaises(BlockingIOError):
                listener.accept()

    def test_reverse_loop_with_sub_sample_start(self):
        self.run_group('reverse-loop')

    def test_scheduled_sources_use_exact_times(self):
        self.run_group('exact-times')

    def test_script_processor(self):
        self.run_group('script-processor')

    def test_render_quantum_size(self):
        self.run_group('render-quantum')

    def test_detached_document(self):
        self.run_group('detached-document')

    def test_source_node_options(self):
        self.run_group('source-options')

    def test_detached_channel(self):
        self.run_group('detached-channel')

    def test_attribute_accessor_names(self):
        self.run_group('accessor-names')

    def test_structured_clone_transfer(self):
        self.run_group('structured-clone-transfer')

    def test_out_of_memory(self):
        # Each group asks for the limit once its graph is built, so the
        # margin is what is left for the allocations under test, regardless
        # of host RAM: less than the failing ones need, more than the rest.
        self.run_group('oom-channel-count', 4 << 30)
        self.run_group('oom-node-creation', 512 << 20)

    def test_audio_worklet(self):
        # Needs a build with AUDIO_WORKLET=1, as CI builds.
        self.run_group('audio-worklet')
        self.run_group('audio-worklet-node')
        self.run_group('audio-worklet-slow-render')
        self.run_group('audio-worklet-failed-node')


if __name__ == '__main__':
    unittest.main()
