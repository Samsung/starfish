# MSE / XHR ingress smoke test

Quick runtime smoke test for the media-source and XHR arraybuffer paths.
No real codec media required; it exercises the plumbing end to end:

- XHR `responseType=arraybuffer`: byte-exact 1MB delivery (validates the
  ingress copy-reduction path: Content-Length reserve, flush move, vector
  adoption into the ArrayBuffer backing store).
- `xhr.response === null` during Loading (XHR spec; regressed to a partial
  snapshot before the Done-gating fix).
- MediaSource open / addSourceBuffer / appendBuffer event flow and
  `timeupdate` listener registration.

## Run

```sh
python3 tools/mse-smoke/server.py &           # serves on 127.0.0.1:8799
./out/x11/debug/bin/Starfish http://127.0.0.1:8799/test.html
cat /tmp/sf-test/results.txt                  # PASS/FAIL per check
```

If the binary fails to start with an OpenSSL symbol error (bundled
libssl vs system libcurl), prefix with:

```sh
LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:out/x11/debug/lib
```

`server.py` reads `test.html` from its own directory if
`/tmp/sf-test/test.html` is absent; results are appended to
`/tmp/sf-test/results.txt`.

## Run the append/remove micro-benchmark (bench.html)

`bench.html` is preloaded-bytes only (no network in the timed loop), so
it measures the MSE append/remove/buffered path, not fetch. It needs the
generated WebM assets and the bench routes that `server.py` now serves
(`/bench.html`, `/init.webm`, `/seg*.webm`):

```sh
python3 tools/mse-smoke/gen_webm.py /tmp/sf-test   # init.webm + seg0-4.webm
python3 tools/mse-smoke/server.py &
./out/x11/release/bin/Starfish http://127.0.0.1:8799/bench.html
grep BENCH-DONE /tmp/sf-test/results.txt           # ms / perAppend / buffered
```

Each run prints one `BENCH-DONE rounds=120 ms=<wall> perAppend=<ms>
removed=11 buffered=[..] n=3` line. The `buffered` range must be
identical across runs (regression guard); `ms` carries event-loop
scheduling noise (see below), so take a 5-run mean.

## Known baseline quirk

`mse_append_events` reports FAIL on garbage (non-parsable) appendBuffer
input: only `updatestart` is observed, `updating` returns to false, but
`update`/`updateend` never fire. This reproduces identically on
`master_build` - it is a pre-existing engine behavior for failed init
segments, not a regression of the perf branch.

## Measurement notes (2026-06-13, Linux x11 debug, WSL2)

- `bench.html` (120 appends of ~60KB clusters + 11 removes + per-round
  `buffered` polls): perf branch ~239ms mean vs master_build ~284ms mean
  (~16% less wall time), identical final buffered ranges.
- gprofng profile of the bench run: engine code (`libStarfish-impl.so`)
  does not register above noise even in an 8s window - the remaining
  per-append latency (~2ms) is event-loop scheduling granularity, not
  CPU in the MSE path. The profile is dominated by the test shell's
  console reader thread and software-GL compositing, both harness
  artifacts absent on a TV target.
- Scaling frame payloads to 100KB (1080p-bitrate-class clusters) hits
  the SourceBuffer quota with playback parked at t=0 (nothing behind
  the position to evict), so a no-playback append loop cannot exercise
  high-bitrate volume; a decode-capable media asset would be needed.

## Final branch state (2026-06-13, after cycles 27-29)

Late-branch changes verified with this harness:

- codedFrameEviction quota memoization (resolution-tier loop now runs
  once per init segment instead of per append).
- m_bufferUnprocessed switched to atomic (unscanned) GC allocation -
  MB-scale leftover append bytes no longer scanned by the collector.
- Combined MediaPacket allocation: struct + payload in one block,
  ownership-transfer onDetectPacket; halves per-frame allocator
  traffic.

Results: mse2.html and test.html outputs unchanged vs the cycle-24/25
baseline on both debug and release builds (only the documented
pre-existing garbage-append quirk fails). bench.html on the release
build: 127 ms mean +- 15 over 5 runs (~1.06 ms per append), identical
buffered/removed output every run; debug build ~250-265 ms mean,
within noise of the pre-cycle-27 ~239 ms (the cycle 27-29 wins are
allocator/GC-side and below this harness's noise floor on WSL2).

## Reproducibility note (2026-06-13, cycle 30)

`server.py` now serves the bench routes (`/bench.html`, `/init.webm`,
`/seg*.webm`) so the bench.html numbers above are reproducible straight
from the committed repo - previously only `test.html`/`blob.bin` were
routed and the bench had to be run against an ad-hoc server. Fresh
release re-measure on the branch HEAD after the branch switch + clean
relink: 115 ms mean over 5 runs (76/82/116/141/160; ~0.96 ms per
append), buffered `[0.1-53.3] n=3` and removed=11 identical every run.
The wide spread (76-160 ms) is event-loop scheduling granularity, not
MSE-path CPU - consistent with the cycle-27-29 finding that the engine
no longer registers above this harness's noise floor on WSL2 x11.
