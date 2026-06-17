# MP4 AVC NALU scan equivalence test

Standalone correctness proof for the commit *"perf(demux): scan AVC sample
NALUs once in MP4PacketGenerator"* (cycle 32).

That change replaced the second `MP4AVCParser::parseNext` pass in
`generateForAVC` with a record/replay of the NALU boundaries gathered by the
first pass. `equiv.cpp` reproduces `parseNext` and **both** algorithms
verbatim:

- `genOld` — the pre-change two-pass version (parse to count, parse again to copy).
- `genNew` — the post-change record/replay version.

It then runs both over 200,000 randomized length-prefixed NALU samples
(NAL size length 1/2/4, mixed NALU types including SPS/PPS/IDR, extra data
present and absent) and asserts the AnnexB output bytes, `resultSize`, and
`hasIdr` are identical.

## Run

```sh
g++ -O2 -std=c++11 -Wall tools/mp4-avc-equiv/equiv.cpp -o /tmp/avc_equiv
/tmp/avc_equiv          # expect: cases=200000 mismatches=0  (exit 0)
```

## Scope

This proves the scan/copy refactor is output-identical. It is a **frozen
snapshot** of the relevant `MP4PacketGenerator.cpp` logic, not auto-synced
with the source — re-verify against the source if `generateForAVC` changes.

It does NOT exercise `DemuxerMP4`'s box parsing (ftyp/moov/moof/trun/mdat);
end-to-end fMP4/H.264 pipeline coverage remains a harness gap (no ffmpeg in
this environment to mint a valid fMP4 fixture).
