# Agent headless memory bench

Compares engine memory, and optionally layout geometry, across builds by
driving the same pages through the LWE CLI (`-DCLI=1`).

```sh
./gen_pages.py /tmp/agent-pages          # text, images, gifs, effects pages (needs Pillow)
./bench.py --cli out/headless/bin/lightweight-web-engine-cli \
    --geom 200 --out headless.json \
    about:blank file:///tmp/agent-pages/text.html \
    file:///tmp/agent-pages/images.html file:///tmp/agent-pages/effects.html
```

Serve the pages over HTTP (for example `python3 -m http.server` in the
page directory) for numbers that match the web: `file:` loads are
synchronous and skip the background image decoding path.

For each URL the bench opens a fresh session, waits for
`document.readyState == "complete"` plus `--settle` seconds, and reads the
engine process's `/proc/<pid>/smaps_rollup` (Rss, Pss, Pss_Anon) and
`VmHWM` (peak RSS), all in kB. Pss_Anon is the engine's own heap; the rest of Pss is
mostly shared library text. `--geom N` also stores the rounded bounding
boxes of the first N body elements, so two builds' JSON outputs can be
diffed to confirm identical layout.

A build with an x11 shell needs a display; run it under
`xvfb-run -s '-screen 0 1920x1080x24' -a`.
