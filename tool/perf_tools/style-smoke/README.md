# Style / layout invalidation smoke test

A deterministic regression oracle for style-recalc and layout changes. It
drives the kinds of DOM mutations a TV SPA (YouTube) hits constantly -
remote-focus class toggles, attribute changes, ancestor-class flips, and
structural add/remove - then folds every element's resolved style and box
geometry into a single rolling hash. The hash is stable across runs of the
same build, so a code change that alters computed styles or layout is caught
as a signature change.

`page.html` deliberately includes the selector shapes that style
invalidation must get right:

- `.item.active` - class change on the element itself
- `.item.active + .item` - adjacent sibling combinator
- `.item.active ~ .item` - general sibling combinator (every following item)
- `.sel .label` - descendant via ancestor class
- `.item[data-mark="1"] .label` - descendant via attribute

The script walks focus down the list (toggling `.active` on each row),
marks attributes, flips the ancestor class, and removes/re-adds rows,
snapshotting `getComputedStyle` (color / font-weight / background) of every
item and its label plus `getBoundingClientRect` after each step.

## Run

```sh
python3 tools/style-smoke/server.py &        # serves on 127.0.0.1:8801
./out/x11/release/bin/Starfish http://127.0.0.1:8801/page.html
grep STYLE-DONE /tmp/sf-style/results.txt    # sig / steps / items
```

`server.py` reads `page.html` from `/tmp/sf-style/` if present, else from its
own directory; results append to `/tmp/sf-style/results.txt`.

## Use as an oracle

Run before a style/layout change to record the baseline `sig`, then run
after - the `sig`, `steps` and `items` values must be identical. Because the
stylesheet contains sibling combinators, any change that breaks
sibling-combinator invalidation (e.g. wrongly skipping the sibling-recalc
walk) changes the signature.

Baseline on the perf branch (release, 2026-06-13): `sig=2756853606 steps=34
items=18`, stable across runs. The absolute value is build-specific - it is
only meaningful as a before/after comparison on the same build.
