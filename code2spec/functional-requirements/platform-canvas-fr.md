# Functional Requirements: platform-canvas

> **Relevant source files**
> - [`CompositorCairo.cpp`](src/platform/canvas/CompositorCairo.cpp#L42)
> - [`CompositorGL.cpp`](src/platform/canvas/CompositorGL.cpp#L1)
> - [`FontImplCairo.h`](src/platform/canvas/font/FontImplCairo.h#L1)

## FR-001: Cairo Compositing
**Description**: CompositorImplCairo renders to cairo_image_surface with ARGB32 format and device pixel ratio scaling.
**Source**: [`CompositorImplCairo`](src/platform/canvas/CompositorCairo.cpp#L42)

## FR-002: GL Compositing
**Description**: CompositorGL provides hardware-accelerated compositing using OpenGL with Clipper2 path clipping.
**Source**: [`CompositorGL`](src/platform/canvas/CompositorGL.cpp#L1)

## FR-003: Mock Compositing
**Description**: CompositorMock provides a no-op compositor for headless testing.
**Source**: [`CompositorMock`](src/platform/canvas/CompositorMock.cpp#L1)

## FR-004: Path Rendering
**Description**: PathCairo/PathMock provide path operations (moveTo, lineTo, arc) for vector drawing.
**Source**: [`PathCairo.h`](src/platform/canvas/PathCairo.h#L1)

## FR-005: Font Rendering
**Description**: FontImplCairo renders text using Cairo, Freetype, and HarfBuzz for glyph shaping.
**Source**: [`FontImplCairo.h`](src/platform/canvas/font/FontImplCairo.h#L1)

## FR-006: Backend Selection
**Description**: The module supports three compositing backends (Cairo, GL, Mock) selected at build/runtime.
**Source**: [`CompositorCairo.cpp`](src/platform/canvas/CompositorCairo.cpp#L42)
