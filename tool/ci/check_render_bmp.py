#!/usr/bin/env python3
"""Judge a StarfishShell --screenshot capture of tool/ci/windows_render_check.html.

The point of this script is that an artifact nobody opens is not a test. The
cairo float-word-order bug compiled, linked, started, loaded the page, swapped
frames and reported no error anywhere -- it just painted nothing, and only a
human looking at the window caught it. These assertions are what would have
caught it automatically:

  paint  a flat #1040c0 fill exists            -> the paint path draws at all
  image  #ffd000 exists, and it is a colour     -> the PNG decoder ran and its
         used nowhere in the page's CSS            output reached the screen
  font   dark ink pixels exist on the light     -> glyphs actually rasterised
         text band                                 rather than leaving a gap

Reads the BMP that RendererWGL writes: 32bpp BI_RGB, bottom-up, no palette.
No third-party module, because this runs on a CI runner whose Python has only
what the workflow pip-installs for the build itself.
"""

import argparse
import struct
import sys
from collections import Counter

# Tolerance per channel. Nothing here should be resampled, but alpha blending
# against the backdrop and any driver-side dithering can move a channel by a
# little; a window this size makes false negatives far more likely than a
# coincidental match.
TOLERANCE = 8

PAINT_COLOR = (0x10, 0x40, 0xc0)
IMAGE_COLOR = (0xff, 0xd0, 0x00)

# Fractions of the total pixel count. The fill alone is 220x120 on a window of
# at least 1024x768, i.e. ~3%; these are deliberately an order of magnitude
# below the expected areas so a different window size or DPI cannot fail them.
MIN_PAINT_FRACTION = 0.002
MIN_IMAGE_FRACTION = 0.0002
MIN_INK_FRACTION = 0.0001


def read_bmp(path):
    """Return (width, height, [(r, g, b), ...]) for a 32bpp BI_RGB BMP."""
    with open(path, "rb") as handle:
        data = handle.read()

    if data[:2] != b"BM":
        raise ValueError("not a BMP file")
    if len(data) < 54:
        raise ValueError("BMP header is truncated (%d bytes)" % len(data))
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    header_size, width, height = struct.unpack_from("<Iii", data, 14)
    bit_count = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    if header_size < 40 or bit_count != 32 or compression != 0:
        raise ValueError(
            "expected a 32bpp BI_RGB BMP, got %dbpp compression=%d"
            % (bit_count, compression)
        )

    # A negative height means top-down. RendererWGL writes bottom-up, but row
    # order does not matter to any check here, so just normalise the count.
    row_count = abs(height)
    stride = width * 4
    expected = pixel_offset + stride * row_count
    if len(data) < expected:
        raise ValueError(
            "truncated: %d bytes, expected at least %d" % (len(data), expected)
        )

    pixels = []
    for row in range(row_count):
        start = pixel_offset + row * stride
        chunk = data[start:start + stride]
        # BGRA on disk; the 4th byte is padding for BI_RGB, not alpha.
        for x in range(0, stride, 4):
            pixels.append((chunk[x + 2], chunk[x + 1], chunk[x]))
    return width, row_count, pixels


def close_to(pixel, target):
    return all(abs(pixel[i] - target[i]) <= TOLERANCE for i in range(3))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bmp", help="screenshot written by --screenshot")
    parser.add_argument(
        "--require",
        default="paint,image,font",
        help="comma-separated subset of paint,image,font to assert",
    )
    args = parser.parse_args()
    required = {name.strip() for name in args.require.split(",") if name.strip()}

    try:
        width, height, pixels = read_bmp(args.bmp)
    except (OSError, ValueError) as error:
        print("ERROR: %s: %s" % (args.bmp, error))
        return 1

    total = len(pixels)
    if not total:
        print("ERROR: %s has no pixels" % args.bmp)
        return 1

    counts = Counter(pixels)
    paint = sum(n for color, n in counts.items() if close_to(color, PAINT_COLOR))
    image = sum(n for color, n in counts.items() if close_to(color, IMAGE_COLOR))
    # Ink: dark but not pure black-on-black; the page has no dark backgrounds,
    # so anything this dark is a glyph.
    ink = sum(n for color, n in counts.items() if max(color) <= 0x40)

    print("screenshot %dx%d, %d distinct colours" % (width, height, len(counts)))
    print(
        "paint #1040c0: %d px (%.4f%%)" % (paint, 100.0 * paint / total)
    )
    print(
        "image #ffd000: %d px (%.4f%%)" % (image, 100.0 * image / total)
    )
    print("font ink:      %d px (%.4f%%)" % (ink, 100.0 * ink / total))

    # Deliberately not asserting a minimum colour count as a blank-render
    # proxy: a correct render with grayscale or disabled antialiasing can have
    # very few distinct colours, and the three checks below already fail on a
    # blank capture. Printed above only as a diagnostic.
    failures = []
    if "paint" in required and paint < total * MIN_PAINT_FRACTION:
        failures.append(
            "no painted fill found: #1040c0 covers %.4f%%, expected at least "
            "%.4f%%" % (100.0 * paint / total, 100.0 * MIN_PAINT_FRACTION)
        )
    if "image" in required and image < total * MIN_IMAGE_FRACTION:
        failures.append(
            "the embedded PNG did not reach the screen: #ffd000 covers "
            "%.4f%%, expected at least %.4f%%"
            % (100.0 * image / total, 100.0 * MIN_IMAGE_FRACTION)
        )
    if "font" in required and ink < total * MIN_INK_FRACTION:
        failures.append(
            "no glyphs rasterised: dark ink covers %.4f%%, expected at least "
            "%.4f%%" % (100.0 * ink / total, 100.0 * MIN_INK_FRACTION)
        )

    for failure in failures:
        print("ERROR: %s" % failure)
    if failures:
        return 1
    print("render check OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
