# Functional Requirements: platform-multimedia

> **Relevant source files**
> - [`MediaPlayer.h`](src/platform/multimedia/MediaPlayer.h#L62)
> - [`StreamInfo.h`](src/platform/multimedia/StreamInfo.h#L52)
> - [`DemuxerSource.h`](src/platform/multimedia/DemuxerSource.h#L26)

## FR-001: Media Playback
**Description**: MediaPlayer provides play(), pause(), seek(), destroy() for media playback control.
**Source**: [`MediaPlayer`](src/platform/multimedia/MediaPlayer.h#L62)

## FR-002: Codec Support
**Description**: Supports audio codecs (AAC, MP3, Vorbis, Opus) and video codecs (H264, HEVC, VP9, AV1).
**Source**: [`MediaCodec`](src/platform/multimedia/StreamInfo.h#L59)

## FR-003: Stream Metadata
**Description**: StreamInfo provides stream type, codec, duration, framerate, resolution, audio channels/sample rate.
**Source**: [`StreamInfo`](src/platform/multimedia/StreamInfo.h#L91)

## FR-004: Playback State Machine
**Description**: MediaPlayer tracks PlaybackState (NONE, PLAYING, PAUSED, END) and SeekState (NO_SEEK, SEEKING, WAITING).
**Source**: [`PlaybackState`](src/platform/multimedia/MediaPlayer.h#L67)

## FR-005: Seek Supersede
**Description**: supersedeSeek() allows backends to retarget in-flight seeks to prevent deadlock in push-model backends.
**Source**: [`supersedeSeek`](src/platform/multimedia/MediaPlayer.h#L96)

## FR-006: Platform-Specific Players
**Description**: MediaPlayer::create() factory creates platform-specific players (Linux, Tizen, TV, WebRTC, ESPlusPlayer).
**Source**: [`create`](src/platform/multimedia/MediaPlayer.h#L82)

## FR-007: Loop Playback
**Description**: MediaPlayer supports loop playback via setLoop()/loop().
**Source**: [`setLoop`](src/platform/multimedia/MediaPlayer.h#L105)

## FR-008: Video Resolution Limit
**Description**: Maximum video resolution is 1920x1080 (STARFISH_VIDEO_MAX_WIDTH/HEIGHT).
**Source**: [`MediaPlayerLinux.cpp:134`](src/platform/multimedia/MediaPlayerLinux.cpp#L134)
