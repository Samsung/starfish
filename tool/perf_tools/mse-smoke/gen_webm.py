#!/usr/bin/env python3
"""Generate a minimal MSE-compliant WebM init segment + media segments.

Container structure is fully valid EBML/WebM; frame payloads are dummy
bytes (the platform decoder may reject them, but the demux/MSE pipeline
- the part under test - processes them as real packets)."""
import struct
import sys


def ebml_id(i):
    return bytes.fromhex(i)


def vint(n):
    # EBML variable-size integer (data size form)
    for length in range(1, 9):
        if n < (1 << (7 * length)) - 1:
            b = n | (1 << (7 * length))
            return b.to_bytes(length, "big")
    raise ValueError(n)


def el(eid, payload):
    return ebml_id(eid) + vint(len(payload)) + payload


def uint_el(eid, value):
    b = value.to_bytes(max(1, (value.bit_length() + 7) // 8), "big")
    return el(eid, b)


def str_el(eid, s):
    return el(eid, s.encode())


def float_el(eid, f):
    return el(eid, struct.pack(">d", f))


def init_segment():
    ebml = el("1A45DFA3",
              uint_el("4286", 1) +      # EBMLVersion
              uint_el("42F7", 1) +      # EBMLReadVersion
              uint_el("42F2", 4) +      # EBMLMaxIDLength
              uint_el("42F3", 8) +      # EBMLMaxSizeLength
              str_el("4282", "webm") +  # DocType
              uint_el("4287", 2) +      # DocTypeVersion
              uint_el("4285", 2))       # DocTypeReadVersion

    info = el("1549A966",
              uint_el("2AD7B1", 1000000) +   # TimecodeScale (1ms)
              float_el("4489", 10000.0) +    # Duration (timecode units)
              str_el("4D80", "sfgen") +      # MuxingApp
              str_el("5741", "sfgen"))       # WritingApp

    video = el("E0",
               uint_el("B0", 320) +    # PixelWidth
               uint_el("BA", 240))     # PixelHeight

    track = el("AE",
               uint_el("D7", 1) +                  # TrackNumber
               uint_el("73C5", 0xDEADBEEF) +       # TrackUID
               uint_el("83", 1) +                  # TrackType: video
               uint_el("9C", 0) +                  # FlagLacing
               uint_el("23E383", 33000000) +       # DefaultDuration (ns)
               str_el("86", "V_VP9") +             # CodecID
               video)

    tracks = el("1654AE6B", track)

    # Segment with unknown size (streaming): ID + 0xFF size marker
    segment_header = ebml_id("18538067") + b"\xFF"
    return ebml + segment_header + info + tracks


def simple_block(track, timecode_rel, keyframe, payload):
    flags = 0x80 if keyframe else 0x00
    head = vint(track) + struct.pack(">h", timecode_rel) + bytes([flags])
    return el("A3", head + payload)


def cluster(timecode_ms, n_frames, frame_ms, payload_size):
    body = uint_el("E7", timecode_ms)
    for i in range(n_frames):
        payload = bytes([(i * 31 + j) & 0xFF for j in range(payload_size)])
        body += simple_block(1, i * frame_ms, i == 0, payload)
    return el("1F43B675", body)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/sf-test"
    with open(out + "/init.webm", "wb") as f:
        f.write(init_segment())
    # 5 clusters of 30 frames x 33ms = ~1s each, 2KB dummy frames
    for c in range(5):
        with open(out + "/seg%d.webm" % c, "wb") as f:
            f.write(cluster(c * 990, 30, 33, 2048))
    print("generated init + 5 segments")
