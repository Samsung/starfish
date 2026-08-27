# Functional Requirements: platform-system

> **Relevant source files**
> - [`PlatformFile.h`](src/platform/file/PlatformFile.h#L33)
> - [`DeviceInfo.h`](src/platform/public/DeviceInfo.h#L25)
> - [`ScreenOrientationType.h`](src/platform/public/ScreenOrientationType.h#L24)

## FR-001: File I/O
**Description**: PlatformFile provides open, read, write, seek, readLine, writeLine for file operations.
**Source**: [`PlatformFile`](src/platform/file/PlatformFile.h#L33)

## FR-002: File Utilities
**Description**: PlatformFileUtil provides removeFile, absolutePath, joinPath utilities.
**Source**: [`PlatformFileUtil`](src/platform/file/PlatformFile.h#L25)

## FR-003: Device Information
**Description**: DeviceInfo.getLocalIPAddress queries local IP address for a given network interface.
**Source**: [`DeviceInfo`](src/platform/public/DeviceInfo.h#L25)

## FR-004: Screen Orientation
**Description**: ScreenOrientationType defines orientation values: Undefined, PortraitPrimary, PortraitSecondary, LandscapePrimary, LandscapeSecondary.
**Source**: [`ScreenOrientationType`](src/platform/public/ScreenOrientationType.h#L24)

## FR-005: Text-to-Speech
**Description**: TTSBase provides text-to-speech capability with platform implementations (TTSTizen, TTSTV).
**Source**: [`TTSBase`](src/platform/tts/TTSBase.cpp#L1)
