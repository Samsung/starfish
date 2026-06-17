# Canvas measureText micro-benchmark

A CPU-bound benchmark + correctness oracle for the text-measurement path
(font fallback resolution, harfbuzz shaping, freetype glyph advances). Unlike
the media/style harnesses, `measureText` in a tight JS loop is dominated by
real font/glyph CPU work, not event-loop scheduling, so wall time here is a
usable perf signal on WSL2 x11 - not just noise.

`page.html` measures a corpus of YouTube-on-TV-style strings (titles, channel
names, view/time metadata, menu labels), including repeated strings and many
strings sharing glyphs - the pattern a glyph/shaping cache helps. It records:

- `ms` - wall time for the timed loop (the perf metric)
- `widthsig` / `wsum` - signature of all measured widths (the correctness
  oracle; must not change across a perf optimization)
- `calls`, `perCall`

## Run

```sh
python3 tools/measure-bench/server.py &        # serves on 127.0.0.1:8802
./out/x11/release/bin/Starfish http://127.0.0.1:8802/page.html
grep MEASURE-DONE /tmp/sf-measure/results.txt
```

`server.py` reads `page.html` from `/tmp/sf-measure/` if present, else from
its own directory; results append to `/tmp/sf-measure/results.txt`.

## Use

Run before/after a text-measurement change. `widthsig` and `wsum` must be
identical (correctness); `ms` is the perf comparison (take a few runs).

Baseline (perf branch, release, 2026-06-13): `ms=39 calls=11700
perCall=0.0033 widthsig=828485824 wsum=4275.00`, stable across runs.
