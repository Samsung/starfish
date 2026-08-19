/*
 *  Copyright (C) 2011 Google Inc. All rights reserved.
 *  Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *  Copyright (C) 2012 Igalia S.L.
 *  Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "Starfish.h"

#if !defined(STARFISH_HEADLESS)

#include "core/style/Style.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/canvas/CompositorFactory.h"
#include "core/dom/canvas/webgl/gl/SurfaceCreationScope.h"

#if defined(STARFISH_ENABLE_WEBGL)
#include "core/dom/canvas/webgl/gl/GLContext.h"
#endif

#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
#include "platform/multimedia/MediaPlayerLinux.h"
#endif

#include <array>
#include <clipper2/clipper.h>
#include <stdlib.h>

#if defined(STARFISH_ENABLE_TEST)
#include <dlfcn.h>
#endif

namespace std {
template <>
struct tuple_size<Clipper2Lib::PointD> : integral_constant<size_t, 2> {};

template <>
struct tuple_element<0, Clipper2Lib::PointD> {
    typedef double type;
};

template <>
struct tuple_element<1, Clipper2Lib::PointD> {
    typedef double type;
};

template <std::size_t N>
const typename std::tuple_element<N, Clipper2Lib::PointD>::type& get(
    const Clipper2Lib::PointD& p);

template <>
inline const double& get<0>(const Clipper2Lib::PointD& p)
{
    return p.x;
}

template <>
inline const double& get<1>(const Clipper2Lib::PointD& p)
{
    return p.y;
}
} // namespace std

#include <earcut.hpp>
// The number type to use for tessellation
using Coord = double;
// The index type. Defaults to uint32_t, but you can also pass uint16_t if you
// know that your
// data won't have more than 65536 vertices.
using N = uint16_t;
// Create array
using Point = std::array<Coord, 2>;

#include "platform/canvas/gl/IncludeGL.h"
#include "platform/canvas/gl/GL.h"

#if defined(STARFISH_ENABLE_TEST) && defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
namespace Starfish {
void dumpTextureToPNG(GL* gl, GLuint textureId, int width, int height,
                      const char* path, GLenum textureTarget = GL_TEXTURE_2D);
}
#endif

#if defined(STARFISH_ANDROID)
static void logEglError(const char* name) noexcept
{
    const char* err;
    switch (eglGetError()) {
    case EGL_NOT_INITIALIZED:
        err = "EGL_NOT_INITIALIZED";
        break;
    case EGL_BAD_ACCESS:
        err = "EGL_BAD_ACCESS";
        break;
    case EGL_BAD_ALLOC:
        err = "EGL_BAD_ALLOC";
        break;
    case EGL_BAD_ATTRIBUTE:
        err = "EGL_BAD_ATTRIBUTE";
        break;
    case EGL_BAD_CONTEXT:
        err = "EGL_BAD_CONTEXT";
        break;
    case EGL_BAD_CONFIG:
        err = "EGL_BAD_CONFIG";
        break;
    case EGL_BAD_CURRENT_SURFACE:
        err = "EGL_BAD_CURRENT_SURFACE";
        break;
    case EGL_BAD_DISPLAY:
        err = "EGL_BAD_DISPLAY";
        break;
    case EGL_BAD_SURFACE:
        err = "EGL_BAD_SURFACE";
        break;
    case EGL_BAD_MATCH:
        err = "EGL_BAD_MATCH";
        break;
    case EGL_BAD_PARAMETER:
        err = "EGL_BAD_PARAMETER";
        break;
    case EGL_BAD_NATIVE_PIXMAP:
        err = "EGL_BAD_NATIVE_PIXMAP";
        break;
    case EGL_BAD_NATIVE_WINDOW:
        err = "EGL_BAD_NATIVE_WINDOW";
        break;
    case EGL_CONTEXT_LOST:
        err = "EGL_CONTEXT_LOST";
        break;
    default:
        err = "unknown";
        break;
    }
    STARFISH_LOG_ERROR("%s failed with %s", name, err);
}
#endif

#if defined(STARFISH_TIZEN)
#define EVAS_GL_IMAGE_PRESERVED 0x30D2
#define EVAS_GL_NATIVE_SURFACE_TIZEN 0x32A1
#if defined(STARFISH_SHELL_EFL) && defined(STARFISH_GLIB_CAIRO_GL)
#include <tbm_surface.h>
typedef GLint EGLint;
#define EGL_TRUE 1
#define EGL_NONE 0x3038
#define EGL_IMAGE_PRESERVED_KHR 0x30D2
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>

#ifndef EGL_DMA_BUF_PLANE3_FD_EXT
#define EGL_DMA_BUF_PLANE3_FD_EXT 0x3440
#endif
#ifndef EGL_DMA_BUF_PLANE3_OFFSET_EXT
#define EGL_DMA_BUF_PLANE3_OFFSET_EXT 0x3441
#endif
#ifndef EGL_DMA_BUF_PLANE3_PITCH_EXT
#define EGL_DMA_BUF_PLANE3_PITCH_EXT 0x3442
#endif
#define EGL_ATTRIBUTE_MAX 50

#define EGL_NATIVE_SURFACE_TIZEN 0x32A1

#define RETURN_IF_INVALID_INDEX(atti, attrib_max) \
    if ((atti) >= (attrib_max)) {                 \
        return false;                             \
    }

static bool prepareEglAttributeList(EGLint* attribs, int attrib_max,
                                    tbm_surface_h tbm_surface)
{
    int atti = 0;
    tbm_bo tbo = nullptr;
    int bo_idx, num_planes, i;
    int plane_fd_ext[] = { EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE1_FD_EXT,
                           EGL_DMA_BUF_PLANE2_FD_EXT,
                           EGL_DMA_BUF_PLANE3_FD_EXT };
    int plane_offset_ext[] = { EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                               EGL_DMA_BUF_PLANE1_OFFSET_EXT,
                               EGL_DMA_BUF_PLANE2_OFFSET_EXT,
                               EGL_DMA_BUF_PLANE3_OFFSET_EXT };
    int plane_pitch_ext[] = { EGL_DMA_BUF_PLANE0_PITCH_EXT,
                              EGL_DMA_BUF_PLANE1_PITCH_EXT,
                              EGL_DMA_BUF_PLANE2_PITCH_EXT,
                              EGL_DMA_BUF_PLANE3_PITCH_EXT };

    tbm_surface_info_s info;
    if (tbm_surface_get_info(tbm_surface, &info) != TBM_SURFACE_ERROR_NONE) {
        return false;
    }

    attribs[atti++] = EGL_WIDTH;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    attribs[atti++] = info.width;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    attribs[atti++] = EGL_HEIGHT;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    attribs[atti++] = info.height;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    attribs[atti++] = info.format;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    num_planes = tbm_surface_internal_get_num_planes(info.format);
    for (i = 0; i < num_planes; i++) {
        bo_idx = tbm_surface_internal_get_plane_bo_idx(tbm_surface, i);
        tbo = tbm_surface_internal_get_bo(tbm_surface, bo_idx);
        attribs[atti++] = plane_fd_ext[i];
        RETURN_IF_INVALID_INDEX(atti, attrib_max);

        attribs[atti++] =
            (int)(size_t)tbm_bo_get_handle(tbo, TBM_DEVICE_3D).ptr;
        RETURN_IF_INVALID_INDEX(atti, attrib_max);

        attribs[atti++] = plane_offset_ext[i];
        RETURN_IF_INVALID_INDEX(atti, attrib_max);

        attribs[atti++] = info.planes[i].offset;
        RETURN_IF_INVALID_INDEX(atti, attrib_max);

        attribs[atti++] = plane_pitch_ext[i];
        RETURN_IF_INVALID_INDEX(atti, attrib_max);

        attribs[atti++] = info.planes[i].stride;
        RETURN_IF_INVALID_INDEX(atti, attrib_max);
    }
    attribs[atti++] = EGL_NONE;
    RETURN_IF_INVALID_INDEX(atti, attrib_max);

    return true;
}
#undef RETURN_IF_INVALID_INDEX
#endif

#endif

namespace Starfish {

static bool g_needsCheckCompatibility = true;
static bool g_isOpenGLES3 = false;
static bool g_isSupportExtensionEGLImageExternal = false;
static bool g_isSupportBGRATexture = false;
static bool g_isSupportTextureSwizzle = false;
static bool g_shouldUseEGLImageOnPlainSurface = true;
static bool g_needsRGBShuffle = true;
static bool g_isSupported_EGL_NATIVE_SURFACE_TIZEN = false;

// Generic canvas textures use immutable storage (glTexStorage2D) instead of
// glTexImage2D, so the driver never has to re-validate/reallocate storage on
// later updates. Always allocates as plain GL_RGBA8 (core ES3, no BGRA
// extension dependency) and corrects channel order at sample time via the
// fixed-function GL_TEXTURE_SWIZZLE_R/G/B/A state (core since ES 3.0) instead
// of relying on GL_EXT_texture_format_BGRA8888 having a matching sized
// internalformat for texStorage2D
static bool immutableTextureUpload()
{
    return g_isOpenGLES3;
}

#ifndef MIN_MAX_TEXTURE_SIZE
#define MIN_MAX_TEXTURE_SIZE 2048
#endif
static size_t g_maxTextureSize = MIN_MAX_TEXTURE_SIZE;

static void checkError(GL* gl)
{
#if !defined(NDEBUG)
    volatile auto error = gl->getError();
    if (error != 0) {
        STARFISH_LOG_ERROR("OpenGL error.. 0x%04x", error);
        STARFISH_ASSERT_NOT_REACHED();
    }
#endif
}

static GLuint loadShader(GL* gl, GLenum type, const GLchar* shaderSrc)
{
    GLuint shader;
    GLint compiled;

    checkError(gl);
    LongTaskFinder t("loadShader");

    // Create the shader object
    shader = gl->createShader(type);

    // Load the shader source
    gl->shaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    gl->compileShader(shader);

    // Check the compile status
    gl->getShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    checkError(gl);

    if (!compiled) {
        GLint maxLength = 0;
        gl->getShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        gl->getShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        STARFISH_LOG_ERROR("loadShader error.. shader source -> %s", shaderSrc);
        STARFISH_LOG_ERROR("loadShader error.. error desc -> %s",
                           errorLog.data());
        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        gl->deleteShader(shader); // Don't leak the shader.
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return shader;
}

inline static GLenum textureFormat()
{
    GLenum kind = GL_RGBA;
#if defined(PORT_PIXEL_ORDER_BGRA)
    if (g_isSupportBGRATexture) {
        kind = GL_BGRA_EXT;
    }
#endif

    if (g_needsCheckCompatibility) {
        STARFISH_LOG_ERROR("Read textureFormat before check compatibility");
        STARFISH_ASSERT_NOT_REACHED();
    }

    return kind;
}

template <typename T, typename D = double>
static Unit::Rect toRect(const T& path)
{
    D minX = std::get<0>(path[0]);
    D minY = std::get<1>(path[0]);
    D maxX = std::get<0>(path[0]);
    D maxY = std::get<1>(path[0]);

    for (size_t j = 1; j < 4; j++) {
        minX = std::min(std::get<0>(path[j]), minX);
        minY = std::min(std::get<1>(path[j]), minY);
        maxX = std::max(std::get<0>(path[j]), maxX);
        maxY = std::max(std::get<1>(path[j]), maxY);
    }
    return Unit::Rect(minX, minY, maxX - minX, maxY - minY);
}

static Unit::Rect toRect(float (&path)[4][2])
{
    float minX = path[0][0];
    float minY = path[0][1];
    float maxX = path[0][0];
    float maxY = path[0][1];

    for (size_t j = 1; j < 4; j++) {
        minX = std::min(path[j][0], minX);
        minY = std::min(path[j][1], minY);
        maxX = std::max(path[j][0], maxX);
        maxY = std::max(path[j][1], maxY);
    }
    return Unit::Rect(minX, minY, maxX - minX, maxY - minY);
}

static Clipper2Lib::PathD toPath(const Unit::Rect& rect)
{
    Clipper2Lib::PathD path;
    path.reserve(4);
    path.emplace_back(rect.x(), rect.y());
    path.emplace_back(rect.maxX(), rect.y());
    path.emplace_back(rect.maxX(), rect.maxY());
    path.emplace_back(rect.x(), rect.maxY());
    return path;
}

static Clipper2Lib::PathsD toPaths(const Unit::Rect& rect)
{
    if (rect.isEmpty()) {
        return {};
    }
    return { toPath(rect) };
}

static bool isRectangleClipPath(const Clipper2Lib::PathsD& paths)
{
    if (paths.size() != 1) {
        return false;
    }

    const auto& p = paths[0];
    if (p.size() != 4) {
        return false;
    }

    auto x1 = p[0].x, x2 = p[0].x;
    auto y1 = p[0].y, y2 = p[0].y;

    for (size_t i = 1; i < 4; i++) {
        if (p[i].x != x1) {
            x2 = p[i].x;
        }
        if (p[i].y != y1) {
            y2 = p[i].y;
        }
    }

    int xcnt = (x1 == x2) ? 1 : 2;
    int ycnt = (y1 == y2) ? 1 : 2;

    return (xcnt == 2 && ycnt == 2);
}

static size_t roundUpToPowerOfTwo(size_t n)
{
    if (n <= 0) {
        return 1;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if defined(STARFISH_64)
    n |= n >> 32; // 64-bit size_t
#endif
    return n + 1;
}

struct CompositorImplGLState {
    bool matrixStaysInRect;
    SkMatrix matrix;
    float opacity;
    float blurRadius;
    Unit::Color color;
    Unit::Rect clipRect;

    Clipper2Lib::PathsD abbreviatedClipPaths;

    struct PathCommand {
        enum class Command { MoveTo, LineTo, ArcNegative };
        Command command;
        float x;
        float y;
        float data[3];
        SkMatrix matrix;

        bool operator==(const PathCommand& src) const
        {
            return command == src.command && x == src.x && y == src.y &&
                   data[0] == src.data[0] && data[1] == src.data[1] &&
                   data[2] == src.data[2] && matrix == src.matrix;
        }
    };
    std::vector<std::vector<PathCommand>> pathCommands;
    Optional<Clipper2Lib::PathsD> computedPathCommands;

    // Analytic rounded-rectangle clip (logical-screen space).
    // Up to kMaxAnalyticRoundedClips rounded-rect clips are tracked; the
    // fragment shader evaluates all of their SDFs and multiplies coverage.
    // Nested clips are compressed (outer discarded, inner kept) to save slots.
    // If the chain exceeds the limit, roundedClipChainOk is cleared and the
    // mask-FBO path is used instead.
    static constexpr int kMaxAnalyticRoundedClips = 4;
    struct RoundedRectClip {
        float cx = 0, cy = 0; // center
        float hx = 0, hy = 0; // half-size
        float radius = 0;
    };
    RoundedRectClip roundedRectClips[kMaxAnalyticRoundedClips];
    int roundedRectClipCount = 0;
    // True while the clip chain so far stays analytic-representable.
    // A non-rounded or overflow clip sets it false and forces the mask path.
    bool roundedClipChainOk = true;

    BlendMode blendMode;
};

struct CanvasSurfaceTextureInfo {
    struct CanvasSurfaceTextureInfoFragment {
        size_t textureID;
        size_t textureWidth;
        size_t textureHeight;
        float srcX;      // [0~1]
        float srcY;      // [0~1]
        float srcWidth;  // [0~1]
        float srcHeight; // [0~1]
        bool sharedTexture = false;
    };

    std::vector<CanvasSurfaceTextureInfoFragment> fragments;
};

class CompositorContextGL : public CompositorContext {
public:
    GLuint m_polygonVertexShader;
    GLuint m_polygonFragmentShader;
    GLuint m_polygonShaderProgram;
    GLint m_polygonShaderProgramPosition;
    GLint m_polygonShaderProgramCoverage;
    GLint m_polygonShaderProgramColor;

    // Streaming VBO for polygon fill + AA outline (avoid client-side arrays)
    GLuint m_polygonPosBuffer;
    size_t m_polygonPosBufferCapacity;

    GLuint m_rectVertexShader;
    GLuint m_pixelFragmentShader;
    GLuint m_rectShaderProgram;
    GLint m_rectShaderProgramPosition;
    GLint m_rectShaderProgramColor;
    GLint m_rectShaderProgramTexIdx;

    GLuint m_texVertexShader;
    GLuint m_texFragmentShader;
    GLuint m_texShaderProgram; // Without mask
    GLint m_texShaderProgramTexPos;
    GLint m_texShaderProgramTexIdx;
    GLint m_texShaderProgramPosition;
    GLint m_texShaderProgramTexture;
    GLint m_texShaderProgramAlpha;

    // Analytic rounded-rect clip: clips the textured quad with up to
    // kMaxAnalyticRoundedClips rounded-box SDFs in the fragment shader. Both
    // the GL_TEXTURE_2D and EGLImageExternal (video) variants share one
    // vertex shader (texVertexShaderRoundedClip); only the fragment sampler
    // type differs.
    GLuint m_texVertexShaderRoundedClip;
    GLuint m_texFragmentShaderRoundedClip;
    GLuint m_texShaderProgramRoundedClip;
    GLint m_texShaderProgramRoundedClipTexPos;
    GLint m_texShaderProgramRoundedClipTexIdx;
    GLint m_texShaderProgramRoundedClipPosition;
    GLint m_texShaderProgramRoundedClipClipPos;
    GLint m_texShaderProgramRoundedClipTexture;
    GLint m_texShaderProgramRoundedClipAlpha;
    GLint m_texShaderProgramRoundedClipRef;    // uClipRef: reference center
                                               // (vertex, highp)
    GLint m_texShaderProgramRoundedClipOffset; // uClipOffset[N]: center - ref
                                               // (fragment)
    GLint m_texShaderProgramRoundedClipHalf;   // uClipHalf[N]
    GLint m_texShaderProgramRoundedClipRadius; // uClipRadius[N]
    GLint m_texShaderProgramRoundedClipCount;  // uClipCount

    // Same analytic rounded-rect clip, for GL_TEXTURE_EXTERNAL_OES (video).
    GLuint m_texFragmentShaderRoundedClipEGLImageExternal;
    GLuint m_texShaderProgramRoundedClipEGLImageExternal;
    GLint m_texShaderProgramRoundedClipEGLImageExternalTexPos;
    GLint m_texShaderProgramRoundedClipEGLImageExternalTexIdx;
    GLint m_texShaderProgramRoundedClipEGLImageExternalPosition;
    GLint m_texShaderProgramRoundedClipEGLImageExternalClipPos;
    GLint m_texShaderProgramRoundedClipEGLImageExternalTexture;
    GLint m_texShaderProgramRoundedClipEGLImageExternalAlpha;
    GLint m_texShaderProgramRoundedClipEGLImageExternalRef;
    GLint m_texShaderProgramRoundedClipEGLImageExternalOffset;
    GLint m_texShaderProgramRoundedClipEGLImageExternalHalf;
    GLint m_texShaderProgramRoundedClipEGLImageExternalRadius;
    GLint m_texShaderProgramRoundedClipEGLImageExternalCount;

    GLuint m_texFragmentShaderWithMask;
    GLuint m_texShaderProgramWithMask; // With mask
    GLint m_texShaderProgramWithMaskTexPos;
    GLint m_texShaderProgramWithMaskTexIdx;
    GLint m_texShaderProgramWithMaskPosition;
    GLint m_texShaderProgramWithMaskTexture;
    GLint m_texShaderProgramWithMaskAlpha;
    GLint m_texShaderProgramWithMaskMaskTexture;
    GLint m_texShaderProgramWithMaskMaskUV;

    GLuint m_texFragmentShaderEGLImageExternal;
    GLuint m_texShaderProgramEGLImageExternal; // Without mask
    GLint m_texShaderProgramEGLImageExternalTexPos;
    GLint m_texShaderProgramEGLImageExternalTexIdx;
    GLint m_texShaderProgramEGLImageExternalPosition;
    GLint m_texShaderProgramEGLImageExternalTexture;
    GLint m_texShaderProgramEGLImageExternalAlpha;

    GLuint m_texFragmentShaderEGLImageExternalWithMask;
    GLuint m_texShaderProgramEGLImageExternalWithMask; // With mask
    GLint m_texShaderProgramEGLImageExternalWithMaskTexPos;
    GLint m_texShaderProgramEGLImageExternalWithMaskTexIdx;
    GLint m_texShaderProgramEGLImageExternalWithMaskPosition;
    GLint m_texShaderProgramEGLImageExternalWithMaskTexture;
    GLint m_texShaderProgramEGLImageExternalWithMaskAlpha;
    GLint m_texShaderProgramEGLImageExternalWithMaskMaskTexture;
    GLint m_texShaderProgramEGLImageExternalWithMaskMaskUV;

    GLuint m_texFragmentBlurShaderW;
    GLuint m_texFragmentBlurShaderEGLImageExternalW;
    GLuint m_texFragmentBlurShaderH;

    GLuint m_texBlurShaderProgramW;
    GLint m_texBlurShaderProgramWTexPos;
    GLint m_texBlurShaderProgramWTexIdx;
    GLint m_texBlurShaderProgramWPosition;
    GLint m_texBlurShaderProgramWTexture;
    GLint m_texBlurShaderProgramWBlurRadius;
    GLint m_texBlurShaderProgramWTextureWidth;
    GLint m_texBlurShaderProgramWTextureHeight;
    GLint m_texBlurShaderProgramWAlphaMask;
    GLuint m_texBlurShaderProgramEGLImageExternalW;
    GLint m_texBlurShaderProgramEGLImageExternalWTexPos;
    GLint m_texBlurShaderProgramEGLImageExternalWTexIdx;
    GLint m_texBlurShaderProgramEGLImageExternalWPosition;
    GLint m_texBlurShaderProgramEGLImageExternalWTexture;
    GLint m_texBlurShaderProgramEGLImageExternalWBlurRadius;
    GLint m_texBlurShaderProgramEGLImageExternalWTextureWidth;
    GLint m_texBlurShaderProgramEGLImageExternalWTextureHeight;
    GLint m_texBlurShaderProgramEGLImageExternalWAlphaMask;

    GLuint m_texFragmentBlurShaderHWithMask;
    GLuint m_texBlurShaderProgramH; // Without mask
    GLint m_texBlurShaderProgramHTexPos;
    GLint m_texBlurShaderProgramHTexIdx;
    GLint m_texBlurShaderProgramHPosition;
    GLint m_texBlurShaderProgramHTexture;
    GLint m_texBlurShaderProgramHBlurRadius;
    GLint m_texBlurShaderProgramHTextureWidth;
    GLint m_texBlurShaderProgramHTextureHeight;
    GLint m_texBlurShaderProgramHAlpha;

    GLuint m_texBlurShaderProgramHWithMask; // With mask
    GLint m_texBlurShaderProgramHWithMaskTexPos;
    GLint m_texBlurShaderProgramHWithMaskTexIdx;
    GLint m_texBlurShaderProgramHWithMaskPosition;
    GLint m_texBlurShaderProgramHWithMaskTexture;
    GLint m_texBlurShaderProgramHWithMaskBlurRadius;
    GLint m_texBlurShaderProgramHWithMaskTextureWidth;
    GLint m_texBlurShaderProgramHWithMaskTextureHeight;
    GLint m_texBlurShaderProgramHWithMaskAlpha;
    GLint m_texBlurShaderProgramHWithMaskMaskTexture;
    GLint m_texBlurShaderProgramHWithMaskMaskUV;

    GLuint m_texTexPosBuffer;
    GLuint m_texIdxBuffer;

    GLuint m_lastProgram;

    // Tracks the GL_BLEND enable state so we can lazily toggle blending
    // (skipping it for fully opaque fills) without issuing redundant calls.
    // Blending is enabled when the GL context is set up, so this starts true.
    bool m_blendEnabled;

    std::vector<std::tuple<size_t, size_t, GLuint, GLenum>> m_cachedTextures;

    struct FBOCacheEntry {
        GLuint fboId = 0;
        GLuint textureId = 0;
        size_t width = 0;
        size_t height = 0;
        GLenum format = GL_RGBA;
    };
    std::vector<FBOCacheEntry> m_cachedFBOs;

    struct ClipPathCacheKey {
        Unit::Rect clipRect;
        size_t pathCommandsHash;
        std::vector<std::vector<CompositorImplGLState::PathCommand>>
            pathCommands;
    };

    struct ClipPathCacheKeyHash {
        size_t operator()(const ClipPathCacheKey& key) const
        {
            size_t h1 = std::hash<double>{}(key.clipRect.x());
            size_t h2 = std::hash<double>{}(key.clipRect.y());
            size_t h3 = std::hash<double>{}(key.clipRect.width());
            size_t h4 = std::hash<double>{}(key.clipRect.height());
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^
                   (key.pathCommandsHash << 4);
        }
    };

    struct MaskTextureCacheKey {
        Clipper2Lib::PathsD clipPaths;
        Unit::Rect clipRect;
        size_t width;
        size_t height;
        GLenum format;

        bool operator==(const MaskTextureCacheKey& other) const
        {
            if (clipRect != other.clipRect || width != other.width ||
                height != other.height || format != other.format) {
                return false;
            }
            if (clipPaths.size() != other.clipPaths.size()) {
                return false;
            }
            for (size_t i = 0; i < clipPaths.size(); i++) {
                if (clipPaths[i].size() != other.clipPaths[i].size()) {
                    return false;
                }
                for (size_t j = 0; j < clipPaths[i].size(); j++) {
                    if (clipPaths[i][j] != other.clipPaths[i][j]) {
                        return false;
                    }
                }
            }
            return true;
        }
    };

    struct MaskTextureCacheKeyHash {
        size_t operator()(const MaskTextureCacheKey& key) const
        {
            size_t h = std::hash<double>{}(key.clipRect.x());
            h ^= std::hash<double>{}(key.clipRect.y()) + 0x9e3779b9 + (h << 6) +
                 (h >> 2);
            h ^= std::hash<double>{}(key.clipRect.width()) + 0x9e3779b9 +
                 (h << 6) + (h >> 2);
            h ^= std::hash<double>{}(key.clipRect.height()) + 0x9e3779b9 +
                 (h << 6) + (h >> 2);
            h ^= std::hash<size_t>{}(key.width) + 0x9e3779b9 + (h << 6) +
                 (h >> 2);
            h ^= std::hash<size_t>{}(key.height) + 0x9e3779b9 + (h << 6) +
                 (h >> 2);

            // Hash clipPaths
            for (const auto& path : key.clipPaths) {
                for (const auto& point : path) {
                    h ^= std::hash<double>{}(point.x) + 0x9e3779b9 + (h << 6) +
                         (h >> 2);
                    h ^= std::hash<double>{}(point.y) + 0x9e3779b9 + (h << 6) +
                         (h >> 2);
                }
            }

            return h;
        }
    };

    struct MaskTextureCacheEntry {
        GLuint textureId = 0;
        size_t width = 0;
        size_t height = 0;
        GLenum format = GL_RGBA;
    };

    std::unordered_map<MaskTextureCacheKey, MaskTextureCacheEntry,
                       MaskTextureCacheKeyHash>
        m_maskTextureCache;
    std::deque<MaskTextureCacheKey> m_maskTextureCacheOrder;
    size_t m_maskTextureCacheBytes = 0;
    static constexpr size_t kMaxMaskTextureCacheBytesMultiplier =
        2; // screenWidth * screenHeight * 2

    static size_t hashPathCommand(const CompositorImplGLState::PathCommand& cmd)
    {
        size_t h = std::hash<int>{}(static_cast<int>(cmd.command));
        h ^= std::hash<float>{}(cmd.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<float>{}(cmd.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        for (int i = 0; i < 3; i++) {
            h ^= std::hash<float>{}(cmd.data[i]) + 0x9e3779b9 + (h << 6) +
                 (h >> 2);
        }
        return h;
    }

    static size_t hashPathCommands(
        const std::vector<std::vector<CompositorImplGLState::PathCommand>>&
            pathCommands)
    {
        size_t h = 0;
        for (const auto& pathCmds : pathCommands) {
            for (const auto& cmd : pathCmds) {
                h ^= hashPathCommand(cmd) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
        }
        return h;
    }

    std::vector<
        std::unique_ptr<std::pair<ClipPathCacheKey, Clipper2Lib::PathsD>>>
        m_clipPathCache;

    Renderer* m_renderer;

#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
    GLuint m_mainViewTexture;
    GLuint m_mainViewFBO;
    GLuint m_mainViewRBO;
    EGLImageKHR m_mainViewImage;
#endif

    CompositorContextGL(Renderer* renderer)
    {
        STARFISH_LOG_INFO("CompositorContextGL::CompositorContextGL");

        m_renderer = renderer;
        clearGLProgramVariables();

        m_texIdxBuffer = m_texTexPosBuffer = 0;
        // Generated once by the factory and kept across program rebuilds, so
        // (like the tex buffers) zero the handle only here, not in
        // clearGLProgramVariables().
        m_polygonPosBuffer = 0;
        m_polygonPosBufferCapacity = 0;
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
        m_mainViewTexture = 0;
        m_mainViewFBO = 0;
        m_mainViewRBO = 0;
#endif
    }

    GL* gl()
    {
        return m_renderer->gl();
    }

    void clearGLProgramVariables()
    {
        m_polygonVertexShader = m_polygonShaderProgram = m_texShaderProgram = 0;
        m_texVertexShaderRoundedClip = 0;
        m_texFragmentShaderRoundedClip = 0;
        m_texShaderProgramRoundedClip = 0;
        m_texFragmentShaderRoundedClipEGLImageExternal = 0;
        m_texShaderProgramRoundedClipEGLImageExternal = 0;
        m_polygonFragmentShader = 0;
        m_polygonShaderProgramPosition = 0;
        m_polygonShaderProgramCoverage = 0;
        m_polygonShaderProgramColor = 0;

        m_rectVertexShader = m_pixelFragmentShader = m_rectShaderProgram = 0;
        m_rectShaderProgramPosition = 0;
        m_rectShaderProgramColor = 0;
        m_rectShaderProgramTexIdx = 0;

        // texShaderProgram (without mask)
        m_texShaderProgramPosition = 0;
        m_texShaderProgramTexture = 0;
        m_texShaderProgramAlpha = 0;

        // texShaderProgramWithMask
        m_texShaderProgramWithMask = 0;
        m_texShaderProgramWithMaskPosition = 0;
        m_texShaderProgramWithMaskTexture = 0;
        m_texShaderProgramWithMaskAlpha = 0;
        m_texShaderProgramWithMaskMaskTexture = 0;
        m_texShaderProgramWithMaskMaskUV = 0;
        m_texFragmentShaderWithMask = 0;

        // texShaderProgramEGLImageExternal (without mask)
        m_texShaderProgramEGLImageExternalPosition = 0;
        m_texShaderProgramEGLImageExternalTexture = 0;
        m_texShaderProgramEGLImageExternalAlpha = 0;

        // texShaderProgramEGLImageExternalWithMask
        m_texShaderProgramEGLImageExternalWithMask = 0;
        m_texShaderProgramEGLImageExternalWithMaskPosition = 0;
        m_texShaderProgramEGLImageExternalWithMaskTexture = 0;
        m_texShaderProgramEGLImageExternalWithMaskAlpha = 0;
        m_texShaderProgramEGLImageExternalWithMaskMaskTexture = 0;
        m_texShaderProgramEGLImageExternalWithMaskMaskUV = 0;
        m_texFragmentShaderEGLImageExternalWithMask = 0;

        m_texVertexShader = m_texFragmentShader = 0;
        m_texShaderProgramEGLImageExternal =
            m_texFragmentShaderEGLImageExternal = 0;
        m_texFragmentBlurShaderH = m_texFragmentBlurShaderW =
            m_texFragmentBlurShaderEGLImageExternalW = 0;
        m_texBlurShaderProgramW = m_texBlurShaderProgramEGLImageExternalW =
            m_texBlurShaderProgramH = 0;

        m_texBlurShaderProgramWPosition = 0;
        m_texBlurShaderProgramWTexture = 0;
        m_texBlurShaderProgramWBlurRadius = 0;
        m_texBlurShaderProgramWTextureWidth = 0;
        m_texBlurShaderProgramWTextureHeight = 0;
        m_texBlurShaderProgramWAlphaMask = 0;

        m_texBlurShaderProgramEGLImageExternalWPosition = 0;
        m_texBlurShaderProgramEGLImageExternalWTexture = 0;
        m_texBlurShaderProgramEGLImageExternalWBlurRadius = 0;
        m_texBlurShaderProgramEGLImageExternalWTextureWidth = 0;
        m_texBlurShaderProgramEGLImageExternalWTextureHeight = 0;
        m_texBlurShaderProgramEGLImageExternalWAlphaMask = 0;

        m_texBlurShaderProgramHPosition = 0;
        m_texBlurShaderProgramHTexture = 0;
        m_texBlurShaderProgramHBlurRadius = 0;
        m_texBlurShaderProgramHTextureWidth = 0;
        m_texBlurShaderProgramHTextureHeight = 0;
        m_texBlurShaderProgramHAlpha = 0;

        // texBlurShaderProgramHWithMask
        m_texFragmentBlurShaderHWithMask = 0;
        m_texBlurShaderProgramHWithMask = 0;
        m_texBlurShaderProgramHWithMaskPosition = 0;
        m_texBlurShaderProgramHWithMaskTexture = 0;
        m_texBlurShaderProgramHWithMaskBlurRadius = 0;
        m_texBlurShaderProgramHWithMaskTextureWidth = 0;
        m_texBlurShaderProgramHWithMaskTextureHeight = 0;
        m_texBlurShaderProgramHWithMaskAlpha = 0;
        m_texBlurShaderProgramHWithMaskMaskTexture = 0;
        m_texBlurShaderProgramHWithMaskMaskUV = 0;
        m_texBlurShaderProgramHWithMaskTexPos = 0;
        m_texBlurShaderProgramHWithMaskTexIdx = 0;

        m_texShaderProgramTexPos = 0;
        m_texShaderProgramEGLImageExternalTexPos = 0;
        m_texBlurShaderProgramWTexPos = 0;
        m_texBlurShaderProgramEGLImageExternalWTexPos = 0;
        m_texBlurShaderProgramHTexPos = 0;

        m_texShaderProgramTexIdx = 0;
        m_texShaderProgramEGLImageExternalTexIdx = 0;
        m_texBlurShaderProgramWTexIdx = 0;
        m_texBlurShaderProgramEGLImageExternalWTexIdx = 0;
        m_texBlurShaderProgramHTexIdx = 0;

        m_lastProgram = 0;
        m_blendEnabled = true;
    }

    ~CompositorContextGL()
    {
        STARFISH_LOG_INFO("CompositorContextGL::~CompositorContextGL");
        gl()->useProgram(0);

        cleanUpTextureCache();
        cleanUpFBOCache();
        cleanUpGLPrograms();

        gl()->deleteBuffers(1, &m_texTexPosBuffer);
        gl()->deleteBuffers(1, &m_texIdxBuffer);
        gl()->deleteBuffers(1, &m_polygonPosBuffer);

#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
        if (m_mainViewRBO) {
            gl()->deleteRenderbuffers(1, &m_mainViewRBO);
        }
        if (m_mainViewFBO) {
            gl()->deleteFramebuffers(1, &m_mainViewFBO);
        }
#endif
    }

    void cleanUpTextureCache()
    {
        for (size_t i = 0; i < m_cachedTextures.size(); i++) {
            gl()->deleteTextures(1, &std::get<2>(m_cachedTextures[i]));
        }
        std::vector<std::tuple<size_t, size_t, GLuint, GLenum>>().swap(
            m_cachedTextures);
    }

    void cleanUpGLPrograms()
    {
        if (m_texBlurShaderProgramW) {
            gl()->detachShader(m_texBlurShaderProgramW, m_texVertexShader);
            gl()->detachShader(m_texBlurShaderProgramW,
                               m_texFragmentBlurShaderW);
            gl()->deleteProgram(m_texBlurShaderProgramW);
        }

        if (m_texBlurShaderProgramEGLImageExternalW) {
            gl()->detachShader(m_texBlurShaderProgramEGLImageExternalW,
                               m_texVertexShader);
            gl()->detachShader(m_texBlurShaderProgramEGLImageExternalW,
                               m_texFragmentBlurShaderEGLImageExternalW);
            gl()->deleteProgram(m_texBlurShaderProgramEGLImageExternalW);
        }

        if (m_texBlurShaderProgramH) {
            gl()->detachShader(m_texBlurShaderProgramH, m_texVertexShader);
            gl()->detachShader(m_texBlurShaderProgramH,
                               m_texFragmentBlurShaderH);
            gl()->deleteProgram(m_texBlurShaderProgramH);
        }

        if (m_texBlurShaderProgramHWithMask) {
            gl()->detachShader(m_texBlurShaderProgramHWithMask,
                               m_texVertexShader);
            gl()->detachShader(m_texBlurShaderProgramHWithMask,
                               m_texFragmentBlurShaderHWithMask);
            gl()->deleteProgram(m_texBlurShaderProgramHWithMask);
        }

        if (m_texFragmentBlurShaderW) {
            gl()->deleteShader(m_texFragmentBlurShaderW);
        }

        if (m_texFragmentBlurShaderH) {
            gl()->deleteShader(m_texFragmentBlurShaderH);
        }

        if (m_texFragmentBlurShaderEGLImageExternalW) {
            gl()->deleteShader(m_texFragmentBlurShaderEGLImageExternalW);
        }

        if (m_polygonShaderProgram) {
            gl()->detachShader(m_polygonShaderProgram, m_polygonVertexShader);
            gl()->detachShader(m_polygonShaderProgram, m_polygonFragmentShader);
            gl()->deleteProgram(m_polygonShaderProgram);
            gl()->deleteShader(m_polygonVertexShader);
            gl()->deleteShader(m_polygonFragmentShader);
        }

        if (m_rectShaderProgram) {
            gl()->detachShader(m_rectShaderProgram, m_rectVertexShader);
            gl()->detachShader(m_rectShaderProgram, m_pixelFragmentShader);
            gl()->deleteProgram(m_rectShaderProgram);
            gl()->deleteShader(m_rectVertexShader);
        }

        if (m_pixelFragmentShader) {
            gl()->deleteShader(m_pixelFragmentShader);
        }

        if (m_texShaderProgramEGLImageExternal) {
            gl()->detachShader(m_texShaderProgramEGLImageExternal,
                               m_texVertexShader);
            gl()->detachShader(m_texShaderProgramEGLImageExternal,
                               m_texFragmentShaderEGLImageExternal);
            gl()->deleteProgram(m_texShaderProgramEGLImageExternal);
            gl()->deleteShader(m_texFragmentShaderEGLImageExternal);
        }

        if (m_texShaderProgramRoundedClip) {
            gl()->detachShader(m_texShaderProgramRoundedClip,
                               m_texVertexShaderRoundedClip);
            gl()->detachShader(m_texShaderProgramRoundedClip,
                               m_texFragmentShaderRoundedClip);
            gl()->deleteProgram(m_texShaderProgramRoundedClip);
            gl()->deleteShader(m_texFragmentShaderRoundedClip);
        }

        if (m_texShaderProgramRoundedClipEGLImageExternal) {
            gl()->detachShader(m_texShaderProgramRoundedClipEGLImageExternal,
                               m_texVertexShaderRoundedClip);
            gl()->detachShader(m_texShaderProgramRoundedClipEGLImageExternal,
                               m_texFragmentShaderRoundedClipEGLImageExternal);
            gl()->deleteProgram(m_texShaderProgramRoundedClipEGLImageExternal);
            gl()->deleteShader(m_texFragmentShaderRoundedClipEGLImageExternal);
        }

        if (m_texVertexShaderRoundedClip) {
            gl()->deleteShader(m_texVertexShaderRoundedClip);
        }

        if (m_texShaderProgram) {
            gl()->detachShader(m_texShaderProgram, m_texVertexShader);
            gl()->detachShader(m_texShaderProgram, m_texFragmentShader);
            gl()->deleteProgram(m_texShaderProgram);
        }

        if (m_texShaderProgramWithMask) {
            gl()->detachShader(m_texShaderProgramWithMask, m_texVertexShader);
            gl()->detachShader(m_texShaderProgramWithMask,
                               m_texFragmentShaderWithMask);
            gl()->deleteProgram(m_texShaderProgramWithMask);
        }

        if (m_texVertexShader) {
            gl()->deleteShader(m_texVertexShader);
        }

        if (m_texFragmentShader) {
            gl()->deleteShader(m_texFragmentShader);
        }

        if (m_texFragmentShaderWithMask) {
            gl()->deleteShader(m_texFragmentShaderWithMask);
        }

        if (m_texShaderProgramEGLImageExternalWithMask) {
            gl()->detachShader(m_texShaderProgramEGLImageExternalWithMask,
                               m_texVertexShader);
            gl()->detachShader(m_texShaderProgramEGLImageExternalWithMask,
                               m_texFragmentShaderEGLImageExternalWithMask);
            gl()->deleteProgram(m_texShaderProgramEGLImageExternalWithMask);
        }

        if (m_texFragmentShaderEGLImageExternalWithMask) {
            gl()->deleteShader(m_texFragmentShaderEGLImageExternalWithMask);
        }

        clearGLProgramVariables();
    }

    void putGenericTextureToCache(GLuint textureID, size_t textureDataWidth,
                                  size_t textureDataHeight,
                                  GLenum textureFormat = GL_RGBA)
    {
        m_cachedTextures.push_back(std::make_tuple(
            textureDataWidth, textureDataHeight, textureID, textureFormat));
    }

    GLuint takeGenericTextureFromCache(size_t textureDataWidth,
                                       size_t textureDataHeight,
                                       GLenum textureFormat = GL_RGBA)
    {
        for (size_t i = 0; i < m_cachedTextures.size(); i++) {
            if (std::get<0>(m_cachedTextures[i]) == textureDataWidth &&
                std::get<1>(m_cachedTextures[i]) == textureDataHeight &&
                std::get<3>(m_cachedTextures[i]) == textureFormat) {
                GLuint textureID = std::get<2>(m_cachedTextures[i]);
                m_cachedTextures.erase(m_cachedTextures.begin() + i);
                return textureID;
            }
        }
        return 0;
    }

    void putFBOToCache(GLuint fboId, GLuint textureId, size_t width,
                       size_t height, GLenum format)
    {
        size_t maxCacheSize = m_renderer->width() * m_renderer->height();
        size_t currentCacheSize = 0;
        for (const auto& e : m_cachedFBOs) {
            currentCacheSize += e.width * e.height;
        }
        size_t newEntrySize = width * height;
        while (!m_cachedFBOs.empty() &&
               currentCacheSize + newEntrySize > maxCacheSize) {
            FBOCacheEntry& oldest = m_cachedFBOs.front();
            currentCacheSize -= oldest.width * oldest.height;
            gl()->deleteFramebuffers(1, &oldest.fboId);
            gl()->deleteTextures(1, &oldest.textureId);
            m_cachedFBOs.erase(m_cachedFBOs.begin());
        }
        FBOCacheEntry entry;
        entry.fboId = fboId;
        entry.textureId = textureId;
        entry.width = width;
        entry.height = height;
        entry.format = format;
        m_cachedFBOs.push_back(entry);
    }

    bool takeFBOFromCache(size_t width, size_t height, GLenum format,
                          FBOCacheEntry& outEntry)
    {
        for (size_t i = 0; i < m_cachedFBOs.size(); i++) {
            auto& entry = m_cachedFBOs[i];
            if (entry.width == width && entry.height == height &&
                entry.format == format) {
                outEntry = entry;
                m_cachedFBOs.erase(m_cachedFBOs.begin() + i);
                return true;
            }
        }
        return false;
    }

    void cleanUpFBOCache()
    {
        for (auto& entry : m_cachedFBOs) {
            gl()->deleteTextures(1, &entry.textureId);
            gl()->deleteFramebuffers(1, &entry.fboId);
        }
        m_cachedFBOs.clear();
    }

    void putMaskTextureToCache(GLuint textureId, size_t width, size_t height,
                               GLenum format,
                               const Clipper2Lib::PathsD& clipPaths,
                               const Unit::Rect& clipRect)
    {
        if (textureId == 0) {
            return;
        }

        MaskTextureCacheKey key;
        key.clipPaths = clipPaths;
        key.clipRect = clipRect;
        key.width = width;
        key.height = height;
        key.format = format;

        size_t textureBytes = width * height * (format == GL_RED ? 1 : 4);
        size_t maxCacheBytes = m_renderer->width() * m_renderer->height() *
                               kMaxMaskTextureCacheBytesMultiplier;

        while (m_maskTextureCacheBytes + textureBytes > maxCacheBytes &&
               !m_maskTextureCacheOrder.empty()) {
            MaskTextureCacheKey oldestKey = m_maskTextureCacheOrder.front();
            m_maskTextureCacheOrder.pop_front();
            auto it = m_maskTextureCache.find(oldestKey);
            if (it != m_maskTextureCache.end()) {
                size_t oldBytes = it->second.width * it->second.height *
                                  (it->second.format == GL_RED ? 1 : 4);
                m_maskTextureCacheBytes -= oldBytes;
                gl()->deleteTextures(1, &it->second.textureId);
                m_maskTextureCache.erase(it);
            }
        }

        MaskTextureCacheEntry entry;
        entry.textureId = textureId;
        entry.width = width;
        entry.height = height;
        entry.format = format;

        STARFISH_ASSERT(m_maskTextureCache.find(key) ==
                        m_maskTextureCache.end());

        auto orderIt = std::find(m_maskTextureCacheOrder.begin(),
                                 m_maskTextureCacheOrder.end(), key);
        if (orderIt != m_maskTextureCacheOrder.end()) {
            m_maskTextureCacheOrder.erase(orderIt);
        }

        m_maskTextureCache[key] = entry;
        m_maskTextureCacheOrder.push_back(key);
        m_maskTextureCacheBytes += textureBytes;
    }

    GLuint takeMaskTextureFromCache(size_t width, size_t height, GLenum format,
                                    const Clipper2Lib::PathsD& clipPaths,
                                    const Unit::Rect& clipRect)
    {
        MaskTextureCacheKey key;
        key.clipPaths = clipPaths;
        key.clipRect = clipRect;
        key.width = width;
        key.height = height;
        key.format = format;

        auto it = m_maskTextureCache.find(key);
        if (it != m_maskTextureCache.end()) {
            GLuint textureId = it->second.textureId;
            auto orderIt = std::find(m_maskTextureCacheOrder.begin(),
                                     m_maskTextureCacheOrder.end(), key);
            if (orderIt != m_maskTextureCacheOrder.end()) {
                m_maskTextureCacheOrder.erase(orderIt);
            }
            m_maskTextureCacheOrder.push_back(key);

            return textureId;
        }
        return 0;
    }

    void cleanUpMaskTextureCache()
    {
        for (auto& entry : m_maskTextureCache) {
            gl()->deleteTextures(1, &entry.second.textureId);
        }
        m_maskTextureCache.clear();
        m_maskTextureCacheOrder.clear();
    }

    virtual void willRendering() override
    {
    }

#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
    virtual void prepareExternalSurface(void* externalSurface) override
    {
        gl()->bindFramebuffer(GL_FRAMEBUFFER, m_mainViewFBO);
        gl()->bindRenderbuffer(GL_RENDERBUFFER, m_mainViewRBO);

        EGLDisplay display = eglGetCurrentDisplay();

        if (g_isSupported_EGL_NATIVE_SURFACE_TIZEN) {
            EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
            m_mainViewImage =
                gl()->xglCreateImage(EGL_NATIVE_SURFACE_TIZEN,
                                     (void*)(intptr_t)externalSurface, attribs);
        } else {
            EGLint attribs[EGL_ATTRIBUTE_MAX];
            if (!prepareEglAttributeList(
                    attribs, EGL_ATTRIBUTE_MAX,
                    static_cast<tbm_surface_h>(externalSurface))) {
                return;
            }
            m_mainViewImage =
                gl()->xglCreateImage(EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
        }

        gl()->genTextures(1, &m_mainViewTexture);
        gl()->bindTexture(GL_TEXTURE_2D, m_mainViewTexture);

        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        gl()->xglImageTargetTexture2DOES(GL_TEXTURE_2D, m_mainViewImage);

        gl()->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, m_mainViewTexture, 0);
    }

    virtual void flushExternalSurface(
        const std::function<void(bool needsFlush)>& cb,
        bool isRendered) override
    {
        gl()->bindTexture(GL_TEXTURE_2D, 0);
        gl()->bindFramebuffer(GL_FRAMEBUFFER, 0);

        if (isRendered) {
            gl()->finish();
        }
        cb(isRendered);
        gl()->xglDestroyImage(m_mainViewImage);
        m_mainViewImage = nullptr;
        gl()->deleteTextures(1, &m_mainViewTexture);
    }
#endif

    virtual void didRendering() override
    {
        cleanUpTextureCache();
    }

    virtual void onIdle() override
    {
        cleanUpTextureCache();
        cleanUpFBOCache();
        cleanUpMaskTextureCache();
        cleanUpGLPrograms();
    }

    void ensurePixelFragmentShader()
    {
        if (m_pixelFragmentShader) {
            return;
        }
        const GLchar* pixelFragmentSource =
            "#ifdef GL_ES\n"
            "  precision mediump float;\n"
            "#endif\n"
            "uniform vec4 uColor;\n"
            "void main(void)\n"
            "{\n"
            "  gl_FragColor = uColor;\n"
            "}";

        if (g_needsRGBShuffle) {
            pixelFragmentSource =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform vec4 uColor;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor.r = uColor[2];\n"
                "  gl_FragColor.g = uColor[1];\n"
                "  gl_FragColor.b = uColor[0];\n"
                "  gl_FragColor.a = uColor[3];\n"
                "}";
        }
        m_pixelFragmentShader =
            loadShader(gl(), GL_FRAGMENT_SHADER, pixelFragmentSource);
        checkError(gl());
    }

    GLuint polygonProgram()
    {
        if (!m_polygonShaderProgram) {
            // Per-vertex coverage drives both the solid fill (coverage == 1)
            // and the anti-aliased outline ring (coverage fades 1 -> 0
            // outward), so fill and AA are rendered in a single pass.
            GLchar polygonVertexSource[] =
                "attribute vec2 aPosition;\n"
                "attribute float aCoverage;\n"
                "varying float vCoverage;\n"
                "void main() {\n"
                "  gl_Position = vec4(aPosition.xy, 0.0, 1.0);\n"
                "  vCoverage = aCoverage;\n"
                "}";

            m_polygonVertexShader =
                loadShader(gl(), GL_VERTEX_SHADER, polygonVertexSource);
            checkError(gl());

            // Premultiplied output (matches GL_ONE / GL_ONE_MINUS_SRC_ALPHA):
            // scaling all four channels by coverage keeps it premultiplied.
            const GLchar* polygonFragmentSource =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform vec4 uColor;\n"
                "varying float vCoverage;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor = uColor * vCoverage;\n"
                "}";

            if (g_needsRGBShuffle) {
                polygonFragmentSource =
                    "#ifdef GL_ES\n"
                    "  precision mediump float;\n"
                    "#endif\n"
                    "uniform vec4 uColor;\n"
                    "varying float vCoverage;\n"
                    "void main(void)\n"
                    "{\n"
                    "  gl_FragColor.r = uColor[2] * vCoverage;\n"
                    "  gl_FragColor.g = uColor[1] * vCoverage;\n"
                    "  gl_FragColor.b = uColor[0] * vCoverage;\n"
                    "  gl_FragColor.a = uColor[3] * vCoverage;\n"
                    "}";
            }

            m_polygonFragmentShader =
                loadShader(gl(), GL_FRAGMENT_SHADER, polygonFragmentSource);
            checkError(gl());

            m_polygonShaderProgram = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_polygonShaderProgram, m_polygonVertexShader);
            checkError(gl());
            gl()->attachShader(m_polygonShaderProgram, m_polygonFragmentShader);
            checkError(gl());

            gl()->linkProgram(m_polygonShaderProgram);
            checkError(gl());

            m_lastProgram = m_polygonShaderProgram;
            gl()->useProgram(m_polygonShaderProgram);

            m_polygonShaderProgramPosition =
                gl()->getAttribLocation(m_polygonShaderProgram, "aPosition");
            m_polygonShaderProgramCoverage =
                gl()->getAttribLocation(m_polygonShaderProgram, "aCoverage");
            m_polygonShaderProgramColor =
                gl()->getUniformLocation(m_polygonShaderProgram, "uColor");
        } else {
            if (m_lastProgram != m_polygonShaderProgram) {
                m_lastProgram = m_polygonShaderProgram;
                gl()->useProgram(m_polygonShaderProgram);
            }
        }

        return m_polygonShaderProgram;
    }

    GLuint rectProgram()
    {
        if (!m_rectShaderProgram) {
            GLchar rectVertexSource[] =
                "uniform vec2 uPosition[4];\n"
                "attribute float aTexIdx;\n"
                "void main() {\n"
                "  vec2 data = uPosition[int(aTexIdx)];\n"
                "  gl_Position = vec4(data.xy, 0.0, 1.0);\n"
                "}";

            m_rectVertexShader =
                loadShader(gl(), GL_VERTEX_SHADER, rectVertexSource);
            checkError(gl());

            ensurePixelFragmentShader();

            m_rectShaderProgram = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_rectShaderProgram, m_rectVertexShader);
            checkError(gl());
            gl()->attachShader(m_rectShaderProgram, m_pixelFragmentShader);
            checkError(gl());

            gl()->linkProgram(m_rectShaderProgram);
            checkError(gl());

            m_lastProgram = m_rectShaderProgram;
            gl()->useProgram(m_rectShaderProgram);

            m_rectShaderProgramPosition =
                gl()->getUniformLocation(m_rectShaderProgram, "uPosition");
            m_rectShaderProgramColor =
                gl()->getUniformLocation(m_rectShaderProgram, "uColor");
            m_rectShaderProgramTexIdx =
                gl()->getAttribLocation(m_rectShaderProgram, "aTexIdx");

            bindTexIdx(m_rectShaderProgramTexIdx, false);
        } else {
            if (m_lastProgram != m_rectShaderProgram) {
                m_lastProgram = m_rectShaderProgram;
                gl()->useProgram(m_rectShaderProgram);

                bindTexIdx(m_rectShaderProgramTexIdx, true);
            }
        }

        return m_rectShaderProgram;
    }

    void bindTexIdx(GLint texIdx, bool attach)
    {
        gl()->bindBuffer(GL_ARRAY_BUFFER, m_texIdxBuffer);
        if (!attach) {
            float index[4] = { 0, 1, 2, 3 };
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, index,
                             GL_STATIC_DRAW);
        }
        gl()->vertexAttribPointer(texIdx, 1, GL_FLOAT, false, 0, 0);
        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void bindTexPos(GLint texPos, bool flipY = false)
    {
        gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        if (flipY) {
            float data[] = { 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f };
            gl()->bufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8, data);
        } else {
            float data[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f };
            gl()->bufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8, data);
        }
        gl()->vertexAttribPointer(texPos, 2, GL_FLOAT, false, 0, 0);
        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Stream-upload vertex data into a GL_STREAM_DRAW VBO. Orphans the previous
    // allocation (buffer respecification) to avoid CPU/GPU sync stalls when the
    // same buffer is reused many times per frame. Leaves the buffer bound.
    void streamArrayBuffer(GLuint buffer, size_t& capacity, const void* data,
                           size_t bytes)
    {
        gl()->bindBuffer(GL_ARRAY_BUFFER, buffer);
        if (bytes > capacity) {
            capacity = bytes;
            gl()->bufferData(GL_ARRAY_BUFFER, bytes, NULL, GL_STREAM_DRAW);
        } else {
            gl()->bufferData(GL_ARRAY_BUFFER, capacity, NULL, GL_STREAM_DRAW);
        }
        gl()->bufferSubData(GL_ARRAY_BUFFER, 0, bytes, data);
    }

    GLuint texVertexShader()
    {
        if (!m_texVertexShader) {
            GLchar texVertexSource[] = R"(
            uniform vec2 uPosition[4];
            attribute vec2 aTexPos;
            attribute float aTexIdx;
            varying vec2 vTexPos;
            void main() {
              vTexPos = vec2(aTexPos.x, aTexPos.y);
              vec2 data = uPosition[int(aTexIdx)];
              gl_Position = vec4(data.xy, 0.0, 1.0);
            })";
            m_texVertexShader =
                loadShader(gl(), GL_VERTEX_SHADER, texVertexSource);
        }
        return m_texVertexShader;
    }

    // EGLImageExternal shader program without mask support (simpler, faster)
    GLuint texShaderProgramEGLImageExternal()
    {
        if (!m_texShaderProgramEGLImageExternal) {
            // Simple version without mask support
            const GLchar* texFragmentSourceEGLImageExternal =
                "#extension GL_OES_EGL_image_external : require\n"
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform samplerExternalOES uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor = texture2D(uTexture, vTexPos) * uAlpha;\n"
                "}";
            if (g_needsRGBShuffle) {
                texFragmentSourceEGLImageExternal =
                    "#extension GL_OES_EGL_image_external : require\n"
                    "#ifdef GL_ES\n"
                    "  precision mediump float;\n"
                    "#endif\n"
                    "uniform samplerExternalOES uTexture;\n"
                    "varying vec2 vTexPos;\n"
                    "uniform float uAlpha;\n"
                    "void main(void)\n"
                    "{\n"
                    "  vec4 texData = texture2D(uTexture, vTexPos) * uAlpha;\n"
                    "  gl_FragColor.r = texData[2];\n"
                    "  gl_FragColor.g = texData[1];\n"
                    "  gl_FragColor.b = texData[0];\n"
                    "  gl_FragColor.a = texData[3];\n"
                    "}";
            }

            m_texFragmentShaderEGLImageExternal = loadShader(
                gl(), GL_FRAGMENT_SHADER, texFragmentSourceEGLImageExternal);
            checkError(gl());

            m_texShaderProgramEGLImageExternal = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_texShaderProgramEGLImageExternal,
                               texVertexShader());
            checkError(gl());
            gl()->attachShader(m_texShaderProgramEGLImageExternal,
                               m_texFragmentShaderEGLImageExternal);
            checkError(gl());

            gl()->linkProgram(m_texShaderProgramEGLImageExternal);
            checkError(gl());

            m_lastProgram = m_texShaderProgramEGLImageExternal;
            gl()->useProgram(m_texShaderProgramEGLImageExternal);
            checkError(gl());

            m_texShaderProgramEGLImageExternalPosition =
                gl()->getUniformLocation(m_texShaderProgramEGLImageExternal,
                                         "uPosition");
            m_texShaderProgramEGLImageExternalTexPos = gl()->getAttribLocation(
                m_texShaderProgramEGLImageExternal, "aTexPos");
            m_texShaderProgramEGLImageExternalTexIdx = gl()->getAttribLocation(
                m_texShaderProgramEGLImageExternal, "aTexIdx");
            m_texShaderProgramEGLImageExternalTexture =
                gl()->getUniformLocation(m_texShaderProgramEGLImageExternal,
                                         "uTexture");
            m_texShaderProgramEGLImageExternalAlpha = gl()->getUniformLocation(
                m_texShaderProgramEGLImageExternal, "uAlpha");

            gl()->uniform1i(m_texShaderProgramEGLImageExternalTexture, 0);
            gl()->uniform1f(m_texShaderProgramEGLImageExternalAlpha, 1);

            gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                             GL_STREAM_DRAW);
            gl()->vertexAttribPointer(m_texShaderProgramEGLImageExternalTexPos,
                                      2, GL_FLOAT, false, 0, 0);
            gl()->bindBuffer(GL_ARRAY_BUFFER, 0);

            bindTexPos(m_texShaderProgramEGLImageExternalTexPos);
            bindTexIdx(m_texShaderProgramEGLImageExternalTexIdx, false);
        } else {
            if (m_lastProgram != m_texShaderProgramEGLImageExternal) {
                m_lastProgram = m_texShaderProgramEGLImageExternal;
                gl()->useProgram(m_texShaderProgramEGLImageExternal);

                bindTexPos(m_texShaderProgramEGLImageExternalTexPos);
                bindTexIdx(m_texShaderProgramEGLImageExternalTexIdx, true);
            }
        }

        return m_texShaderProgramEGLImageExternal;
    }

    // EGLImageExternal shader program with mask support
    GLuint texShaderProgramEGLImageExternalWithMask()
    {
        if (!m_texShaderProgramEGLImageExternalWithMask) {
            const GLchar* texFragmentSourceEGLImageExternalWithMask =
                "#extension GL_OES_EGL_image_external : require\n"
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform samplerExternalOES uTexture;\n"
                "uniform sampler2D uMaskTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "uniform vec4 uMaskUV;\n"
                "void main(void)\n"
                "{\n"
                "  vec4 texColor = texture2D(uTexture, vTexPos);\n"
                "  vec2 maskCoord = vec2(vTexPos.x * uMaskUV.z + uMaskUV.x, "
                "1.0 - (vTexPos.y * uMaskUV.w + uMaskUV.y));\n"
                "  float maskAlpha = texture2D(uMaskTexture, maskCoord).a;\n"
                "  gl_FragColor = texColor * uAlpha * maskAlpha;\n"
                "}";
            if (g_needsRGBShuffle) {
                texFragmentSourceEGLImageExternalWithMask =
                    "#extension GL_OES_EGL_image_external : require\n"
                    "#ifdef GL_ES\n"
                    "  precision mediump float;\n"
                    "#endif\n"
                    "uniform samplerExternalOES uTexture;\n"
                    "uniform sampler2D uMaskTexture;\n"
                    "varying vec2 vTexPos;\n"
                    "uniform float uAlpha;\n"
                    "uniform vec4 uMaskUV;\n"
                    "void main(void)\n"
                    "{\n"
                    "  vec4 texData = texture2D(uTexture, vTexPos);\n"
                    "  vec2 maskCoord = vec2(vTexPos.x * uMaskUV.z + "
                    "uMaskUV.x, 1.0 - (vTexPos.y * uMaskUV.w + uMaskUV.y));\n"
                    "  float maskAlpha = texture2D(uMaskTexture, "
                    "maskCoord).a;\n"
                    "  texData = texData * uAlpha * maskAlpha;\n"
                    "  gl_FragColor.r = texData[2];\n"
                    "  gl_FragColor.g = texData[1];\n"
                    "  gl_FragColor.b = texData[0];\n"
                    "  gl_FragColor.a = texData[3];\n"
                    "}";
            }

            m_texFragmentShaderEGLImageExternalWithMask =
                loadShader(gl(), GL_FRAGMENT_SHADER,
                           texFragmentSourceEGLImageExternalWithMask);
            checkError(gl());

            m_texShaderProgramEGLImageExternalWithMask = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_texShaderProgramEGLImageExternalWithMask,
                               texVertexShader());
            checkError(gl());
            gl()->attachShader(m_texShaderProgramEGLImageExternalWithMask,
                               m_texFragmentShaderEGLImageExternalWithMask);
            checkError(gl());

            gl()->linkProgram(m_texShaderProgramEGLImageExternalWithMask);
            checkError(gl());

            m_lastProgram = m_texShaderProgramEGLImageExternalWithMask;
            gl()->useProgram(m_texShaderProgramEGLImageExternalWithMask);
            checkError(gl());

            m_texShaderProgramEGLImageExternalWithMaskPosition =
                gl()->getUniformLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "uPosition");
            m_texShaderProgramEGLImageExternalWithMaskTexPos =
                gl()->getAttribLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "aTexPos");
            m_texShaderProgramEGLImageExternalWithMaskTexIdx =
                gl()->getAttribLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "aTexIdx");
            m_texShaderProgramEGLImageExternalWithMaskTexture =
                gl()->getUniformLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "uTexture");
            m_texShaderProgramEGLImageExternalWithMaskAlpha =
                gl()->getUniformLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "uAlpha");
            m_texShaderProgramEGLImageExternalWithMaskMaskTexture =
                gl()->getUniformLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "uMaskTexture");
            m_texShaderProgramEGLImageExternalWithMaskMaskUV =
                gl()->getUniformLocation(
                    m_texShaderProgramEGLImageExternalWithMask, "uMaskUV");

            gl()->uniform1i(m_texShaderProgramEGLImageExternalWithMaskTexture,
                            0);
            gl()->uniform1i(
                m_texShaderProgramEGLImageExternalWithMaskMaskTexture, 1);
            gl()->uniform1f(m_texShaderProgramEGLImageExternalWithMaskAlpha, 1);
            gl()->uniform4f(m_texShaderProgramEGLImageExternalWithMaskMaskUV, 0,
                            0, 1, 1);

            gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                             GL_STREAM_DRAW);
            gl()->vertexAttribPointer(
                m_texShaderProgramEGLImageExternalWithMaskTexPos, 2, GL_FLOAT,
                false, 0, 0);
            gl()->bindBuffer(GL_ARRAY_BUFFER, 0);

            bindTexPos(m_texShaderProgramEGLImageExternalWithMaskTexPos);
            bindTexIdx(m_texShaderProgramEGLImageExternalWithMaskTexIdx, false);
        } else {
            if (m_lastProgram != m_texShaderProgramEGLImageExternalWithMask) {
                m_lastProgram = m_texShaderProgramEGLImageExternalWithMask;
                gl()->useProgram(m_texShaderProgramEGLImageExternalWithMask);

                bindTexPos(m_texShaderProgramEGLImageExternalWithMaskTexPos);
                bindTexIdx(m_texShaderProgramEGLImageExternalWithMaskTexIdx,
                           true);
            }
        }

        return m_texShaderProgramEGLImageExternalWithMask;
    }

    // Shader program without mask support (simpler, faster)
    GLuint texShaderProgram()
    {
        if (!m_texShaderProgram) {
            // We only Support OpenGL ES 2.0+ context
            // but some develoment environment only support desktop context
            // so we add `#ifdef GL_ES` for debug purpose
            // This is the simple version without mask support
            const GLchar* texFragmentSource =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform sampler2D uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor = texture2D(uTexture, vTexPos) * uAlpha;\n"
                "}";
            if (g_needsRGBShuffle) {
                texFragmentSource =
                    "#ifdef GL_ES\n"
                    "  precision mediump float;\n"
                    "#endif\n"
                    "uniform sampler2D uTexture;\n"
                    "varying vec2 vTexPos;\n"
                    "uniform float uAlpha;\n"
                    "void main(void)\n"
                    "{\n"
                    "  vec4 texData = texture2D(uTexture, vTexPos) * uAlpha;\n"
                    "  gl_FragColor.r = texData[2];\n"
                    "  gl_FragColor.g = texData[1];\n"
                    "  gl_FragColor.b = texData[0];\n"
                    "  gl_FragColor.a = texData[3];\n"
                    "}";
            }

            m_texFragmentShader =
                loadShader(gl(), GL_FRAGMENT_SHADER, texFragmentSource);
            checkError(gl());

            m_texShaderProgram = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_texShaderProgram, texVertexShader());
            checkError(gl());
            gl()->attachShader(m_texShaderProgram, m_texFragmentShader);
            checkError(gl());

            gl()->linkProgram(m_texShaderProgram);
            checkError(gl());

            m_lastProgram = m_texShaderProgram;
            gl()->useProgram(m_texShaderProgram);
            checkError(gl());

            m_texShaderProgramPosition =
                gl()->getUniformLocation(m_texShaderProgram, "uPosition");
            m_texShaderProgramTexPos =
                gl()->getAttribLocation(m_texShaderProgram, "aTexPos");
            m_texShaderProgramTexIdx =
                gl()->getAttribLocation(m_texShaderProgram, "aTexIdx");
            m_texShaderProgramTexture =
                gl()->getUniformLocation(m_texShaderProgram, "uTexture");
            m_texShaderProgramAlpha =
                gl()->getUniformLocation(m_texShaderProgram, "uAlpha");

            gl()->uniform1i(m_texShaderProgramTexture, 0);
            gl()->uniform1f(m_texShaderProgramAlpha, 1);

            gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                             GL_STREAM_DRAW);
            gl()->vertexAttribPointer(m_texShaderProgramTexPos, 2, GL_FLOAT,
                                      false, 0, 0);
            gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
            bindTexPos(m_texShaderProgramTexPos);
            bindTexIdx(m_texShaderProgramTexIdx, false);
        } else {
            if (m_lastProgram != m_texShaderProgram) {
                m_lastProgram = m_texShaderProgram;
                gl()->useProgram(m_texShaderProgram);

                bindTexPos(m_texShaderProgramTexPos);
                bindTexIdx(m_texShaderProgramTexIdx, true);
            }
        }

        return m_texShaderProgram;
    }

    // Vertex shader shared by both analytic rounded-clip fragment programs
    // (GL_TEXTURE_2D and EGLImageExternal) - only the fragment sampler type
    // differs between them.
    GLuint texVertexShaderRoundedClip()
    {
        if (!m_texVertexShaderRoundedClip) {
            const GLchar* vertexSource =
                "uniform vec2 uPosition[4];\n"
                "uniform vec2 uClipPos[4];\n"
                "uniform vec2 uClipRef;\n"
                "attribute vec2 aTexPos;\n"
                "attribute float aTexIdx;\n"
                "varying vec2 vTexPos;\n"
                "varying vec2 vClipPos;\n"
                "void main() {\n"
                "  vTexPos = vec2(aTexPos.x, aTexPos.y);\n"
                "  int idx = int(aTexIdx);\n"
                "  gl_Position = vec4(uPosition[idx].xy, 0.0, 1.0);\n"
                "  vClipPos = uClipPos[idx] - uClipRef;\n"
                "}";
            m_texVertexShaderRoundedClip =
                loadShader(gl(), GL_VERTEX_SHADER, vertexSource);
        }
        return m_texVertexShaderRoundedClip;
    }

    // Unrolled N=4 SDF loop (GLSL ES 1.00 safe), shared by the GL_TEXTURE_2D
    // and EGLImageExternal rounded-clip fragment shaders.
    // uClipOffset[i] = clips[i].center - uClipRef (so [0] == vec2(0)).
    // coverage = product of per-clip clamp(0.5 - sdf, 0, 1) ramps.
#define RRCLIP_FRAG_BODY(SAMPLER_PREAMBLE, RGB_SHUFFLE)                        \
    SAMPLER_PREAMBLE                                                           \
    "#ifdef GL_ES\n"                                                           \
    "  precision mediump float;\n"                                             \
    "#endif\n"                                                                 \
    "uniform float uAlpha;\n"                                                  \
    "uniform vec2 uClipOffset[4];\n"                                           \
    "uniform vec2 uClipHalf[4];\n"                                             \
    "uniform float uClipRadius[4];\n"                                          \
    "uniform int uClipCount;\n"                                                \
    "varying vec2 vTexPos;\n"                                                  \
    "varying vec2 vClipPos;\n"                                                 \
    "void main(void)\n"                                                        \
    "{\n"                                                                      \
    "  vec2 pos, q;\n"                                                         \
    "  float d;\n"                                                             \
    "  pos = vClipPos - uClipOffset[0];\n"                                     \
    "  q = abs(pos) - uClipHalf[0] + vec2(uClipRadius[0]);\n"                  \
    "  d = min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-uClipRadius[0];\n"   \
    "  float coverage = clamp(0.5 - d, 0.0, 1.0);\n"                           \
    "  if (uClipCount > 1) {\n"                                                \
    "    pos = vClipPos - uClipOffset[1];\n"                                   \
    "    q = abs(pos) - uClipHalf[1] + vec2(uClipRadius[1]);\n"                \
    "    d = min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-uClipRadius[1];\n" \
    "    coverage *= clamp(0.5 - d, 0.0, 1.0);\n"                              \
    "  }\n"                                                                    \
    "  if (uClipCount > 2) {\n"                                                \
    "    pos = vClipPos - uClipOffset[2];\n"                                   \
    "    q = abs(pos) - uClipHalf[2] + vec2(uClipRadius[2]);\n"                \
    "    d = min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-uClipRadius[2];\n" \
    "    coverage *= clamp(0.5 - d, 0.0, 1.0);\n"                              \
    "  }\n"                                                                    \
    "  if (uClipCount > 3) {\n"                                                \
    "    pos = vClipPos - uClipOffset[3];\n"                                   \
    "    q = abs(pos) - uClipHalf[3] + vec2(uClipRadius[3]);\n"                \
    "    d = min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-uClipRadius[3];\n" \
    "    coverage *= clamp(0.5 - d, 0.0, 1.0);\n"                              \
    "  }\n"                                                                    \
    "  vec4 texData = texture2D(uTexture, vTexPos) * uAlpha;\n" RGB_SHUFFLE    \
    "}\n"

    // Shader program that clips a GL_TEXTURE_2D quad to the intersection of up
    // to kMaxAnalyticRoundedClips rounded rectangles via per-fragment SDFs,
    // replacing the mask-FBO path. vClipPos carries the per-vertex position
    // relative to uClipRef (subtracted in the highp vertex stage for mediump
    // accuracy in the fragment stage). Each additional clip's center offset is
    // passed as uClipOffset[i]; coverage = product of per-clip 1px ramps.
    GLuint texShaderProgramRoundedClip()
    {
        if (!m_texShaderProgramRoundedClip) {
            const GLchar* fragmentSource =
                RRCLIP_FRAG_BODY("uniform sampler2D uTexture;\n",
                                 "  gl_FragColor = texData * coverage;\n");
            if (g_needsRGBShuffle) {
                fragmentSource = RRCLIP_FRAG_BODY(
                    "uniform sampler2D uTexture;\n",
                    "  gl_FragColor.r = texData[2] * coverage;\n"
                    "  gl_FragColor.g = texData[1] * coverage;\n"
                    "  gl_FragColor.b = texData[0] * coverage;\n"
                    "  gl_FragColor.a = texData[3] * coverage;\n");
            }

            m_texFragmentShaderRoundedClip =
                loadShader(gl(), GL_FRAGMENT_SHADER, fragmentSource);
            checkError(gl());

            m_texShaderProgramRoundedClip = gl()->createProgram();
            gl()->attachShader(m_texShaderProgramRoundedClip,
                               texVertexShaderRoundedClip());
            gl()->attachShader(m_texShaderProgramRoundedClip,
                               m_texFragmentShaderRoundedClip);
            gl()->linkProgram(m_texShaderProgramRoundedClip);
            checkError(gl());

            m_lastProgram = m_texShaderProgramRoundedClip;
            gl()->useProgram(m_texShaderProgramRoundedClip);
            checkError(gl());

            GLuint p = m_texShaderProgramRoundedClip;
            m_texShaderProgramRoundedClipPosition =
                gl()->getUniformLocation(p, "uPosition");
            m_texShaderProgramRoundedClipClipPos =
                gl()->getUniformLocation(p, "uClipPos");
            m_texShaderProgramRoundedClipTexPos =
                gl()->getAttribLocation(p, "aTexPos");
            m_texShaderProgramRoundedClipTexIdx =
                gl()->getAttribLocation(p, "aTexIdx");
            m_texShaderProgramRoundedClipTexture =
                gl()->getUniformLocation(p, "uTexture");
            m_texShaderProgramRoundedClipAlpha =
                gl()->getUniformLocation(p, "uAlpha");
            m_texShaderProgramRoundedClipRef =
                gl()->getUniformLocation(p, "uClipRef");
            m_texShaderProgramRoundedClipOffset =
                gl()->getUniformLocation(p, "uClipOffset");
            m_texShaderProgramRoundedClipHalf =
                gl()->getUniformLocation(p, "uClipHalf");
            m_texShaderProgramRoundedClipRadius =
                gl()->getUniformLocation(p, "uClipRadius");
            m_texShaderProgramRoundedClipCount =
                gl()->getUniformLocation(p, "uClipCount");

            gl()->uniform1i(m_texShaderProgramRoundedClipTexture, 0);
            gl()->uniform1f(m_texShaderProgramRoundedClipAlpha, 1);

            gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                             GL_STREAM_DRAW);
            gl()->vertexAttribPointer(m_texShaderProgramRoundedClipTexPos, 2,
                                      GL_FLOAT, false, 0, 0);
            gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
            bindTexPos(m_texShaderProgramRoundedClipTexPos);
            bindTexIdx(m_texShaderProgramRoundedClipTexIdx, false);
        } else {
            if (m_lastProgram != m_texShaderProgramRoundedClip) {
                m_lastProgram = m_texShaderProgramRoundedClip;
                gl()->useProgram(m_texShaderProgramRoundedClip);

                bindTexPos(m_texShaderProgramRoundedClipTexPos);
                bindTexIdx(m_texShaderProgramRoundedClipTexIdx, true);
            }
        }

        return m_texShaderProgramRoundedClip;
    }

    // Same analytic rounded-clip SDF, for GL_TEXTURE_EXTERNAL_OES (video via
    // EGLImage). Identical math; only the sampler type differs.
    GLuint texShaderProgramRoundedClipEGLImageExternal()
    {
        if (!m_texShaderProgramRoundedClipEGLImageExternal) {
#define RRCLIP_EGL_SAMPLER_PREAMBLE                    \
    "#extension GL_OES_EGL_image_external : require\n" \
    "uniform samplerExternalOES uTexture;\n"
            const GLchar* fragmentSource =
                RRCLIP_FRAG_BODY(RRCLIP_EGL_SAMPLER_PREAMBLE,
                                 "  gl_FragColor = texData * coverage;\n");
            if (g_needsRGBShuffle) {
                fragmentSource = RRCLIP_FRAG_BODY(
                    RRCLIP_EGL_SAMPLER_PREAMBLE,
                    "  gl_FragColor.r = texData[2] * coverage;\n"
                    "  gl_FragColor.g = texData[1] * coverage;\n"
                    "  gl_FragColor.b = texData[0] * coverage;\n"
                    "  gl_FragColor.a = texData[3] * coverage;\n");
            }
#undef RRCLIP_EGL_SAMPLER_PREAMBLE

            m_texFragmentShaderRoundedClipEGLImageExternal =
                loadShader(gl(), GL_FRAGMENT_SHADER, fragmentSource);
            checkError(gl());

            m_texShaderProgramRoundedClipEGLImageExternal =
                gl()->createProgram();
            gl()->attachShader(m_texShaderProgramRoundedClipEGLImageExternal,
                               texVertexShaderRoundedClip());
            gl()->attachShader(m_texShaderProgramRoundedClipEGLImageExternal,
                               m_texFragmentShaderRoundedClipEGLImageExternal);
            gl()->linkProgram(m_texShaderProgramRoundedClipEGLImageExternal);
            checkError(gl());

            m_lastProgram = m_texShaderProgramRoundedClipEGLImageExternal;
            gl()->useProgram(m_texShaderProgramRoundedClipEGLImageExternal);
            checkError(gl());

            GLuint p = m_texShaderProgramRoundedClipEGLImageExternal;
            m_texShaderProgramRoundedClipEGLImageExternalPosition =
                gl()->getUniformLocation(p, "uPosition");
            m_texShaderProgramRoundedClipEGLImageExternalClipPos =
                gl()->getUniformLocation(p, "uClipPos");
            m_texShaderProgramRoundedClipEGLImageExternalTexPos =
                gl()->getAttribLocation(p, "aTexPos");
            m_texShaderProgramRoundedClipEGLImageExternalTexIdx =
                gl()->getAttribLocation(p, "aTexIdx");
            m_texShaderProgramRoundedClipEGLImageExternalTexture =
                gl()->getUniformLocation(p, "uTexture");
            m_texShaderProgramRoundedClipEGLImageExternalAlpha =
                gl()->getUniformLocation(p, "uAlpha");
            m_texShaderProgramRoundedClipEGLImageExternalRef =
                gl()->getUniformLocation(p, "uClipRef");
            m_texShaderProgramRoundedClipEGLImageExternalOffset =
                gl()->getUniformLocation(p, "uClipOffset");
            m_texShaderProgramRoundedClipEGLImageExternalHalf =
                gl()->getUniformLocation(p, "uClipHalf");
            m_texShaderProgramRoundedClipEGLImageExternalRadius =
                gl()->getUniformLocation(p, "uClipRadius");
            m_texShaderProgramRoundedClipEGLImageExternalCount =
                gl()->getUniformLocation(p, "uClipCount");

            gl()->uniform1i(
                m_texShaderProgramRoundedClipEGLImageExternalTexture, 0);
            gl()->uniform1f(m_texShaderProgramRoundedClipEGLImageExternalAlpha,
                            1);

            gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                             GL_STREAM_DRAW);
            gl()->vertexAttribPointer(
                m_texShaderProgramRoundedClipEGLImageExternalTexPos, 2,
                GL_FLOAT, false, 0, 0);
            gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
            bindTexPos(m_texShaderProgramRoundedClipEGLImageExternalTexPos);
            bindTexIdx(m_texShaderProgramRoundedClipEGLImageExternalTexIdx,
                       false);
        } else {
            if (m_lastProgram !=
                m_texShaderProgramRoundedClipEGLImageExternal) {
                m_lastProgram = m_texShaderProgramRoundedClipEGLImageExternal;
                gl()->useProgram(m_texShaderProgramRoundedClipEGLImageExternal);

                bindTexPos(m_texShaderProgramRoundedClipEGLImageExternalTexPos);
                bindTexIdx(m_texShaderProgramRoundedClipEGLImageExternalTexIdx,
                           true);
            }
        }

        return m_texShaderProgramRoundedClipEGLImageExternal;
    }
#undef RRCLIP_FRAG_BODY

    // Shader program with mask support
    GLuint texShaderProgramWithMask()
    {
        if (!m_texShaderProgramWithMask) {
            const GLchar* texFragmentSourceWithMask =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform sampler2D uTexture;\n"
                "uniform sampler2D uMaskTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "uniform vec4 uMaskUV;\n"
                "void main(void)\n"
                "{\n"
                "  vec4 texColor = texture2D(uTexture, vTexPos);\n"
                "  vec2 maskCoord = vec2(vTexPos.x * uMaskUV.z + uMaskUV.x, "
                "1.0 - (vTexPos.y * uMaskUV.w + uMaskUV.y));\n"
                "  float maskAlpha = texture2D(uMaskTexture, maskCoord).a;\n"
                "  gl_FragColor = texColor * uAlpha * maskAlpha;\n"
                "}";
            if (g_needsRGBShuffle) {
                texFragmentSourceWithMask =
                    "#ifdef GL_ES\n"
                    "  precision mediump float;\n"
                    "#endif\n"
                    "uniform sampler2D uTexture;\n"
                    "uniform sampler2D uMaskTexture;\n"
                    "varying vec2 vTexPos;\n"
                    "uniform float uAlpha;\n"
                    "uniform vec4 uMaskUV;\n"
                    "void main(void)\n"
                    "{\n"
                    "  vec4 texData = texture2D(uTexture, vTexPos);\n"
                    "  vec2 maskCoord = vec2(vTexPos.x * uMaskUV.z + "
                    "uMaskUV.x, 1.0 - (vTexPos.y * uMaskUV.w + uMaskUV.y));\n"
                    "  float maskAlpha = texture2D(uMaskTexture, "
                    "maskCoord).a;\n"
                    "  texData = texData * uAlpha * maskAlpha;\n"
                    "  gl_FragColor.r = texData[2];\n"
                    "  gl_FragColor.g = texData[1];\n"
                    "  gl_FragColor.b = texData[0];\n"
                    "  gl_FragColor.a = texData[3];\n"
                    "}";
            }

            m_texFragmentShaderWithMask =
                loadShader(gl(), GL_FRAGMENT_SHADER, texFragmentSourceWithMask);
            checkError(gl());

            m_texShaderProgramWithMask = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_texShaderProgramWithMask, texVertexShader());
            checkError(gl());
            gl()->attachShader(m_texShaderProgramWithMask,
                               m_texFragmentShaderWithMask);
            checkError(gl());

            gl()->linkProgram(m_texShaderProgramWithMask);
            checkError(gl());

            m_lastProgram = m_texShaderProgramWithMask;
            gl()->useProgram(m_texShaderProgramWithMask);
            checkError(gl());

            m_texShaderProgramWithMaskPosition = gl()->getUniformLocation(
                m_texShaderProgramWithMask, "uPosition");
            m_texShaderProgramWithMaskTexPos =
                gl()->getAttribLocation(m_texShaderProgramWithMask, "aTexPos");
            m_texShaderProgramWithMaskTexIdx =
                gl()->getAttribLocation(m_texShaderProgramWithMask, "aTexIdx");
            m_texShaderProgramWithMaskTexture = gl()->getUniformLocation(
                m_texShaderProgramWithMask, "uTexture");
            m_texShaderProgramWithMaskAlpha =
                gl()->getUniformLocation(m_texShaderProgramWithMask, "uAlpha");
            m_texShaderProgramWithMaskMaskTexture = gl()->getUniformLocation(
                m_texShaderProgramWithMask, "uMaskTexture");
            m_texShaderProgramWithMaskMaskUV =
                gl()->getUniformLocation(m_texShaderProgramWithMask, "uMaskUV");

            gl()->uniform1i(m_texShaderProgramWithMaskTexture, 0);
            gl()->uniform1i(m_texShaderProgramWithMaskMaskTexture, 1);
            gl()->uniform1f(m_texShaderProgramWithMaskAlpha, 1);
            gl()->uniform4f(m_texShaderProgramWithMaskMaskUV, 0, 0, 1, 1);

            gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                             GL_STREAM_DRAW);
            gl()->vertexAttribPointer(m_texShaderProgramWithMaskTexPos, 2,
                                      GL_FLOAT, false, 0, 0);
            gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
            bindTexPos(m_texShaderProgramWithMaskTexPos);
            bindTexIdx(m_texShaderProgramWithMaskTexIdx, false);
        } else {
            if (m_lastProgram != m_texShaderProgramWithMask) {
                m_lastProgram = m_texShaderProgramWithMask;
                gl()->useProgram(m_texShaderProgramWithMask);

                bindTexPos(m_texShaderProgramWithMaskTexPos);
                bindTexIdx(m_texShaderProgramWithMaskTexIdx, true);
            }
        }

        return m_texShaderProgramWithMask;
    }

// I take blur shader source from WebKit
// https://github.com/WebKit/webkit/blob/master/Source/WebCore/platform/graphics/texmap/TextureMapperShaderProgram.cpp(6f9b511a115311b13c06eb58038ddc2c78da5531)
#define GAUSSIAN_KERNEL_HALF_WIDTH 11
#define GAUSSIAN_KERNEL_STEP 0.2

    static inline float gauss(float x)
    {
        return exp(-(x * x) / 2.);
    }

    static std::vector<float> computeGaussianKernel()
    {
        std::vector<float> kernel;
        kernel.resize(GAUSSIAN_KERNEL_HALF_WIDTH);

        kernel[0] = gauss(0);
        float sum = kernel[0];
        for (unsigned i = 1; i < GAUSSIAN_KERNEL_HALF_WIDTH; ++i) {
            kernel[i] = gauss(i * GAUSSIAN_KERNEL_STEP);
            sum += 2 * kernel[i];
        }

        // Normalize the kernel.
        float scale = 1 / sum;
        for (unsigned i = 0; i < GAUSSIAN_KERNEL_HALF_WIDTH; ++i)
            kernel[i] *= scale;

        return kernel;
    }

    static std::string generateBlurEffectFragmentShader(
        bool isEGLImage, bool addColorAlign = false, bool withMask = false)
    {
        // Don't support when needsRGBShuffle is true.
        STARFISH_ASSERT(!g_needsRGBShuffle);

        std::vector<float> gaussianKernel = computeGaussianKernel();
        std::stringstream ss;

        if (isEGLImage) {
            ss << "#extension GL_OES_EGL_image_external : require\n";
        }
        ss << "#ifdef GL_ES\n";
        ss << "  precision mediump float;\n";
        ss << "#endif\n";
        if (isEGLImage) {
            ss << "uniform samplerExternalOES uTexture;\n";
        } else {
            ss << "uniform sampler2D uTexture;\n";
        }

        ss << "uniform float uTextureWidth;\n";
        ss << "uniform float uTextureHeight;\n";
        if (addColorAlign) {
            ss << "uniform float uAlpha;\n";
        }
        if (withMask) {
            ss << "uniform sampler2D uMaskTexture;\n";
            ss << "uniform vec4 uMaskUV;\n";
        }
        ss << "uniform vec2 uBlurRadius;\n";
        ss << "varying vec2 vTexPos;\n";
        ss << "vec4 sampleColorAtRadius(float radius, vec2 texCoord, float sx, "
              "float sy) {\n";
        ss << "  vec2 coord = texCoord + vec2(radius * sx, radius * sy) * "
              "uBlurRadius;\n";
        ss << "  return texture2D(uTexture, coord);\n";
        ss << "}\n";
        ss << "void main(void) {\n";
        ss << "  float sy = 1.0;\n";
        ss << "  sy /= uTextureHeight;\n";
        ss << "  float sx = 1.0;\n";
        ss << "  sx /= uTextureWidth;\n";

        ss << "  vec4 total = sampleColorAtRadius(0., vTexPos, sx, sy) * "
           << gaussianKernel[0] << ";\n";
        for (int i = 1; i < GAUSSIAN_KERNEL_HALF_WIDTH; i++) {
            ss << "  total += sampleColorAtRadius(float("
               << i * GAUSSIAN_KERNEL_STEP << "), vTexPos, sx, sy) * "
               << gaussianKernel[i] << ";\n";
            ss << "  total += sampleColorAtRadius(float("
               << -i * GAUSSIAN_KERNEL_STEP << "), vTexPos, sx, sy) * "
               << gaussianKernel[i] << ";\n";
        }

        if (withMask) {
            ss << "  vec2 maskCoord = vec2(vTexPos.x * uMaskUV.z + uMaskUV.x, "
                  "1.0 - (vTexPos.y * uMaskUV.w + uMaskUV.y));\n";
            ss << "  float maskAlpha = 0.0;\n";
            ss << "  if (maskCoord.x >= 0.0 && maskCoord.x <= 1.0 && "
                  "maskCoord.y "
                  ">= 0.0 && maskCoord.y <= 1.0) {\n";
            ss << "    maskAlpha = texture2D(uMaskTexture, maskCoord).a;\n";
            ss << "  }\n";
            if (addColorAlign) {
                ss << "  gl_FragColor = total * uAlpha * maskAlpha;\n";
            } else {
                ss << "  gl_FragColor = total * maskAlpha;\n";
            }
        } else {
            if (addColorAlign) {
                ss << "  gl_FragColor = total * uAlpha;\n";
            } else {
                ss << "  gl_FragColor = total;\n";
            }
        }

        ss << "}\n";
        return ss.str();
    }

    GLuint texFragmentBlurShaderW()
    {
        if (m_texFragmentBlurShaderW) {
            return m_texFragmentBlurShaderW;
        }
        m_texFragmentBlurShaderW =
            loadShader(gl(), GL_FRAGMENT_SHADER,
                       generateBlurEffectFragmentShader(false).data());
        checkError(gl());
        return m_texFragmentBlurShaderW;
    }

    GLuint texFragmentBlurShaderEGLImageExternalW()
    {
        if (m_texFragmentBlurShaderEGLImageExternalW) {
            return m_texFragmentBlurShaderEGLImageExternalW;
        }
        m_texFragmentBlurShaderEGLImageExternalW =
            loadShader(gl(), GL_FRAGMENT_SHADER,
                       generateBlurEffectFragmentShader(true).data());
        checkError(gl());
        return m_texFragmentBlurShaderEGLImageExternalW;
    }

    GLuint texFragmentBlurShaderH()
    {
        if (m_texFragmentBlurShaderH) {
            return m_texFragmentBlurShaderH;
        }
        m_texFragmentBlurShaderH =
            loadShader(gl(), GL_FRAGMENT_SHADER,
                       generateBlurEffectFragmentShader(false, true).data());
        checkError(gl());
        return m_texFragmentBlurShaderH;
    }

    GLuint texBlurShaderProgramW()
    {
        if (m_texBlurShaderProgramW) {
            if (m_lastProgram != m_texBlurShaderProgramW) {
                m_lastProgram = m_texBlurShaderProgramW;
                gl()->useProgram(m_texBlurShaderProgramW);
                bindTexPos(m_texBlurShaderProgramWTexPos);
                bindTexIdx(m_texBlurShaderProgramWTexIdx, true);
            }
            return m_texBlurShaderProgramW;
        }
        m_texBlurShaderProgramW = gl()->createProgram();

        gl()->attachShader(m_texBlurShaderProgramW, texVertexShader());
        gl()->attachShader(m_texBlurShaderProgramW, texFragmentBlurShaderW());
        gl()->linkProgram(m_texBlurShaderProgramW);
        checkError(gl());

        m_lastProgram = m_texBlurShaderProgramW;
        gl()->useProgram(m_texBlurShaderProgramW);

        m_texBlurShaderProgramWPosition =
            gl()->getUniformLocation(m_texBlurShaderProgramW, "uPosition");
        m_texBlurShaderProgramWTexPos =
            gl()->getAttribLocation(m_texBlurShaderProgramW, "aTexPos");
        m_texBlurShaderProgramWTexIdx =
            gl()->getAttribLocation(m_texBlurShaderProgramW, "aTexIdx");
        m_texBlurShaderProgramWTexture =
            gl()->getUniformLocation(m_texBlurShaderProgramW, "uTexture");
        m_texBlurShaderProgramWBlurRadius =
            gl()->getUniformLocation(m_texBlurShaderProgramW, "uBlurRadius");
        m_texBlurShaderProgramWTextureWidth =
            gl()->getUniformLocation(m_texBlurShaderProgramW, "uTextureWidth");
        m_texBlurShaderProgramWTextureHeight =
            gl()->getUniformLocation(m_texBlurShaderProgramW, "uTextureHeight");
        m_texBlurShaderProgramWAlphaMask =
            gl()->getUniformLocation(m_texBlurShaderProgramW, "uAlphaMask");

        gl()->uniform1i(m_texBlurShaderProgramWTexture, 0);
        gl()->uniform1i(m_texBlurShaderProgramWAlphaMask, 1);

        gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                         GL_STREAM_DRAW);
        gl()->vertexAttribPointer(m_texBlurShaderProgramWTexPos, 2, GL_FLOAT,
                                  false, 0, 0);
        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
        bindTexPos(m_texBlurShaderProgramWTexPos);
        bindTexIdx(m_texBlurShaderProgramWTexIdx, false);

        return m_texBlurShaderProgramW;
    }

    GLuint texBlurShaderProgramEGLImageExternalW()
    {
        if (m_texBlurShaderProgramEGLImageExternalW) {
            if (m_lastProgram != m_texBlurShaderProgramEGLImageExternalW) {
                m_lastProgram = m_texBlurShaderProgramEGLImageExternalW;
                gl()->useProgram(m_texBlurShaderProgramEGLImageExternalW);
                bindTexPos(m_texBlurShaderProgramEGLImageExternalWTexPos);
                bindTexIdx(m_texBlurShaderProgramEGLImageExternalWTexIdx, true);
            }
            return m_texBlurShaderProgramEGLImageExternalW;
        }
        m_texBlurShaderProgramEGLImageExternalW = gl()->createProgram();

        gl()->attachShader(m_texBlurShaderProgramEGLImageExternalW,
                           texVertexShader());
        gl()->attachShader(m_texBlurShaderProgramEGLImageExternalW,
                           texFragmentBlurShaderEGLImageExternalW());
        gl()->linkProgram(m_texBlurShaderProgramEGLImageExternalW);
        checkError(gl());

        m_lastProgram = m_texBlurShaderProgramEGLImageExternalW;
        gl()->useProgram(m_texBlurShaderProgramEGLImageExternalW);

        m_texBlurShaderProgramEGLImageExternalWPosition =
            gl()->getUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                     "uPosition");
        m_texBlurShaderProgramEGLImageExternalWTexPos = gl()->getAttribLocation(
            m_texBlurShaderProgramEGLImageExternalW, "aTexPos");
        m_texBlurShaderProgramEGLImageExternalWTexIdx = gl()->getAttribLocation(
            m_texBlurShaderProgramEGLImageExternalW, "aTexIdx");
        m_texBlurShaderProgramEGLImageExternalWTexture =
            gl()->getUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                     "uTexture");
        m_texBlurShaderProgramEGLImageExternalWBlurRadius =
            gl()->getUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                     "uBlurRadius");
        m_texBlurShaderProgramEGLImageExternalWTextureWidth =
            gl()->getUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                     "uTextureWidth");
        m_texBlurShaderProgramEGLImageExternalWTextureHeight =
            gl()->getUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                     "uTextureHeight");
        m_texBlurShaderProgramEGLImageExternalWAlphaMask =
            gl()->getUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                     "uAlphaMask");

        gl()->uniform1i(m_texBlurShaderProgramEGLImageExternalWTexture, 0);
        gl()->uniform1i(m_texBlurShaderProgramEGLImageExternalWAlphaMask, 1);

        gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                         GL_STREAM_DRAW);
        gl()->vertexAttribPointer(m_texBlurShaderProgramEGLImageExternalWTexPos,
                                  2, GL_FLOAT, false, 0, 0);
        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
        bindTexPos(m_texBlurShaderProgramEGLImageExternalWTexPos);
        bindTexIdx(m_texBlurShaderProgramEGLImageExternalWTexIdx, false);
        return m_texBlurShaderProgramEGLImageExternalW;
    }

    GLuint texBlurShaderProgramH()
    {
        if (m_texBlurShaderProgramH) {
            if (m_lastProgram != m_texBlurShaderProgramH) {
                m_lastProgram = m_texBlurShaderProgramH;
                gl()->useProgram(m_texBlurShaderProgramH);
                bindTexPos(m_texBlurShaderProgramHTexPos);
                bindTexIdx(m_texBlurShaderProgramHTexIdx, true);
            }
            return m_texBlurShaderProgramH;
        }
        m_texBlurShaderProgramH = gl()->createProgram();

        gl()->attachShader(m_texBlurShaderProgramH, texVertexShader());
        gl()->attachShader(m_texBlurShaderProgramH, texFragmentBlurShaderH());
        gl()->linkProgram(m_texBlurShaderProgramH);
        checkError(gl());

        m_lastProgram = m_texBlurShaderProgramH;
        gl()->useProgram(m_texBlurShaderProgramH);

        m_texBlurShaderProgramHPosition =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uPosition");
        m_texBlurShaderProgramHTexPos =
            gl()->getAttribLocation(m_texBlurShaderProgramH, "aTexPos");
        m_texBlurShaderProgramHTexIdx =
            gl()->getAttribLocation(m_texBlurShaderProgramH, "aTexIdx");
        m_texBlurShaderProgramHTexture =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uTexture");
        m_texBlurShaderProgramHBlurRadius =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uBlurRadius");
        m_texBlurShaderProgramHTextureWidth =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uTextureWidth");
        m_texBlurShaderProgramHTextureHeight =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uTextureHeight");
        m_texBlurShaderProgramHAlpha =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uAlpha");

        gl()->uniform1i(m_texBlurShaderProgramHTexture, 0);
        gl()->uniform1f(m_texBlurShaderProgramHAlpha, 1);

        gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                         GL_STREAM_DRAW);
        gl()->vertexAttribPointer(m_texBlurShaderProgramHTexPos, 2, GL_FLOAT,
                                  false, 0, 0);
        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
        bindTexPos(m_texBlurShaderProgramHTexPos);
        bindTexIdx(m_texBlurShaderProgramHTexIdx, false);
        return m_texBlurShaderProgramH;
    }

    GLuint texFragmentBlurShaderHWithMask()
    {
        if (m_texFragmentBlurShaderHWithMask) {
            return m_texFragmentBlurShaderHWithMask;
        }
        m_texFragmentBlurShaderHWithMask = loadShader(
            gl(), GL_FRAGMENT_SHADER,
            generateBlurEffectFragmentShader(false, true, true).data());
        checkError(gl());
        return m_texFragmentBlurShaderHWithMask;
    }

    GLuint texBlurShaderProgramHWithMask()
    {
        if (m_texBlurShaderProgramHWithMask) {
            if (m_lastProgram != m_texBlurShaderProgramHWithMask) {
                m_lastProgram = m_texBlurShaderProgramHWithMask;
                gl()->useProgram(m_texBlurShaderProgramHWithMask);
                bindTexPos(m_texBlurShaderProgramHWithMaskTexPos);
                bindTexIdx(m_texBlurShaderProgramHWithMaskTexIdx, true);
            }
            return m_texBlurShaderProgramHWithMask;
        }
        m_texBlurShaderProgramHWithMask = gl()->createProgram();

        gl()->attachShader(m_texBlurShaderProgramHWithMask, texVertexShader());
        gl()->attachShader(m_texBlurShaderProgramHWithMask,
                           texFragmentBlurShaderHWithMask());
        gl()->linkProgram(m_texBlurShaderProgramHWithMask);
        checkError(gl());

        m_lastProgram = m_texBlurShaderProgramHWithMask;
        gl()->useProgram(m_texBlurShaderProgramHWithMask);

        m_texBlurShaderProgramHWithMaskPosition = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uPosition");
        m_texBlurShaderProgramHWithMaskTexPos =
            gl()->getAttribLocation(m_texBlurShaderProgramHWithMask, "aTexPos");
        m_texBlurShaderProgramHWithMaskTexIdx =
            gl()->getAttribLocation(m_texBlurShaderProgramHWithMask, "aTexIdx");
        m_texBlurShaderProgramHWithMaskTexture = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uTexture");
        m_texBlurShaderProgramHWithMaskBlurRadius = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uBlurRadius");
        m_texBlurShaderProgramHWithMaskTextureWidth = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uTextureWidth");
        m_texBlurShaderProgramHWithMaskTextureHeight = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uTextureHeight");
        m_texBlurShaderProgramHWithMaskAlpha =
            gl()->getUniformLocation(m_texBlurShaderProgramHWithMask, "uAlpha");
        m_texBlurShaderProgramHWithMaskMaskTexture = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uMaskTexture");
        m_texBlurShaderProgramHWithMaskMaskUV = gl()->getUniformLocation(
            m_texBlurShaderProgramHWithMask, "uMaskUV");

        gl()->uniform1i(m_texBlurShaderProgramHWithMaskTexture, 0);
        gl()->uniform1i(m_texBlurShaderProgramHWithMaskMaskTexture, 1);
        gl()->uniform1f(m_texBlurShaderProgramHWithMaskAlpha, 1);
        gl()->uniform4f(m_texBlurShaderProgramHWithMaskMaskUV, 0, 0, 1, 1);

        gl()->bindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        gl()->bufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, NULL,
                         GL_STREAM_DRAW);
        gl()->vertexAttribPointer(m_texBlurShaderProgramHWithMaskTexPos, 2,
                                  GL_FLOAT, false, 0, 0);
        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
        bindTexPos(m_texBlurShaderProgramHWithMaskTexPos);
        bindTexIdx(m_texBlurShaderProgramHWithMaskTexIdx, false);
        return m_texBlurShaderProgramHWithMask;
    }
};

void CompositorFactory::destroyCompositorContextGl(Renderer* renderer,
                                                   CompositorContext* ctxInput)
{
    if (ctxInput) {
        CompositorContextGL* ctx = (CompositorContextGL*)ctxInput;
        delete ctx;
    }
}

#if defined(STARFISH_ENABLE_TEST)
typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar* message, const void* userParam);
typedef void (*PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC callback,
                                              const void* userParam);

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                   GLsizei length, const GLchar* message, const void* userParam)
{
    STARFISH_LOG_ERROR("GL Debug: %s", message);
}

void setupDebugCallback(GL* gl)
{
    PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback =
        (PFNGLDEBUGMESSAGECALLBACKPROC)dlsym(RTLD_DEFAULT,
                                             "glDebugMessageCallback");

    if (glDebugMessageCallback) {
#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT 0x92E0
#endif
#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#endif
        gl->enable(GL_DEBUG_OUTPUT);
        gl->enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugCallback, NULL);
    } else {
        STARFISH_LOG_ERROR("glDebugMessageCallback not supported");
    }
}
#endif

CompositorContext* CompositorFactory::initCompositorContextGl(
    Renderer* renderer)
{
    CompositorContextGL* compositorContext = new CompositorContextGL(renderer);
    GL* gl = renderer->gl();

    if (g_needsCheckCompatibility) {
        GLint siz;
        gl->getIntegerv(GL_MAX_TEXTURE_SIZE, &siz);
        checkError(gl);
        g_maxTextureSize = siz;

        STARFISH_RELEASE_ASSERT(CanvasSurface::g_canvasSurfaceTileSize <=
                                g_maxTextureSize);
        STARFISH_RELEASE_ASSERT(MIN_MAX_TEXTURE_SIZE <= siz);

#if defined(STARFISH_ENABLE_TEST)
        setupDebugCallback(gl);
#endif

        bool isOpenGLES3 = true;
        int major;
        gl->getIntegerv(GL_MAJOR_VERSION, &major);
        if (gl->getError()) {
            isOpenGLES3 = false;
            major = 2;
        }

        if (isOpenGLES3) {
            STARFISH_LOG_INFO("GL_MAJOR_VERSION %d", (int)major);
        } else {
            STARFISH_LOG_INFO("GL_MAJOR_VERSION 2");
        }

        if (major >= 3) {
            g_isOpenGLES3 = true;
        }

        const char* ex = (const char*)gl->getString(GL_EXTENSIONS);

        if (ex) {
            // STARFISH_LOG_INFO("GL_EXTENSIONS -> %s", ex);
            g_isSupportExtensionEGLImageExternal =
                strstr(ex, "GL_OES_EGL_image_external") != nullptr;
            g_isSupportBGRATexture =
                strstr(ex, "GL_EXT_texture_format_BGRA8888") != nullptr;
            g_isSupportTextureSwizzle =
                strstr(ex, "GL_ARB_texture_swizzle") != nullptr;
        } else {
            STARFISH_LOG_INFO("GL_EXTENSIONS -> returns null...");
        }

#if (!defined(STARFISH_TIZEN) && !defined(STARFISH_ANDROID)) || \
    (defined(STARFISH_ANDROID) && !defined(USE_EGLIMAGE_EXT_ANDROID))
        g_isSupportExtensionEGLImageExternal = false;
#endif

#if defined(STARFISH_TIZEN)
        if (isOpenGLES3) {
            g_shouldUseEGLImageOnPlainSurface = false;
        }
#endif

        // if efl enabled, there is no way to support egl image with evasgl
#if defined(STARFISH_SHELL_EFL) && defined(STARFISH_GLIB_CAIRO_GL)
        g_shouldUseEGLImageOnPlainSurface = false;
#endif

        if (g_isSupportExtensionEGLImageExternal) {
            STARFISH_LOG_INFO("support EGLImageExternal!");
        }

        if (g_isSupportBGRATexture) {
            STARFISH_LOG_INFO("support BGRA texture!");
        }

        if (g_isSupportTextureSwizzle) {
            STARFISH_LOG_INFO("support Texture Swizzle!");
            g_isSupportBGRATexture = false;
        }

#if defined(PORT_PIXEL_ORDER_BGRA)
        if (g_isSupportTextureSwizzle || g_isSupportBGRATexture) {
            g_needsRGBShuffle = false;
        }
#endif
        const char* nativeSurfaceExtensionStr;
        if (gl->isGeneric()) {
            nativeSurfaceExtensionStr = "EGL_TIZEN_image_native_surface";
        } else {
            nativeSurfaceExtensionStr = "EVAS_GL_TIZEN_image_native_surface";
        }
        g_isSupported_EGL_NATIVE_SURFACE_TIZEN =
            renderer->isSupportedExtension(nativeSurfaceExtensionStr);
        g_needsCheckCompatibility = false;
        checkError(gl);
    }

    gl->enable(GL_BLEND);
    gl->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl->activeTexture(GL_TEXTURE0);

    gl->genBuffers(1, &compositorContext->m_texTexPosBuffer);
    gl->genBuffers(1, &compositorContext->m_texIdxBuffer);
    gl->genBuffers(1, &compositorContext->m_polygonPosBuffer);

#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
    gl->genFramebuffers(1, &compositorContext->m_mainViewFBO);
    gl->genRenderbuffers(1, &compositorContext->m_mainViewRBO);
#endif

    return compositorContext;
}

uint32_t CompositorFactory::maximumTextureSizeGl()
{
    return g_maxTextureSize;
}

class CanvasSurfaceGL : public CanvasSurface {
public:
    void* operator new(size_t size);
    void clearNativeResources();
    // Objects allocated via GC_finalized_malloc must not be freed with
    // GC_FREE or delete. The no-op operator delete below prevents this.
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

    CanvasSurfaceGL(Renderer* renderer, size_t w, size_t h,
                    float additionalPixelRatio, CanvasSurfaceFlag flag)
        : CanvasSurface(additionalPixelRatio)
    {
        m_renderer = (Renderer*)renderer;
        m_width = w;
        m_height = h;
        m_bufferWidth = m_width = -1;
        m_bufferHeight = m_height = -1;
        m_bufferStride = 0;
        m_buffer = nullptr;
        m_lastDevicePixelRatio = -1;
        m_isEGLImageExternal = false;
        m_isEGLBufferOwner = false;
        m_flag = flag;
        m_wTextureCount = 0;
        m_hTextureCount = 0;
        m_textureTileSize = 0;
#if defined(STARFISH_TIZEN)
        m_tbmSurface = nullptr;
        m_eglImage = nullptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
        m_aHardwareBuffer = nullptr;
        m_eglImage = nullptr;
#endif

        attachNativeBuffer(w, h, flag);
        checkError(gl());
    }

    GL* gl()
    {
        return m_renderer->gl();
    }

#if defined(STARFISH_ENABLE_TEST) && defined(PORT_CANVAS_BACKEND_CAIRO)
    virtual void dump(const char* path)
    {
        STARFISH_ASSERT(m_buffer);
        auto surface = cairo_image_surface_create_for_data(
            m_buffer, CAIRO_FORMAT_ARGB32, bufferWidth(), bufferHeight(),
            bufferStride());
        cairo_surface_write_to_png(surface, path);
        cairo_surface_destroy(surface);
    }
#endif

    virtual void detachNativeBuffer() override
    {
        if (m_textureFragments.size()) {
            if (m_isEGLImageExternal && !m_isEGLBufferOwner) {
            } else {
                g_totalAllocatedCanvasSurfaceSize -=
                    m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            }

#if defined(STARFISH_ENABLE_WEBGL)
            // A GC finalizer can reach here while a WebGL API call's
            // GLContextScope is active. Preserve that context because the
            // compositor context selected below would otherwise leak back into
            // the interrupted WebGL call.
            GLContext contextToRestore = GLContextScope::getCurrentGLContext();
#endif
            bool ret = m_renderer->makeCurrent();
            if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN) || \
    (defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID))
                if (ret) {
                    gl()->xglDestroyImage(m_eglImage);
                }
                m_eglImage = nullptr;
#endif
#if defined(STARFISH_TIZEN)
                if (m_isEGLBufferOwner) {
                    LongTaskFinder t("tbm_surface_destroy", 1);
                    tbm_surface_destroy(m_tbmSurface);
                }
                m_tbmSurface = nullptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                if (m_isEGLBufferOwner) {
                    AHardwareBuffer_release(m_aHardwareBuffer);
                }
                m_aHardwareBuffer = nullptr;
#endif
            }

            if (m_isEGLImageExternal) {
            } else {
                free(m_buffer);
            }

            if (ret) {
                CompositorContextGL* ctx =
                    (CompositorContextGL*)m_renderer->compositorContext();
                for (size_t i = 0; i < m_textureFragments.size(); i++) {
                    if (m_textureFragments[i].sharedTexture) {
                        // The lifetime of externally created shared textures is
                        // managed by the module that created them, not the
                        // Compositor. For example, the FramebufferTexture of
                        // WebGL is created in `WebGLRenderingContextBaseMixIn`
                        // and destroyed in its `finalize()`. If required, make
                        // the `CanvasSurfaceTextureInfoFragment` free its
                        // resource by itself and share it via std::shared_ptr.
                        continue;
                    }
                    GLuint id = m_textureFragments[i].textureID;
                    if (id) {
                        if (ctx) {
                            ctx->putGenericTextureToCache(
                                id, m_textureFragments[i].textureWidth,
                                m_textureFragments[i].textureHeight,
                                textureFormat());
                        } else {
                            gl()->deleteTextures(1, &id);
                        }
                    }
                }
            }

            m_textureFragments.clear();

            m_buffer = nullptr;
            m_width = 0;
            m_height = 0;
            m_bufferStride = m_bufferWidth = m_width = 0;
            m_bufferHeight = m_height = 0;

            m_isEGLBufferOwner = m_isEGLImageExternal = false;
#if defined(STARFISH_ENABLE_WEBGL)
            if (contextToRestore.isValid() && !contextToRestore.setCurrent()) {
                STARFISH_LOG_WARN("Failed to restore WebGL context.");
            }
#endif
        }
    }

    bool attachNativeBuffer(size_t w, size_t h, CanvasSurfaceFlag flag) override
    {
        float devicePixelRatio =
            m_renderer->webView()->screenInfo().devicePixelRatio;
        if (m_width != w || m_height != h ||
            m_lastDevicePixelRatio != devicePixelRatio) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;
            m_lastDevicePixelRatio = devicePixelRatio;
            m_flag = flag;

            m_bufferWidth = std::max((size_t)1, (size_t)(w * devicePixelRatio));
            m_bufferHeight =
                std::max((size_t)1, (size_t)(h * devicePixelRatio));

            if (g_isSupportExtensionEGLImageExternal &&
                (g_shouldUseEGLImageOnPlainSurface ||
                 (m_flag & CanvasSurfaceFlag::PreferEGLImage)) &&
                m_bufferWidth <= g_maxTextureSize &&
                m_bufferHeight <= g_maxTextureSize) {
                m_isEGLBufferOwner = m_isEGLImageExternal = true;
#if defined(STARFISH_TIZEN)
                tbm_surface_info_s surfaceInfo;
                {
                    LongTaskFinder t("tbm_surface_create", 1);
#if defined(PORT_PIXEL_ORDER_BGRA)
                    m_tbmSurface = tbm_surface_create(
                        m_bufferWidth, m_bufferHeight, TBM_FORMAT_ARGB8888);
#else
                    m_tbmSurface = tbm_surface_create(
                        m_bufferWidth, m_bufferHeight, TBM_FORMAT_ABGR8888);
#endif
                    {
                        LongTaskFinder t("tbm_surface_create_clear", 1);
                        tbm_surface_map(m_tbmSurface, TBM_SURF_OPTION_WRITE,
                                        &surfaceInfo);
                        void* buffer = surfaceInfo.planes[0].ptr;
                        memset(buffer, 0,
                               surfaceInfo.planes[0].stride * m_bufferHeight);
                        tbm_surface_unmap(m_tbmSurface);
                    }
                }
                STARFISH_RELEASE_ASSERT(surfaceInfo.num_planes == 1);
                m_bufferStride = surfaceInfo.planes[0].stride;
                m_buffer = nullptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                AHardwareBuffer_Desc desc{
                    m_bufferWidth,
                    m_bufferHeight,
                    1,
                    AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
                    AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
                        AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                        AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                        AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
                    0,
                    0,
                    0
                };

                AHardwareBuffer_allocate(&desc, &m_aHardwareBuffer);
                STARFISH_RELEASE_ASSERT(m_aHardwareBuffer);
                AHardwareBuffer_Desc outDesc;
                AHardwareBuffer_describe(m_aHardwareBuffer, &outDesc);
                m_bufferStride = outDesc.stride * 4;
                m_buffer = nullptr;
#endif
            } else if (SurfaceCreationScope::hasDelegate()) {
                if (SurfaceCreationScope::delegate()->type() ==
                    TextureCreationDelegate::Type::FrameBuffer) {
                    m_isFrameBuffer = true;
                } else {
                    STARFISH_UNIMPLEMENTED();
                }

                m_isEGLImageExternal = false;
                m_isEGLBufferOwner = false;
                m_bufferStride = m_bufferWidth * sizeof(uint32_t);
                m_buffer = nullptr;
            } else {
                m_isEGLImageExternal = false;
                m_isEGLBufferOwner = false;
                m_bufferStride = m_bufferWidth * sizeof(uint32_t);
                m_buffer = nullptr;
            }

            g_totalAllocatedCanvasSurfaceSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);

            ensureGenerateTexture();
            return true;
        }
        return false;
    }

    void ensureGenerateTexture()
    {
        m_renderer->makeCurrent();

        STARFISH_RELEASE_ASSERT(m_textureFragments.size() == 0);

        if (m_isFrameBuffer && SurfaceCreationScope::hasDelegate()) {
            GLuint textureId = 0;

            // 1. Create a texture.
            if (!SurfaceCreationScope::delegate()->create(
                    m_bufferWidth, m_bufferHeight, textureId)) {
                STARFISH_RELEASE_ASSERT(false);
            }

            // 2. Add the texture info newly created to the fragement list.
            CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment fragment;
            fragment.textureWidth = m_bufferWidth;
            fragment.textureHeight = m_bufferHeight;
            fragment.textureID = textureId;
            fragment.srcX = 0;
            fragment.srcY = 0;
            fragment.srcWidth = 1;
            fragment.srcHeight = 1;
            fragment.sharedTexture = true;
            m_textureFragments.push_back(fragment);

            // 3. Set the dimension of the fragment list.
            m_wTextureCount = m_hTextureCount = 1;

            return;
        }

        if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN) || defined(STARFISH_ANDROID)
            CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment fragment;
#if defined(STARFISH_TIZEN)
            {
                STARFISH_RELEASE_ASSERT(m_tbmSurface);
                STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);

                if (gl()->isGeneric()) {
#if defined(STARFISH_SHELL_EFL) && defined(STARFISH_GLIB_CAIRO_GL)
                    EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                         EGL_NONE };
                    m_eglImage = gl()->xglCreateImage(
                        EGL_NATIVE_SURFACE_TIZEN, (void*)(intptr_t)m_tbmSurface,
                        attribs);
#else
                    if (g_isSupported_EGL_NATIVE_SURFACE_TIZEN) {
                        EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                             EGL_NONE };
                        m_eglImage = gl()->xglCreateImage(
                            EGL_NATIVE_SURFACE_TIZEN,
                            (void*)(intptr_t)m_tbmSurface, attribs);
                    } else {
                        EGLint attribs[EGL_ATTRIBUTE_MAX];
                        if (!prepareEglAttributeList(attribs, EGL_ATTRIBUTE_MAX,
                                                     m_tbmSurface)) {
                            return;
                        }
                        m_eglImage = gl()->xglCreateImage(EGL_LINUX_DMA_BUF_EXT,
                                                          nullptr, attribs);
                    }
#endif
                    checkError(gl());
                } else {
                    STARFISH_RELEASE_ASSERT(m_tbmSurface);
                    STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);
                    int eglImgAttr[] = { EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0 };
                    m_eglImage = gl()->xglCreateImage(
                        EVAS_GL_NATIVE_SURFACE_TIZEN,
                        (void*)(intptr_t)m_tbmSurface, eglImgAttr);
                    checkError(gl());
                }
            }
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
            {
                STARFISH_RELEASE_ASSERT(m_aHardwareBuffer);
                STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);

                EGLClientBuffer clientBuffer =
                    eglGetNativeClientBufferANDROID(m_aHardwareBuffer);
                if (UNLIKELY(!clientBuffer)) {
                    logEglError("eglGetNativeClientBufferANDROID");
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
                EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                     EGL_NONE };
                // eglCreateImageKHR will add a ref to the AHardwareBuffer
                m_eglImage = gl()->xglCreateImage(EGL_NATIVE_BUFFER_ANDROID,
                                                  clientBuffer, attribs);
                if (UNLIKELY(!m_eglImage)) {
                    logEglError("eglCreateImageKHR");
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }
#endif

#if defined(USE_EGLIMAGE_EXT_ANDROID) || !defined(STARFISH_ANDROID)
            if (nullptr == m_eglImage) {
                STARFISH_LOG_INFO("result of eglCreateImageKHR is fail");
            }
#endif
            {
                GLuint textureID;
                gl()->genTextures(1, &textureID);

                gl()->bindTexture(GL_TEXTURE_EXTERNAL_OES, textureID);
                checkError(gl());

                gl()->texParameteri(GL_TEXTURE_EXTERNAL_OES,
                                    GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                gl()->texParameteri(GL_TEXTURE_EXTERNAL_OES,
                                    GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                gl()->texParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE);
                gl()->texParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE);

                checkError(gl());
#if defined(STARFISH_TIZEN)
                gl()->xglImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES,
                                                 m_eglImage);
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                gl()->xglImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES,
                                                 m_eglImage);
#endif
                checkError(gl());

                gl()->bindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
                checkError(gl());

                fragment.textureWidth = m_bufferWidth;
                fragment.textureHeight = m_bufferHeight;
                fragment.textureID = textureID;
                fragment.srcX = 0;
                fragment.srcY = 0;
                fragment.srcWidth = 1;
                fragment.srcHeight = 1;

                m_textureFragments.push_back(fragment);
            }
#endif
            {
#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
                CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment
                    fragment;

                if (fragment.textureID == 0) {
                    GLuint textureID;
                    gl()->genTextures(1, &textureID);
                    checkError(gl());
                    fragment.textureID = static_cast<size_t>(textureID);
                }
                gl()->bindTexture(GL_TEXTURE_2D,
                                  static_cast<GLuint>(fragment.textureID));
                checkError(gl());
                gl()->texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_bufferWidth,
                                 m_bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                 m_buffer);
                checkError(gl());
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
                checkError(gl());
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
                checkError(gl());
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE);
                checkError(gl());
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE);
                checkError(gl());
                gl()->bindTexture(GL_TEXTURE_2D, 0);
                checkError(gl());

                fragment.textureWidth = m_bufferWidth;
                fragment.textureHeight = m_bufferHeight;
                fragment.srcX = 0;
                fragment.srcY = 0;
                fragment.srcWidth = 1;
                fragment.srcHeight = 1;
                m_textureFragments.push_back(fragment);
#endif
            }
            return;
        }

        m_textureTileSize = CanvasSurface::g_canvasSurfaceTileSize;
        m_wTextureCount = ceil((float)m_bufferWidth / m_textureTileSize);
        m_hTextureCount = ceil((float)m_bufferHeight / m_textureTileSize);

        if (m_flag & CanvasSurfaceFlag::PreferUnitedTexture) {
            m_wTextureCount = m_hTextureCount = 1;
        }

        size_t coveredRowsCount = 0;
        for (size_t y = 0; y < m_hTextureCount; y++) {
            size_t coveredColsCount = 0;
            for (size_t x = 0; x < m_wTextureCount; x++) {
                size_t texureDataX = coveredColsCount;
                size_t texureDataY = coveredRowsCount;
                size_t texureDataWidth = std::max(
                    (size_t)1, std::min((size_t)m_textureTileSize,
                                        m_bufferWidth - coveredColsCount));
                size_t texureDataHeight = std::max(
                    (size_t)1, std::min((size_t)m_textureTileSize,
                                        m_bufferHeight - coveredRowsCount));

                if (m_flag & CanvasSurfaceFlag::PreferUnitedTexture) {
                    texureDataWidth = m_bufferWidth;
                    texureDataHeight = m_bufferHeight;
                }

                CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment
                    fragment;
                fragment.textureID = 0;
                fragment.textureWidth = texureDataWidth;
                fragment.textureHeight = texureDataHeight;
                fragment.srcX = texureDataX / (float)m_bufferWidth;
                fragment.srcY = texureDataY / (float)m_bufferHeight;
                fragment.srcWidth = texureDataWidth / (float)m_bufferWidth;
                fragment.srcHeight = texureDataHeight / (float)m_bufferHeight;

                m_textureFragments.push_back(fragment);
                coveredColsCount += m_textureTileSize;
            }

            coveredRowsCount += m_textureTileSize;
        }
    }

    virtual MappedNativeBuffer mapBuffer(size_t bufferX, size_t bufferY,
                                         size_t bufferWidth,
                                         size_t bufferHeight) override
    {
        if (m_isEGLImageExternal) {
            if (!m_buffer) {
#if defined(STARFISH_TIZEN)
                tbm_surface_info_s surfaceInfo;
                {
                    LongTaskFinder t("tbm_surface_map", 1);
                    tbm_surface_map(m_tbmSurface, TBM_SURF_OPTION_WRITE,
                                    &surfaceInfo);
                }
                STARFISH_RELEASE_ASSERT(surfaceInfo.num_planes == 1);
                STARFISH_RELEASE_ASSERT(surfaceInfo.planes[0].stride ==
                                        m_bufferStride);
                m_buffer = surfaceInfo.planes[0].ptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                AHardwareBuffer_lock(m_aHardwareBuffer,
                                     AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                                         AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
                                     1, NULL, (void**)&m_buffer);
#endif
            }
        } else {
            if (!m_buffer) {
                m_buffer =
                    (unsigned char*)calloc(1, m_bufferStride * m_bufferHeight);
                STARFISH_RELEASE_ASSERT(m_buffer);
            }
        }

        CanvasSurface::MappedNativeBuffer b;
        b.m_bufferAddress = m_buffer;
        b.m_mappedBufferX = 0;
        b.m_mappedBufferY = 0;
        b.m_mappedBufferWidth = m_bufferWidth;
        b.m_mappedBufferHeight = m_bufferHeight;
        b.m_mappedBufferStride = m_bufferStride;
        return b;
    }

    virtual size_t width() override
    {
        return m_width;
    }

    virtual size_t height() override
    {
        return m_height;
    }

    virtual size_t bufferWidth() override
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight() override
    {
        return m_bufferHeight;
    }

    virtual size_t bufferStride() override
    {
        return m_bufferStride;
    }

    size_t wTextureCount()
    {
        return m_wTextureCount;
    }

    size_t hTextureCount()
    {
        return m_hTextureCount;
    }

    size_t textureTileSize()
    {
        return m_textureTileSize;
    }
    virtual void unmapBufferAndNotifyUpdatedRegion(size_t dirtyX, size_t dirtyY,
                                                   size_t dirtyWidth,
                                                   size_t dirtyHeight) override
    {
        STARFISH_ASSERT(m_wTextureCount != 0);
        STARFISH_ASSERT(m_hTextureCount != 0);
        STARFISH_ASSERT(m_textureTileSize != 0);
        if (m_textureFragments.size() == 0) {
            return;
        }

        if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN)
            {
                LongTaskFinder t("tbm_surface_unmap", 1);
                tbm_surface_unmap(m_tbmSurface);
            }
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
            int32_t fence = -1;
            AHardwareBuffer_unlock(m_aHardwareBuffer, &fence);
#endif
            m_buffer = nullptr;
            return;
        }

        STARFISH_RELEASE_ASSERT(m_buffer);

        if (dirtyWidth && dirtyHeight) {
            m_renderer->makeCurrent();
            size_t fragmentIndex = 0;

            Unit::Rect dRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight);

            size_t coveredRowsCount = 0;
            for (size_t y = 0; y < m_hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < m_wTextureCount; x++) {
                    GLuint textureID;

                    CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment&
                        fragment = m_textureFragments[fragmentIndex];

                    size_t textureDataX = coveredColsCount;
                    size_t textureDataY = coveredRowsCount;
                    size_t textureDataWidth = fragment.textureWidth;
                    size_t textureDataHeight = fragment.textureHeight;

                    Unit::Rect tRect(textureDataX, textureDataY,
                                     textureDataWidth, textureDataHeight);

                    if (tRect.intersects(dRect)) {
                        auto left = std::max(tRect.x(), dRect.x());
                        auto right = std::min(tRect.maxX(), dRect.maxX());
                        auto bottom = std::min(tRect.maxY(), dRect.maxY());
                        auto top = std::max(tRect.y(), dRect.y());

                        left -= textureDataX;
                        right -= textureDataX;
                        bottom -= textureDataY;
                        top -= textureDataY;

                        size_t xx = left;
                        size_t xxEnd = right;
                        size_t yy = top;
                        size_t yyEnd = bottom;

                        if (((xxEnd - xx) > 0) && ((yyEnd - yy) > 0)) {
                            LongTaskFinder t("update texture tile..", 1);

                            if (fragment.textureID == 0) {
                                CompositorContextGL* ctx =
                                    (CompositorContextGL*)
                                        m_renderer->compositorContext();
                                if (ctx) {
                                    fragment.textureID =
                                        ctx->takeGenericTextureFromCache(
                                            fragment.textureWidth,
                                            fragment.textureHeight);
                                }
                            }

                            gl()->pixelStorei(GL_UNPACK_ALIGNMENT, 1);

                            bool useImmutableTex = immutableTextureUpload();

                            bool textureJustCreated = false;
                            if (fragment.textureID == 0) {
                                textureJustCreated = true;
                                gl()->genTextures(1,
                                                  (GLuint*)&fragment.textureID);
                                gl()->bindTexture(GL_TEXTURE_2D,
                                                  fragment.textureID);
                                checkError(gl());

                                gl()->texParameteri(GL_TEXTURE_2D,
                                                    GL_TEXTURE_MIN_FILTER,
                                                    GL_LINEAR);
                                gl()->texParameteri(GL_TEXTURE_2D,
                                                    GL_TEXTURE_MAG_FILTER,
                                                    GL_LINEAR);

                                gl()->texParameteri(GL_TEXTURE_2D,
                                                    GL_TEXTURE_WRAP_S,
                                                    GL_CLAMP_TO_EDGE);
                                gl()->texParameteri(GL_TEXTURE_2D,
                                                    GL_TEXTURE_WRAP_T,
                                                    GL_CLAMP_TO_EDGE);

                                if (useImmutableTex) {
                                    // Always plain GL_RGBA8 (core, no BGRA
                                    // sized-format guessing); correct channel
                                    // order at sample time via the swizzle
                                    // below instead of relying on native BGRA
                                    // storage.
                                    gl()->texStorage2D(GL_TEXTURE_2D, 1,
                                                       GL_RGBA8,
                                                       fragment.textureWidth,
                                                       fragment.textureHeight);
                                }

#if defined(PORT_PIXEL_ORDER_BGRA)
                                // g_needsRGBShuffle means the fragment shader
                                // itself already swaps R/B when sampling
                                // (picked independently of texture storage
                                // type - see g_needsRGBShuffle's shader
                                // variants). Swizzling the texture on top of
                                // that would swap R/B twice, cancelling out
                                // back to the wrong (unfixed) order.
                                if (!g_needsRGBShuffle &&
                                    (useImmutableTex ||
                                     g_isSupportTextureSwizzle)) {
                                    // GL_TEXTURE_SWIZZLE_RGBA (the combined
                                    // 4-at-once pname) is desktop-GL only
                                    // (GL_ARB_texture_swizzle); it isn't valid
                                    // on GLES3 core, which only has the
                                    // per-channel R/G/B/A pnames (as used
                                    // elsewhere in this file, e.g. the mask
                                    // texture's alpha-from-red swizzle) - set
                                    // each individually instead.
                                    gl()->texParameteri(GL_TEXTURE_2D,
                                                        GL_TEXTURE_SWIZZLE_R,
                                                        GL_BLUE);
                                    gl()->texParameteri(GL_TEXTURE_2D,
                                                        GL_TEXTURE_SWIZZLE_G,
                                                        GL_GREEN);
                                    gl()->texParameteri(GL_TEXTURE_2D,
                                                        GL_TEXTURE_SWIZZLE_B,
                                                        GL_RED);
                                    gl()->texParameteri(GL_TEXTURE_2D,
                                                        GL_TEXTURE_SWIZZLE_A,
                                                        GL_ALPHA);
                                }
#endif
                            }

                            auto bData = m_buffer;
                            auto bStride = bufferStride();
                            // With immutable storage the texture is always
                            // plain RGBA8 (see above); the true BGRA channel
                            // order (if any) is corrected by the swizzle
                            // state, not by the upload's declared format.
                            auto kind =
                                useImmutableTex ? GL_RGBA : textureFormat();

                            // Fix (unconditional, not part of any
                            // experiment): only (re)allocate storage
                            // (texImage2D) at true first creation. A REUSED
                            // texture whose whole tile happens to be dirty
                            // must still go through texSubImage2D -- calling
                            // texImage2D again here needlessly reallocates
                            // storage every time the tile is fully repainted
                            // (e.g. every frame on a full-viewport repaint),
                            // not just once. With immutable storage
                            // (texStorage2D, already allocated above) it must
                            // NEVER be called again at all.
                            bool needsTexImageAlloc =
                                textureJustCreated && !useImmutableTex;

                            gl()->bindTexture(GL_TEXTURE_2D,
                                              fragment.textureID);
                            checkError(gl());

                            if (g_isOpenGLES3) {
                                gl()->pixelStorei(GL_UNPACK_ROW_LENGTH,
                                                  bufferWidth());
                                gl()->pixelStorei(GL_UNPACK_SKIP_PIXELS, xx);
                                gl()->pixelStorei(GL_UNPACK_SKIP_ROWS, yy);

                                auto data = bData;
                                data += textureDataY * bStride;
                                data += textureDataX * 4;
                                if (needsTexImageAlloc) {
                                    gl()->texImage2D(GL_TEXTURE_2D, 0, kind,
                                                     fragment.textureWidth,
                                                     fragment.textureHeight, 0,
                                                     kind, GL_UNSIGNED_BYTE,
                                                     data);
                                } else {
                                    gl()->texSubImage2D(GL_TEXTURE_2D, 0, xx,
                                                        yy, xxEnd - xx,
                                                        yyEnd - yy, kind,
                                                        GL_UNSIGNED_BYTE, data);
                                }

                                gl()->pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                gl()->pixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                gl()->pixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                            } else {
                                if (needsTexImageAlloc) {
                                    gl()->texImage2D(GL_TEXTURE_2D, 0, kind,
                                                     fragment.textureWidth,
                                                     fragment.textureHeight, 0,
                                                     kind, GL_UNSIGNED_BYTE,
                                                     nullptr);
                                }
                                for (; yy < yyEnd; yy++) {
                                    auto data = bData;
                                    data += ((yy + textureDataY) * bStride);
                                    data += ((textureDataX + xx) * 4);
                                    gl()->texSubImage2D(GL_TEXTURE_2D, 0, xx,
                                                        yy, xxEnd - xx, 1, kind,
                                                        GL_UNSIGNED_BYTE, data);
                                    checkError(gl());
                                }
                            }

                            gl()->pixelStorei(GL_UNPACK_ALIGNMENT, 4);
                            checkError(gl());
                        }
                    }

                    fragmentIndex++;
                    coveredColsCount += m_textureTileSize;
                }

                coveredRowsCount += m_textureTileSize;
            }
        }

        if (!(m_flag & (CanvasSurface::PreferEGLImage |
                        CanvasSurface::PreferRetainCPUBufferWhenUnmap))) {
            free(m_buffer);
            m_buffer = nullptr;
        }
    }

    virtual void attachPlatformExternalBuffer(void* buffer) override
    {
        detachNativeBuffer();

        m_isEGLBufferOwner = false;
        m_isEGLImageExternal = true;

        size_t w = 0, h = 0;
#if defined(STARFISH_TIZEN)
        m_tbmSurface = (tbm_surface_h)buffer;
        m_buffer = nullptr;
        tbm_surface_info_s surfaceInfo;
        tbm_surface_get_info(m_tbmSurface, &surfaceInfo);
        w = surfaceInfo.width;
        h = surfaceInfo.height;
        m_bufferStride = surfaceInfo.planes[0].stride;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
        m_aHardwareBuffer = (AHardwareBuffer*)buffer;
        AHardwareBuffer_Desc outDesc;
        AHardwareBuffer_describe(m_aHardwareBuffer, &outDesc);
        m_bufferStride = outDesc.stride * 4;
        w = outDesc.width;
        h = outDesc.height;
        m_buffer = nullptr;
#elif defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
        LinuxMediaPacket* packet = static_cast<LinuxMediaPacket*>(buffer);
        uint8_t* pixelData = packet->buffer();
        w = packet->width();
        h = packet->height();
        m_bufferStride = packet->stride();
        m_buffer = reinterpret_cast<unsigned char*>(pixelData);
#endif

        m_width = w;
        m_height = h;

        float devicePixelRatio =
            m_renderer->webView()->screenInfo().devicePixelRatio;

        m_bufferWidth = std::max((size_t)1, (size_t)(w * devicePixelRatio));
        m_bufferHeight = std::max((size_t)1, (size_t)(h * devicePixelRatio));

        ensureGenerateTexture();
    }

protected:
    friend class CompositorImplGL;
    Renderer* m_renderer;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    float m_lastDevicePixelRatio;
    size_t m_wTextureCount;
    size_t m_hTextureCount;
    size_t m_textureTileSize;
    GCAtomicVector<CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment>
        m_textureFragments;

    bool m_isFrameBuffer{ false };
    bool m_isEGLImageExternal;
    bool m_isEGLBufferOwner;
#if defined(STARFISH_TIZEN) && defined(STARFISH_SHELL_EFL) && \
    defined(STARFISH_GLIB_CAIRO_GL)
    tbm_surface_h m_tbmSurface;
    void* m_eglImage;
#elif defined(STARFISH_TIZEN)
    tbm_surface_h m_tbmSurface;
    EGLImageKHR m_eglImage;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
    AHardwareBuffer* m_aHardwareBuffer;
    EGLImageKHR m_eglImage;

#endif
};

static void canvasSurfaceGLClear(void* obj, void* cd)
{
    CanvasSurfaceGL* self = reinterpret_cast<CanvasSurfaceGL*>(obj);
    self->clearNativeResources();
}

void* CanvasSurfaceGL::operator new(size_t size)
{
    constexpr static GC_finalizer_closure data = { canvasSurfaceGLClear,
                                                   nullptr };
    return GC_finalized_malloc(size, &data);
}

void CanvasSurfaceGL::clearNativeResources()
{
    detachNativeBuffer();
}

CanvasSurface* CanvasSurfaceFactory::createGL(
    Renderer* renderer, size_t w, size_t h, float additionalPixelRatio,
    CanvasSurface::CanvasSurfaceFlag flag)
{
    return new CanvasSurfaceGL(renderer, w, h, additionalPixelRatio, flag);
}

class CompositorImplGL : public Compositor {
public:
    GL* gl()
    {
        return m_webView->renderer()->gl();
    }

    void applyDevicePixelRatio()
    {
        m_state.back().matrix.preScale(
            m_webView->screenInfo().devicePixelRatio * m_globalScale,
            m_webView->screenInfo().devicePixelRatio * m_globalScale);
    }

    void setViewport()
    {
        gl()->viewport(0, 0, screenWidth(), screenHeight());
    }

    size_t screenWidth()
    {
        return m_screenWidth * m_globalScale;
    }

    size_t screenHeight()
    {
        return m_screenHeight * m_globalScale;
    }

    void scissor(float x, float y, float width, float height)
    {
        float maxX = x + width;
        float maxY = y + height;

        x = floor(x);
        y = floor(y);
        maxX = ceil(maxX);
        maxY = ceil(maxY);

        if (m_screenMatrix.isIdentity()) {
            gl()->scissor(x, (float)screenHeight() - maxY, maxX - x, maxY - y);
            return;
        }
        // TODO implement cases when m_screenMatrix is not 9, 90, 180, 270
        // degree rotate transform

        if (gl()->isGeneric()) {
            mapPointsByMatrix(x, y, m_screenMatrix);
            mapPointsByMatrix(maxX, maxY, m_screenMatrix);
        }

        float newX = std::min(x, maxX);
        float newWidth = std::abs(x - maxX);
        float newY = std::min(y, maxY);
        float newHeight = std::abs(y - maxY);

        gl()->scissor(newX, (float)screenHeight() - (newY + newHeight),
                      newWidth, newHeight);
    }

    void mapPointsByMatrix(float& x, float& y, const SkMatrix& m)
    {
        SkPoint pt;
        pt = SkPoint::Make(x, y);
        m.mapPoints(&pt, 1);
        x = pt.x();
        y = pt.y();
    }

    void mapPointsToScreen(float& x, float& y)
    {
        auto& lastState = m_state.back();
        mapPointsByMatrix(x, y, lastState.matrix);
        mapPointsByMatrix(x, y, m_screenMatrix);
    }

    void mapPointsToLogicalScreen(float& x, float& y)
    {
        auto& lastState = m_state.back();
        mapPointsByMatrix(x, y, lastState.matrix);
    }

    void mapLogicalScreenPointsToScreen(float& x, float& y)
    {
        auto& lastState = m_state.back();
        mapPointsByMatrix(x, y, m_screenMatrix);
    }

    CompositorImplGL(WebView* webView, CompositorContext* compositorContext)
    {
        // LongTaskFinder t("CompositorImplGL::CompositorImplGL", 1);
        webView->renderer()->makeCurrent();

        m_seenFBOUsage = false;
        m_gotBaseFBORBOId = false;
        m_pendingClear = false;
        m_baseFBOId = 0;
        m_baseRBOId = 0;
        m_webView = webView;
        m_globalScale = m_webView->glCompositorScale();
        m_screenWidth = m_webView->renderer()->width();
        m_screenHeight = m_webView->renderer()->height();
        m_compositorContext = (CompositorContextGL*)compositorContext;
        TransformationMatrix m = m_webView->renderer()->screenMatrix();
        m_screenMatrix.setAll(m.scaleX, m.skewX, m.translateX, m.skewY,
                              m.scaleY, m.translateY, m.perspectiveX,
                              m.perspectiveY, m.perspectiveScale);
        setViewport();

        gl()->disable(GL_CULL_FACE);
        gl()->disable(GL_DEPTH_TEST);

        m_state.reserve(32);
        m_state.push_back(CompositorImplGLState());
        auto& lastState = m_state.back();
        lastState.matrixStaysInRect = true;
        lastState.matrix = SkMatrix::I();
        lastState.opacity = 1;
        lastState.blurRadius = 0;
        lastState.blendMode = BlendMode::Normal;
        lastState.clipRect = Unit::Rect(0, 0, screenWidth(), screenHeight());

        applyDevicePixelRatio();
    }

    ~CompositorImplGL()
    {
        // Commit any clear that was deferred and never overwritten (e.g. a
        // blank frame), so the screen is still cleared at pass end.
        flushPendingClear();

        restore();
        STARFISH_ASSERT(m_state.size() == 0);
        STARFISH_ASSERT(m_fboState.size() == 0);

        setViewport();

        gl()->bindTexture(GL_TEXTURE_2D, 0);
        if (g_isSupportExtensionEGLImageExternal) {
            gl()->bindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        }

        gl()->useProgram(0);
        m_compositorContext->m_lastProgram = 0;
    }

    virtual void clearColor(const Unit::Color& clr) override
    {
        // A clear targeting a bound FBO must happen now (deferral only applies
        // to the default framebuffer).
        if (m_fboState.size() != 0) {
            gl()->clearColor(clr.R(), clr.G(), clr.B(), clr.A());
            gl()->clear(GL_COLOR_BUFFER_BIT);
            return;
        }
        // Defer the clear. A later clearColor supersedes a pending one, and a
        // following full-screen Normal(SrcOver) drawRect folds into it (its
        // result is a constant color) instead of being drawn (a leading clear
        // before a full cover is not free, and skipping it has no load-op
        // penalty).
        m_pendingClear = true;
        m_pendingClearColor = clr;
    }

    void emitPendingClear()
    {
        gl()->clearColor(m_pendingClearColor.R(), m_pendingClearColor.G(),
                         m_pendingClearColor.B(), m_pendingClearColor.A());
        gl()->clear(GL_COLOR_BUFFER_BIT);
        m_pendingClear = false;
    }

    // Commit a pending clear before an op that does not provably overwrite it.
    // Only acts on the default framebuffer (m_pendingClear is screen-only); a
    // pending clear is left untouched while an FBO is bound and committed by
    // the next screen op.
    void flushPendingClear()
    {
        if (m_pendingClear && m_fboState.size() == 0) {
            emitPendingClear();
        }
    }

    // state
    virtual void save() override
    {
        m_state.push_back(m_state.back());
    }

    // pop state stack and restore state
    virtual void restore() override
    {
        m_state.pop_back();
    }

    // Lazily toggles GL_BLEND, skipping the call when already in the
    // requested state. Used to disable blending for fully opaque fills, which
    // lets the GPU skip the destination read/blend in the ROP stage - a win on
    // fill-rate bound low-end ARM GPUs.
    void setBlendEnabled(bool enabled)
    {
        if (m_compositorContext->m_blendEnabled == enabled) {
            return;
        }
        m_compositorContext->m_blendEnabled = enabled;
        if (enabled) {
            gl()->enable(GL_BLEND);
        } else {
            gl()->disable(GL_BLEND);
        }
    }

    // A fill is safe to draw with blending disabled only when its effective
    // alpha is exactly 1.0 and it uses the default SrcOver blend mode. Under
    // premultiplied SrcOver (src + dst*(1-srcAlpha)), srcAlpha == 1 makes the
    // dst term vanish, so the result is identical to blending turned off.
    bool isOpaqueFill(const Unit::Color& color, float opacity)
    {
        return m_state.back().blendMode == BlendMode::Normal &&
               opacity >= 1.0f && color.a() == 255;
    }

    void updateBlendMode()
    {
        BlendMode blendMode = m_state.back().blendMode;

        GLenum srcFactor = GL_ONE, dstFactor = GL_ONE_MINUS_SRC_ALPHA;
        GLenum equation = GL_FUNC_ADD;

        switch (blendMode) {
        case BlendMode::Normal:
            break;
        case BlendMode::Multiply:
            srcFactor = GL_DST_COLOR;
            dstFactor = GL_ZERO;
            equation = GL_FUNC_ADD;
            break;
        case BlendMode::Darken:
            srcFactor = GL_ONE;
            dstFactor = GL_ONE;
            equation = GL_MIN;
            break;
        case BlendMode::Lighten:
            srcFactor = GL_ONE;
            dstFactor = GL_ONE;
            equation = GL_MAX;
            break;
        case BlendMode::Difference:
            srcFactor = GL_ONE;
            dstFactor = GL_ONE;
            equation = GL_FUNC_SUBTRACT;
            break;
        case BlendMode::Screen:
            srcFactor = GL_ONE;
            dstFactor = GL_ONE_MINUS_SRC_ALPHA;
            equation = GL_FUNC_ADD;
            break;
        case BlendMode::ColorDodge:
        case BlendMode::Overlay:
        case BlendMode::ColorBurn:
        case BlendMode::HardLight:
        case BlendMode::SoftLight:
        case BlendMode::Exclusion:
        case BlendMode::Hue:
        case BlendMode::Color:
        case BlendMode::Luminosity:
        default:
            STARFISH_UNSUPPORTED("Unsupported BlendMode %d", (int)blendMode);
        }

        gl()->blendFunc(srcFactor, dstFactor);
        gl()->blendEquation(equation);
    }

    virtual void setBlendMode(BlendMode blendMode) override
    {
        m_state.back().blendMode = blendMode;
        updateBlendMode();
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) override
    {
        m_state.back().matrix.preScale(x, y);
    }

    virtual void rotate(double angle) override
    {
        m_state.back().matrix.preRotate(angle);

        if (!m_state.back().matrix.rectStaysRect()) {
            m_state.back().matrixStaysInRect = false;
        }
    }

    virtual void translate(double x, double y) override
    {
        m_state.back().matrix.preTranslate(x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y) override
    {
        m_state.back().matrix.preTranslate((double)x, (double)y);
    }

    virtual void beginOpacityLayer(float c, const Unit::Rect& rt) override
    {
        save();
        clip(rt);
        m_state.back().opacity *= c;
    }

    virtual void endOpacityLayer() override
    {
        restore();
    }

    virtual void clip(const Unit::Rect& rt) override
    {
        auto& lastState = m_state.back();
        // fast path
        if (lastState.matrixStaysInRect) {
            float dest[4][2];
            dest[0][0] = rt.x();
            dest[0][1] = rt.y();
            mapPointsToLogicalScreen(dest[0][0], dest[0][1]);

            dest[1][0] = rt.x();
            dest[1][1] = rt.maxY();
            mapPointsToLogicalScreen(dest[1][0], dest[1][1]);

            dest[2][0] = rt.maxX();
            dest[2][1] = rt.y();
            mapPointsToLogicalScreen(dest[2][0], dest[2][1]);

            dest[3][0] = rt.maxX();
            dest[3][1] = rt.maxY();
            mapPointsToLogicalScreen(dest[3][0], dest[3][1]);

            lastState.clipRect.intersect(toRect(dest));
            return;
        }
        Clipper2Lib::PathD path;
        SkPoint pt;
        pt = SkPoint::Make(rt.x(), rt.y());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(Clipper2Lib::PointD(pt.x(), pt.y()));

        pt = SkPoint::Make(rt.x() + rt.width(), rt.y());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(Clipper2Lib::PointD(pt.x(), pt.y()));

        pt = SkPoint::Make(rt.x() + rt.width(), rt.y() + rt.height());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(Clipper2Lib::PointD(pt.x(), pt.y()));

        pt = SkPoint::Make(rt.x(), rt.y() + rt.height());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(Clipper2Lib::PointD(pt.x(), pt.y()));
        lastState.abbreviatedClipPaths.push_back(path);
    }

    virtual Optional<Unit::Rect> currentClipRect() override
    {
        return m_state.back().clipRect;
    }

    virtual void setFillColor(const Unit::Color& clr) override
    {
        m_state.back().color = clr;
    }

    virtual void punchHole(const Unit::Rect& rt) override
    {
        save();
        setFillColor(Unit::Color(0, 0, 0, 0));
        auto& lastState = m_state.back();
        auto lastBlendMode = lastState.blendMode;
        // set random blend mode other than Normal to avoid
        // drawCall ignre check when opactiy is 0
        lastState.blendMode = BlendMode::Saturation;
        gl()->blendFunc(GL_ONE, GL_ZERO);
        drawRect(rt);
        lastState.blendMode = lastBlendMode;
        updateBlendMode();
        restore();
    }

    virtual void drawRect(const Unit::Rect& rt) override
    {
        INSTALL_PROFILE_TIMER("CompositorGL::drawRect");
        auto& lastState = m_state.back();
        auto currentColor = lastState.color;
        if (lastState.blendMode == BlendMode::Normal &&
            (currentColor.isTransparent() || lastState.opacity == 0)) {
            return;
        }
        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

        dest[0][0] = rt.x();
        dest[0][1] = rt.y();
        mapPointsToLogicalScreen(dest[0][0], dest[0][1]);

        dest[1][0] = rt.x();
        dest[1][1] = rt.maxY();
        mapPointsToLogicalScreen(dest[1][0], dest[1][1]);

        dest[2][0] = rt.maxX();
        dest[2][1] = rt.y();
        mapPointsToLogicalScreen(dest[2][0], dest[2][1]);

        dest[3][0] = rt.maxX();
        dest[3][1] = rt.maxY();
        mapPointsToLogicalScreen(dest[3][0], dest[3][1]);

        if (lastState.matrixStaysInRect &&
            lastState.abbreviatedClipPaths.size() == 0) {
            m_compositorContext->rectProgram();
            Unit::Rect drawRect = lastState.clipRect;
            drawRect.intersect(toRect(dest));

            // Lazy clear: if this rect fully covers the screen with a Normal
            // (SrcOver) fill, the whole framebuffer becomes a constant, so fold
            // the fill into the deferred clear color and skip the draw entirely
            // (opaque is just the a==1 case). Otherwise commit the clear first.
            // drawRect is already in logical screen coords and clipped to
            // clipRect, which starts as the full logical viewport
            // (0,0,screenWidth,screenHeight). The screen matrix maps that whole
            // viewport onto the whole framebuffer, so covering it in logical
            // space == covering the device framebuffer regardless of screen
            // rotation/scale — no need to map through the screen matrix.
            if (m_pendingClear && m_fboState.size() == 0) {
                const float eps = 0.5f; // absorb sub-pixel rounding
                bool fullCover =
                    drawRect.x() <= eps && drawRect.y() <= eps &&
                    drawRect.maxX() >= (float)screenWidth() - eps &&
                    drawRect.maxY() >= (float)screenHeight() - eps;
                if (fullCover && lastState.blendMode == BlendMode::Normal) {
                    // Premultiplied SrcOver (GL_ONE, GL_ONE_MINUS_SRC_ALPHA),
                    // src = opacity * color (matches the rect shader uniform):
                    //   out = opacity*color + clear*(1 - opacity*color.a)
                    // Round each fold to 8-bit to match the GPU's per-op write.
                    double op = lastState.opacity;
                    double sa = op * currentColor.A();
                    double inv = 1.0 - sa;
                    auto q = [](double v) -> unsigned char {
                        v = v * 255.0 + 0.5;
                        if (v < 0.0)
                            v = 0.0;
                        if (v > 255.0)
                            v = 255.0;
                        return (unsigned char)v;
                    };
                    m_pendingClearColor =
                        Unit::Color(q(op * currentColor.R() +
                                      m_pendingClearColor.R() * inv),
                                    q(op * currentColor.G() +
                                      m_pendingClearColor.G() * inv),
                                    q(op * currentColor.B() +
                                      m_pendingClearColor.B() * inv),
                                    q(op * currentColor.A() +
                                      m_pendingClearColor.A() * inv));
                    return; // fill absorbed into the deferred clear
                }
                emitPendingClear();
            }

            float minX = drawRect.x();
            float minY = drawRect.y();
            float maxX = drawRect.maxX();
            float maxY = drawRect.maxY();

            mapLogicalScreenPointsToScreen(minX, minY);
            mapLogicalScreenPointsToScreen(maxX, maxY);

            float hw = 2.f / screenWidth();
            float hh = -2.f / screenHeight();
            float position[8] = {
                minX * hw - 1, minY * hh + 1, minX * hw - 1, maxY * hh + 1,
                maxX * hw - 1, minY * hh + 1, maxX * hw - 1, maxY * hh + 1,
            };

            gl()->uniform2fv(m_compositorContext->m_rectShaderProgramPosition,
                             4, position);

            float a = lastState.opacity;
            gl()->uniform4f(m_compositorContext->m_rectShaderProgramColor,
                            a * currentColor.R(), a * currentColor.G(),
                            a * currentColor.B(), a * currentColor.A());

            gl()->enableVertexAttribArray(
                m_compositorContext->m_rectShaderProgramTexIdx);
            setBlendEnabled(!isOpaqueFill(currentColor, lastState.opacity));
            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
            checkError(gl());
            setBlendEnabled(true);
        } else {
            flushPendingClear(); // clipped/transformed: cannot prove full cover
            auto result = computeClippath(dest);
            if (result.size()) {
                if (lastState.matrixStaysInRect &&
                    isRectangleClipPath(result)) {
                    m_compositorContext->rectProgram();

                    auto drawRect = toRect(result[0]);
                    float minX = drawRect.x();
                    float minY = drawRect.y();
                    float maxX = drawRect.maxX();
                    float maxY = drawRect.maxY();

                    mapLogicalScreenPointsToScreen(minX, minY);
                    mapLogicalScreenPointsToScreen(maxX, maxY);

                    float hw = 2.f / screenWidth();
                    float hh = -2.f / screenHeight();
                    float position[] = {
                        minX * hw - 1, minY * hh + 1, // V1
                        minX * hw - 1, maxY * hh + 1, // V2
                        maxX * hw - 1, minY * hh + 1, // V3
                        maxX * hw - 1, maxY * hh + 1, // V4
                    };

                    gl()->uniform2fv(
                        m_compositorContext->m_rectShaderProgramPosition, 4,
                        position);

                    float a = lastState.opacity;
                    gl()->uniform4f(
                        m_compositorContext->m_rectShaderProgramColor,
                        a * currentColor.R(), a * currentColor.G(),
                        a * currentColor.B(), a * currentColor.A());

                    gl()->enableVertexAttribArray(
                        m_compositorContext->m_rectShaderProgramTexIdx);
                    setBlendEnabled(
                        !isOpaqueFill(currentColor, lastState.opacity));
                    gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    checkError(gl());
                    setBlendEnabled(true);
                } else {
                    drawTessellatedPolygon(result, currentColor,
                                           lastState.opacity, true);
                }
            }
        }
    }

    void drawTessellatedPolygon(const Clipper2Lib::PathsD& paths,
                                const Unit::Color& color, float opacity,
                                bool drawOutline,
                                const SkMatrix* customScreenMatrix = nullptr,
                                size_t customScreenWidth = 0,
                                size_t customScreenHeight = 0)
    {
        m_compositorContext->polygonProgram();
        size_t count = 0;
        for (size_t i = 0; i < paths.size(); i++) {
            count += paths[i].size();
        }

        if (sizeof(N) != sizeof(size_t) &&
            count > std::numeric_limits<N>::max()) {
            STARFISH_LOG_ERROR("Too many vertices for drawTessellatedPolygon");
            return;
        }

        std::vector<N> indices = mapbox::earcut<N>(paths);

        // Use custom parameters if provided, otherwise use defaults
        const SkMatrix& screenMatrix =
            customScreenMatrix ? *customScreenMatrix : m_screenMatrix;
        size_t sw = customScreenWidth ? customScreenWidth : screenWidth();
        size_t sh = customScreenHeight ? customScreenHeight : screenHeight();

        const float hw = 2.f / sw;
        const float hh = -2.f / sh;

        // Map every path point to screen space once (shared by the fill and
        // the AA outline ring), remembering where each contour starts.
        std::vector<float> screenX(count), screenY(count);
        std::vector<size_t> pathOffset(paths.size());
        {
            size_t k = 0;
            for (size_t i = 0; i < paths.size(); i++) {
                pathOffset[i] = k;
                for (size_t j = 0; j < paths[i].size(); j++) {
                    float x = (float)paths[i][j].x;
                    float y = (float)paths[i][j].y;
                    mapPointsByMatrix(x, y, screenMatrix);
                    screenX[k] = x;
                    screenY[k] = y;
                    k++;
                }
            }
        }

        // Interleaved vertex stream: x, y, coverage. The fill (coverage 1) and
        // the AA outline ring (coverage 1 -> 0 outward) are drawn together in a
        // single pass.
        std::vector<float> verts;
        verts.reserve(indices.size() * 3 + count * 6 * 3);

        auto emit = [&](float sx, float sy, float coverage) {
            verts.push_back(sx * hw - 1.f);
            verts.push_back(sy * hh + 1.f);
            verts.push_back(coverage);
        };

        // Solid fill: coverage == 1 everywhere.
        for (size_t i = 0; i < indices.size(); i++) {
            emit(screenX[indices[i]], screenY[indices[i]], 1.f);
        }

        // Anti-aliased outline: a 1px ring extruded outward from each contour
        // edge, coverage 1 (on the boundary) -> 0 (outer edge). Extruding only
        // outward (instead of straddling the edge) avoids blending the feather
        // over the already-filled interior, which previously darkened edges
        // when opacity < 1.
        if (drawOutline) {
            const float feather = 1.0f;    // ring width in screen pixels
            const float miterLimit = 4.0f; // clamp spikes at sharp corners
            for (size_t i = 0; i < paths.size(); i++) {
                size_t len = paths[i].size();
                if (len < 3)
                    continue;
                size_t base = pathOffset[i];

                // Signed area in screen space -> winding sign, so the outward
                // normal is consistent for outer contours and holes alike.
                double area2 = 0.0;
                for (size_t v = 0; v < len; v++) {
                    size_t w = (v + 1) % len;
                    area2 += (double)screenX[base + v] * screenY[base + w] -
                             (double)screenX[base + w] * screenY[base + v];
                }
                if (area2 == 0.0)
                    continue;
                float s = area2 > 0.0 ? 1.f : -1.f;

                // Outward unit normal of each edge v -> v+1.
                std::vector<float> enx(len), eny(len);
                for (size_t v = 0; v < len; v++) {
                    size_t w = (v + 1) % len;
                    float dx = screenX[base + w] - screenX[base + v];
                    float dy = screenY[base + w] - screenY[base + v];
                    float l = sqrt(dx * dx + dy * dy);
                    if (l < 1e-4f) {
                        enx[v] = eny[v] = 0.f;
                        continue;
                    }
                    dx /= l;
                    dy /= l;
                    enx[v] = s * dy;
                    eny[v] = s * -dx;
                }

                // Per-vertex outward miter offset (joins edges without gaps).
                std::vector<float> ox(len), oy(len);
                for (size_t v = 0; v < len; v++) {
                    size_t pe = (v + len - 1) % len; // incoming edge
                    float mx = enx[pe] + enx[v];
                    float my = eny[pe] + eny[v];
                    float ml = sqrt(mx * mx + my * my);
                    if (ml < 1e-4f) {
                        // ~180deg reversal: fall back to a valid edge normal.
                        if (enx[v] != 0.f || eny[v] != 0.f) {
                            mx = enx[v];
                            my = eny[v];
                        } else {
                            mx = enx[pe];
                            my = eny[pe];
                        }
                        ml = sqrt(mx * mx + my * my);
                        if (ml < 1e-4f) {
                            ox[v] = oy[v] = 0.f;
                            continue;
                        }
                    }
                    mx /= ml;
                    my /= ml;
                    // Distance along the miter that reaches `feather` of
                    // perpendicular offset.
                    float cosHalf = mx * enx[v] + my * eny[v];
                    if (fabs(cosHalf) < 1e-3f)
                        cosHalf = cosHalf < 0.f ? -1e-3f : 1e-3f;
                    float scale = feather / cosHalf;
                    float maxScale = feather * miterLimit;
                    if (scale > maxScale)
                        scale = maxScale;
                    else if (scale < -maxScale)
                        scale = -maxScale;
                    ox[v] = mx * scale;
                    oy[v] = my * scale;
                }

                // Extrude each edge into two triangles.
                for (size_t v = 0; v < len; v++) {
                    size_t w = (v + 1) % len;
                    float ivx = screenX[base + v], ivy = screenY[base + v];
                    float iwx = screenX[base + w], iwy = screenY[base + w];
                    float ovx = ivx + ox[v], ovy = ivy + oy[v];
                    float owx = iwx + ox[w], owy = iwy + oy[w];

                    emit(ivx, ivy, 1.f);
                    emit(ovx, ovy, 0.f);
                    emit(iwx, iwy, 1.f);

                    emit(ovx, ovy, 0.f);
                    emit(owx, owy, 0.f);
                    emit(iwx, iwy, 1.f);
                }
            }
        }

        size_t vertexCount = verts.size() / 3;
        if (vertexCount == 0)
            return;

        m_compositorContext->streamArrayBuffer(
            m_compositorContext->m_polygonPosBuffer,
            m_compositorContext->m_polygonPosBufferCapacity, verts.data(),
            verts.size() * sizeof(float));
        // Data is copied into the VBO by streamArrayBuffer; release the
        // client-side staging buffer now to keep peak memory low.
        std::vector<float>().swap(verts);

        const GLsizei stride = (GLsizei)(3 * sizeof(float));
        gl()->vertexAttribPointer(
            m_compositorContext->m_polygonShaderProgramPosition, 2, GL_FLOAT,
            false, stride, 0);
        gl()->enableVertexAttribArray(
            m_compositorContext->m_polygonShaderProgramPosition);
        gl()->vertexAttribPointer(
            m_compositorContext->m_polygonShaderProgramCoverage, 1, GL_FLOAT,
            false, stride, (const void*)(2 * sizeof(float)));
        gl()->enableVertexAttribArray(
            m_compositorContext->m_polygonShaderProgramCoverage);

        gl()->uniform4f(m_compositorContext->m_polygonShaderProgramColor,
                        opacity * color.R(), opacity * color.G(),
                        opacity * color.B(), opacity * color.A());

        gl()->drawArrays(GL_TRIANGLES, 0, vertexCount);

        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);

        checkError(gl());
        gl()->disableVertexAttribArray(
            m_compositorContext->m_polygonShaderProgramPosition);
        gl()->disableVertexAttribArray(
            m_compositorContext->m_polygonShaderProgramCoverage);
    }

    virtual void drawRect(const LayoutRect& rt) override
    {
        drawRect(Unit::Rect(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    Clipper2Lib::PathsD computeClippath(float (&dest)[4][2])
    {
        auto& lastState = m_state.back();
        if (lastState.matrixStaysInRect &&
            lastState.abbreviatedClipPaths.size() == 0) {
            Unit::Rect r = toRect(dest);
            r.intersect(lastState.clipRect);
            return toPaths(r);
        }

        if (lastState.matrixStaysInRect &&
            lastState.abbreviatedClipPaths.size()) {
            bool contains = true;
            for (const auto& path : lastState.abbreviatedClipPaths) {
                if (contains) {
                    for (size_t i = 0; i < 4; i++) {
                        auto r = Clipper2Lib::PointInPolygon(
                            Clipper2Lib::PointD(dest[i][0], dest[i][1]), path);
                        if (Clipper2Lib::PointInPolygonResult::IsOutside == r) {
                            contains = false;
                            break;
                        }
                    }
                }
            }
            if (contains) {
                auto drawRect = toRect(dest);
                drawRect.intersect(lastState.clipRect);
                return toPaths(drawRect);
            }
        }

        INSTALL_PROFILE_TIMER("CompositorGL::computeClippath(complex)");
        if (!lastState.computedPathCommands) {
            auto hash =
                CompositorContextGL::hashPathCommands(lastState.pathCommands);
            for (auto it = m_compositorContext->m_clipPathCache.begin();
                 it != m_compositorContext->m_clipPathCache.end(); ++it) {
                if ((*it)->first.clipRect == lastState.clipRect &&
                    (*it)->first.pathCommandsHash == hash &&
                    (*it)->first.pathCommands == lastState.pathCommands) {
                    lastState.computedPathCommands = (*it)->second;
                    if (it != m_compositorContext->m_clipPathCache.begin()) {
                        auto entry = std::move(*it);
                        m_compositorContext->m_clipPathCache.erase(it);
                        m_compositorContext->m_clipPathCache.insert(
                            m_compositorContext->m_clipPathCache.begin(),
                            std::move(entry));
                    }
                    break;
                }
            }
            if (!lastState.computedPathCommands) {
                Clipper2Lib::PathD rectClip = toPath(lastState.clipRect);
                Clipper2Lib::PathsD computedPathCommands = { std::move(
                    rectClip) };
                for (const auto& pathCommand : lastState.pathCommands) {
                    Clipper2Lib::PathD path;
                    size_t estimatedSize = pathCommand.size();
                    for (const auto& command : pathCommand) {
                        if (command.command ==
                            CompositorImplGLState::PathCommand::Command::
                                ArcNegative) {
                            float radius = command.data[0];
                            float angle1 = command.data[1];
                            float angle2 = command.data[2];
                            float angleDiff = angle2 - angle1;
                            if (std::abs(angleDiff) >= M_PI * 2) {
                                angleDiff = -M_PI * 2;
                            } else {
                                while (angleDiff > 0.0f) {
                                    angleDiff -= M_PI * 2;
                                }
                            }

                            float scale = command.matrix.getScaleX() *
                                          command.matrix.getScaleY();
                            float arcLength =
                                std::abs(radius * angleDiff * scale);
                            size_t c =
                                std::min(static_cast<size_t>(arcLength / 2.0f),
                                         static_cast<size_t>(64));
                            c = std::max(c, static_cast<size_t>(4));
                            estimatedSize += c;
                        }
                    }
                    path.reserve(estimatedSize);

                    for (const auto& command : pathCommand) {
                        if (command.command ==
                            CompositorImplGLState::PathCommand::Command::
                                ArcNegative) {
                            float radius = command.data[0];
                            float angle1 = command.data[1];
                            float angle2 = command.data[2];
                            float angleDiff = angle2 - angle1;
                            if (std::abs(angleDiff) >= M_PI * 2) {
                                angleDiff = -M_PI * 2;
                            } else {
                                while (angleDiff > 0.0f) {
                                    angleDiff -= M_PI * 2;
                                }
                            }

                            float scale = command.matrix.getScaleX() *
                                          command.matrix.getScaleY();
                            float arcLength =
                                std::abs(radius * angleDiff * scale);
                            size_t c =
                                std::min(static_cast<size_t>(arcLength / 2.0f),
                                         static_cast<size_t>(64));
                            c = std::max(c, static_cast<size_t>(4));
                            float step = 1.0f / static_cast<float>(c);
                            for (size_t i = 0; i <= c; i++) {
                                float t = i * step;
                                float a = angle1 + angleDiff * t;
                                float dx = cos(a);
                                float dy = sin(a);
                                float x = command.x + dx * radius;
                                float y = command.y + dy * radius;
                                addToPath(path, command.matrix, x, y);
                            }
                        } else {
                            addToPath(path, command.matrix, command.x,
                                      command.y);
                        }
                    }
                    computedPathCommands = Clipper2Lib::Intersect(
                        computedPathCommands, { std::move(path) },
                        Clipper2Lib::FillRule::NonZero);
                }

                if (m_compositorContext->m_clipPathCache.size() >= 16) {
                    m_compositorContext->m_clipPathCache.pop_back();
                }
                m_compositorContext->m_clipPathCache.insert(
                    m_compositorContext->m_clipPathCache.begin(),
                    std::unique_ptr<
                        std::pair<CompositorContextGL::ClipPathCacheKey,
                                  Clipper2Lib::PathsD>>(
                        std::move(
                            new std::pair<CompositorContextGL::ClipPathCacheKey,
                                          Clipper2Lib::PathsD>(
                                std::move(std::make_pair(
                                    CompositorContextGL::ClipPathCacheKey{
                                        lastState.clipRect, hash,
                                        lastState.pathCommands },
                                    computedPathCommands))))));
                lastState.computedPathCommands =
                    std::move(computedPathCommands);
            }
        }
        // use computed cache
        Clipper2Lib::PathD subject;
        subject.reserve(4);
        subject.emplace_back(dest[0][0], dest[0][1]);
        subject.emplace_back(dest[2][0], dest[2][1]);
        subject.emplace_back(dest[3][0], dest[3][1]);
        subject.emplace_back(dest[1][0], dest[1][1]);

        return Clipper2Lib::Intersect({ std::move(subject) },
                                      lastState.computedPathCommands.value(),
                                      Clipper2Lib::FillRule::NonZero);
    }

    void drawFilteredTexture(CanvasSurfaceGL* cs, float position[8],
                             GLuint textureID, GLenum textureKind,
                             GLenum textureBindNumber, size_t textureWidth,
                             size_t textureHeight, GLuint maskTextureID,
                             float maskUV[4])
    {
        auto& lastState = m_state.back();
        bool enableMask = maskTextureID != 0;

        // Use FBO in order to 2-pass blur
        pushFBOContext(textureWidth, textureHeight,
                       LayoutRect(0, 0, textureWidth, textureHeight));

        bool isScissorEnabled = gl()->isEnabled(GL_SCISSOR_TEST);

        if (isScissorEnabled) {
            gl()->disable(GL_SCISSOR_TEST);
        }

        gl()->clearColor(0, 0, 0, 0);
        gl()->clear(GL_COLOR_BUFFER_BIT);

        bool isEGLImage = textureKind != GL_TEXTURE_2D;
        float blurMainRadius = lastState.blurRadius;
        // original code don't set sub radius but we set magic number
        // because we don't have antialias yet
        // setting sub radius reduce glitch
        float blurSubRadius = lastState.blurRadius / 5;
        if (blurSubRadius == (int)lastState.blurRadius) {
            blurSubRadius *= 0.85;
        }
        // blur W
        {
            float position[] = { -1, -1, -1, 1, 1, -1, 1, 1 };

            if (isEGLImage) {
                m_compositorContext->texBlurShaderProgramEGLImageExternalW();
            } else {
                m_compositorContext->texBlurShaderProgramW();
            }

            GLint* positionPos;
            GLint* texPos;
            GLint* texIdx;
            GLint* width;
            GLint* height;
            GLint* blurRadius;

            if (isEGLImage) {
                texPos = &m_compositorContext
                              ->m_texBlurShaderProgramEGLImageExternalWTexPos;
                texIdx = &m_compositorContext
                              ->m_texBlurShaderProgramEGLImageExternalWTexIdx;
                positionPos =
                    &m_compositorContext
                         ->m_texBlurShaderProgramEGLImageExternalWPosition;
                width =
                    &m_compositorContext
                         ->m_texBlurShaderProgramEGLImageExternalWTextureWidth;
                height =
                    &m_compositorContext
                         ->m_texBlurShaderProgramEGLImageExternalWTextureHeight;
                blurRadius =
                    &m_compositorContext
                         ->m_texBlurShaderProgramEGLImageExternalWBlurRadius;
            } else {
                texPos = &m_compositorContext->m_texBlurShaderProgramWTexPos;
                texIdx = &m_compositorContext->m_texBlurShaderProgramWTexIdx;
                positionPos =
                    &m_compositorContext->m_texBlurShaderProgramWPosition;
                width =
                    &m_compositorContext->m_texBlurShaderProgramWTextureWidth;
                height =
                    &m_compositorContext->m_texBlurShaderProgramWTextureHeight;
                blurRadius =
                    &m_compositorContext->m_texBlurShaderProgramWBlurRadius;
            }

            gl()->enableVertexAttribArray(*texPos);
            gl()->enableVertexAttribArray(*texIdx);

            gl()->uniform2fv(*positionPos, 4, position);

            gl()->uniform1f(*width, textureWidth);
            gl()->uniform1f(*height, textureHeight);
            gl()->uniform2f(*blurRadius, blurMainRadius, blurSubRadius);

            gl()->activeTexture(GL_TEXTURE0);
            gl()->bindTexture(textureKind, textureID);

            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);

            gl()->disableVertexAttribArray(*texPos);
            gl()->disableVertexAttribArray(*texIdx);
        }

        auto fboState = popFBOContext();
        checkError(gl());

        if (isScissorEnabled) {
            gl()->enable(GL_SCISSOR_TEST);
        }

        // blur H
        {
            // Select appropriate shader program based on mask requirement
            if (enableMask) {
                m_compositorContext->texBlurShaderProgramHWithMask();
            } else {
                m_compositorContext->texBlurShaderProgramH();
            }

            GLint* texPos;
            GLint* texIdx;
            GLint* positionPos;
            GLint* width;
            GLint* height;
            GLint* blurRadius;
            GLint* alphaPos;
            GLint* maskUVUniform = nullptr;

            if (enableMask) {
                texPos =
                    &m_compositorContext->m_texBlurShaderProgramHWithMaskTexPos;
                texIdx =
                    &m_compositorContext->m_texBlurShaderProgramHWithMaskTexIdx;
                positionPos = &m_compositorContext
                                   ->m_texBlurShaderProgramHWithMaskPosition;
                width = &m_compositorContext
                             ->m_texBlurShaderProgramHWithMaskTextureWidth;
                height = &m_compositorContext
                              ->m_texBlurShaderProgramHWithMaskTextureHeight;
                blurRadius = &m_compositorContext
                                  ->m_texBlurShaderProgramHWithMaskBlurRadius;
                alphaPos =
                    &m_compositorContext->m_texBlurShaderProgramHWithMaskAlpha;
                maskUVUniform =
                    &m_compositorContext->m_texBlurShaderProgramHWithMaskMaskUV;
            } else {
                texPos = &m_compositorContext->m_texBlurShaderProgramHTexPos;
                texIdx = &m_compositorContext->m_texBlurShaderProgramHTexIdx;
                positionPos =
                    &m_compositorContext->m_texBlurShaderProgramHPosition;
                width =
                    &m_compositorContext->m_texBlurShaderProgramHTextureWidth;
                height =
                    &m_compositorContext->m_texBlurShaderProgramHTextureHeight;
                blurRadius =
                    &m_compositorContext->m_texBlurShaderProgramHBlurRadius;
                alphaPos = &m_compositorContext->m_texBlurShaderProgramHAlpha;
            }

            gl()->enableVertexAttribArray(*texPos);
            gl()->enableVertexAttribArray(*texIdx);

            gl()->uniform2fv(*positionPos, 4, position);

            if (enableMask) {
                gl()->activeTexture(GL_TEXTURE1);
                gl()->bindTexture(GL_TEXTURE_2D, maskTextureID);
            }

            gl()->activeTexture(GL_TEXTURE0);
            gl()->bindTexture(GL_TEXTURE_2D, fboState.fboTex);

            gl()->uniform1f(*width, textureWidth);
            gl()->uniform1f(*height, textureHeight);

            gl()->uniform2f(*blurRadius, blurSubRadius, blurMainRadius);

            float a = lastState.opacity;
            if (a != 1) {
                gl()->uniform1f(*alphaPos, a);
            }

            if (enableMask) {
                gl()->uniform4f(*maskUVUniform, maskUV[0], maskUV[1], maskUV[2],
                                maskUV[3]);
            }

            if (UNLIKELY(cs->isFlipYNeeded())) {
                m_compositorContext->bindTexPos(*texPos, true);
            }

            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);

            gl()->disableVertexAttribArray(*texPos);
            gl()->disableVertexAttribArray(*texIdx);

            if (a != 1) {
                gl()->uniform1f(*alphaPos, 1);
            }
            if (UNLIKELY(cs->isFlipYNeeded())) {
                m_compositorContext->bindTexPos(*texPos, false);
            }
            checkError(gl());
        }
        checkError(gl());
    }

    void drawTexture(
        CanvasSurfaceGL* cs, float position[8], GLuint textureID,
        GLenum textureKind, GLenum textureBindNumber, size_t textureWidth,
        size_t textureHeight, GLuint maskTextureID, float maskUV[4],
        Optional<const float*> roundedClipPos = nullptr,
        const CompositorImplGLState::RoundedRectClip* roundedClips = nullptr,
        int roundedClipCount = 0)
    {
        auto& lastState = m_state.back();
        if (lastState.blurRadius) {
            drawFilteredTexture(cs, position, textureID, textureKind,
                                textureBindNumber, textureWidth, textureHeight,
                                maskTextureID, maskUV);
            return;
        }
        bool isEGLImage = textureKind != GL_TEXTURE_2D;
        bool enableMask = maskTextureID != 0;

        // Analytic rounded-rect clip: clip the quad with per-fragment SDFs
        // instead of a mask texture. Works for both GL_TEXTURE_2D and
        // EGLImageExternal (video) textures via two otherwise-identical
        // shader programs (texShaderProgramRoundedClip[EGLImageExternal]).
        if (roundedClipPos && roundedClips && roundedClipCount > 0) {
            auto* cc = m_compositorContext;
            struct {
                GLint texPos, texIdx, position, clipPos, alpha, ref, offset,
                    half, radius, count;
            } loc;
            if (isEGLImage) {
                cc->texShaderProgramRoundedClipEGLImageExternal();
                loc = {
                    cc->m_texShaderProgramRoundedClipEGLImageExternalTexPos,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalTexIdx,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalPosition,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalClipPos,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalAlpha,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalRef,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalOffset,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalHalf,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalRadius,
                    cc->m_texShaderProgramRoundedClipEGLImageExternalCount
                };
            } else {
                cc->texShaderProgramRoundedClip();
                loc = { cc->m_texShaderProgramRoundedClipTexPos,
                        cc->m_texShaderProgramRoundedClipTexIdx,
                        cc->m_texShaderProgramRoundedClipPosition,
                        cc->m_texShaderProgramRoundedClipClipPos,
                        cc->m_texShaderProgramRoundedClipAlpha,
                        cc->m_texShaderProgramRoundedClipRef,
                        cc->m_texShaderProgramRoundedClipOffset,
                        cc->m_texShaderProgramRoundedClipHalf,
                        cc->m_texShaderProgramRoundedClipRadius,
                        cc->m_texShaderProgramRoundedClipCount };
            }
            gl()->activeTexture(GL_TEXTURE0);
            gl()->bindTexture(textureKind, textureID);

            gl()->enableVertexAttribArray(loc.texPos);
            gl()->enableVertexAttribArray(loc.texIdx);
            gl()->uniform2fv(loc.position, 4, position);
            gl()->uniform2fv(loc.clipPos, 4, roundedClipPos.value());

            // uClipRef = clips[0].center (subtracted in vertex stage, highp).
            // uClipOffset[i] = clips[i].center - clips[0].center (fragment).
            gl()->uniform2f(loc.ref, roundedClips[0].cx, roundedClips[0].cy);
            float offsets[CompositorImplGLState::kMaxAnalyticRoundedClips * 2];
            float halves[CompositorImplGLState::kMaxAnalyticRoundedClips * 2];
            float radii[CompositorImplGLState::kMaxAnalyticRoundedClips];
            for (int i = 0; i < roundedClipCount; i++) {
                offsets[i * 2] = roundedClips[i].cx - roundedClips[0].cx;
                offsets[i * 2 + 1] = roundedClips[i].cy - roundedClips[0].cy;
                halves[i * 2] = roundedClips[i].hx;
                halves[i * 2 + 1] = roundedClips[i].hy;
                radii[i] = roundedClips[i].radius;
            }
            gl()->uniform2fv(loc.offset, roundedClipCount, offsets);
            gl()->uniform2fv(loc.half, roundedClipCount, halves);
            gl()->uniform1fv(loc.radius, roundedClipCount, radii);
            gl()->uniform1i(loc.count, roundedClipCount);

            float a = lastState.opacity;
            if (a != 1) {
                gl()->uniform1f(loc.alpha, a);
            }
            if (UNLIKELY(cs->isFlipYNeeded())) {
                cc->bindTexPos(loc.texPos, true);
            }
            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
            checkError(gl());
            if (UNLIKELY(cs->isFlipYNeeded())) {
                cc->bindTexPos(loc.texPos, false);
            }
            if (a != 1) {
                gl()->uniform1f(loc.alpha, 1);
            }
            gl()->disableVertexAttribArray(loc.texPos);
            gl()->disableVertexAttribArray(loc.texIdx);
            return;
        }

        // Select appropriate shader program based on mask requirement
        if (isEGLImage) {
            m_webView->renderer()->mayNeedsSync();
            if (enableMask) {
                m_compositorContext->texShaderProgramEGLImageExternalWithMask();
            } else {
                m_compositorContext->texShaderProgramEGLImageExternal();
            }
        } else {
            if (enableMask) {
                m_compositorContext->texShaderProgramWithMask();
            } else {
                m_compositorContext->texShaderProgram();
            }
        }

        if (enableMask) {
            gl()->activeTexture(GL_TEXTURE1);
            gl()->bindTexture(GL_TEXTURE_2D, maskTextureID);
        }

        gl()->activeTexture(GL_TEXTURE0);
        gl()->bindTexture(textureKind, textureID);

        GLint* positionPos;
        GLint* alphaPos;
        GLint* texPos;
        GLint* texIdx;
        GLint* maskUVUniform = nullptr;

        float a = lastState.opacity;
        if (isEGLImage && enableMask) {
            positionPos =
                &m_compositorContext
                     ->m_texShaderProgramEGLImageExternalWithMaskPosition;
            alphaPos = &m_compositorContext
                            ->m_texShaderProgramEGLImageExternalWithMaskAlpha;
            texPos = &m_compositorContext
                          ->m_texShaderProgramEGLImageExternalWithMaskTexPos;
            texIdx = &m_compositorContext
                          ->m_texShaderProgramEGLImageExternalWithMaskTexIdx;
            maskUVUniform =
                &m_compositorContext
                     ->m_texShaderProgramEGLImageExternalWithMaskMaskUV;
        } else if (isEGLImage) {
            positionPos = &m_compositorContext
                               ->m_texShaderProgramEGLImageExternalPosition;
            alphaPos =
                &m_compositorContext->m_texShaderProgramEGLImageExternalAlpha;
            texPos =
                &m_compositorContext->m_texShaderProgramEGLImageExternalTexPos;
            texIdx =
                &m_compositorContext->m_texShaderProgramEGLImageExternalTexIdx;
            STARFISH_ASSERT(!enableMask);
        } else if (enableMask) {
            positionPos =
                &m_compositorContext->m_texShaderProgramWithMaskPosition;
            alphaPos = &m_compositorContext->m_texShaderProgramWithMaskAlpha;
            texPos = &m_compositorContext->m_texShaderProgramWithMaskTexPos;
            texIdx = &m_compositorContext->m_texShaderProgramWithMaskTexIdx;
            maskUVUniform =
                &m_compositorContext->m_texShaderProgramWithMaskMaskUV;
        } else {
            positionPos = &m_compositorContext->m_texShaderProgramPosition;
            alphaPos = &m_compositorContext->m_texShaderProgramAlpha;
            texPos = &m_compositorContext->m_texShaderProgramTexPos;
            texIdx = &m_compositorContext->m_texShaderProgramTexIdx;
            STARFISH_ASSERT(!enableMask);
        }

        gl()->enableVertexAttribArray(*texPos);
        gl()->enableVertexAttribArray(*texIdx);

        gl()->uniform2fv(*positionPos, 4, position);

        if (a != 1) {
            gl()->uniform1f(*alphaPos, a);
        }

        if (enableMask) {
            gl()->uniform4f(*maskUVUniform, maskUV[0], maskUV[1], maskUV[2],
                            maskUV[3]);
        }

        if (UNLIKELY(cs->isFlipYNeeded())) {
            m_compositorContext->bindTexPos(*texPos, true);
        }

        gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
        checkError(gl());

        if (UNLIKELY(cs->isFlipYNeeded())) {
            m_compositorContext->bindTexPos(*texPos, false);
        }

        if (a != 1) {
            gl()->uniform1f(*alphaPos, 1);
        }

        gl()->disableVertexAttribArray(*texPos);
        gl()->disableVertexAttribArray(*texIdx);
    }

    Unit::Rect boundingRect(const Clipper2Lib::PathD& path)
    {
        double minX = 0, minY = 0, maxX = 0, maxY = 0;

        if (path.size()) {
            minX = path[0].x;
            minY = path[0].y;
            maxX = path[0].x;
            maxY = path[0].y;
        }

        for (size_t i = 1; i < path.size(); i++) {
            minX = std::min(path[i].x, minX);
            minY = std::min(path[i].y, minY);
            maxX = std::max(path[i].x, maxX);
            maxY = std::max(path[i].y, maxY);
        }

        return Unit::Rect(minX, minY, std::abs(maxX - minX),
                          std::abs(maxY - minY));
    }

    void computeTexturePosition(const Unit::Rect& dst, const SkMatrix& ctm,
                                const SkMatrix& screenMatrix,
                                size_t screenWidth, size_t screenHeight,
                                float (&position)[8],
                                Optional<float*> clipPos = nullptr)
    {
        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)
        dest[0][0] = dst.x();
        dest[0][1] = dst.y();
        dest[1][0] = dst.x();
        dest[1][1] = dst.maxY();
        dest[2][0] = dst.maxX();
        dest[2][1] = dst.y();
        dest[3][0] = dst.maxX();
        dest[3][1] = dst.maxY();

        mapPointsByMatrix(dest[0][0], dest[0][1], ctm);
        mapPointsByMatrix(dest[1][0], dest[1][1], ctm);
        mapPointsByMatrix(dest[2][0], dest[2][1], ctm);
        mapPointsByMatrix(dest[3][0], dest[3][1], ctm);

        // Logical-screen (post-ctm, pre-screenMatrix) corners for the analytic
        // rounded-rect clip SDF - same space as state.roundedRectClips[].
        if (clipPos) {
            float* cp = clipPos.value();
            for (int i = 0; i < 4; i++) {
                cp[i * 2] = dest[i][0];
                cp[i * 2 + 1] = dest[i][1];
            }
        }

        mapPointsByMatrix(dest[0][0], dest[0][1], screenMatrix);
        mapPointsByMatrix(dest[1][0], dest[1][1], screenMatrix);
        mapPointsByMatrix(dest[2][0], dest[2][1], screenMatrix);
        mapPointsByMatrix(dest[3][0], dest[3][1], screenMatrix);

        float hw = 2.f / screenWidth;
        float hh = -2.f / screenHeight;
        position[0] = dest[0][0] * hw - 1;
        position[1] = dest[0][1] * hh + 1;

        position[2] = dest[1][0] * hw - 1;
        position[3] = dest[1][1] * hh + 1;

        position[4] = dest[2][0] * hw - 1;
        position[5] = dest[2][1] * hh + 1;

        position[6] = dest[3][0] * hw - 1;
        position[7] = dest[3][1] * hh + 1;
    }

    virtual void drawSurface(CanvasSurface* cs, const Unit::Rect& dst) override
    {
        INSTALL_PROFILE_TIMER("CompositorGL::drawSurface");

        CanvasSurfaceGL* csGL = (CanvasSurfaceGL*)cs;
        auto& textureInfo = csGL->m_textureFragments;
        if (textureInfo.size() == 0) {
            return;
        }

        auto& lastState = m_state.back();
        if (lastState.blendMode == BlendMode::Normal &&
            lastState.opacity == 0) {
            return;
        }

        // A textured blit cannot be proven to opaquely cover the screen, so
        // commit any deferred clear before it.
        flushPendingClear();

        bool scissorClippingEnabled = false;
        bool shouldSkipTexturePainting = false;
        Unit::Rect visibleArea =
            Unit::Rect(0, 0, screenWidth(), screenHeight());

        SkMatrix ctm = lastState.matrix;
        SkMatrix screenMatrix = m_screenMatrix;
        size_t screenWidth = this->screenWidth();
        size_t screenHeight = this->screenHeight();
        FBOState maskFBO;
        float maskUV[4] = { 0, 0, 1, 1 };

        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)
        dest[0][0] = dst.x();
        dest[0][1] = dst.y();
        mapPointsToLogicalScreen(dest[0][0], dest[0][1]);

        dest[1][0] = dst.x();
        dest[1][1] = dst.maxY();
        mapPointsToLogicalScreen(dest[1][0], dest[1][1]);

        dest[2][0] = dst.maxX();
        dest[2][1] = dst.y();
        mapPointsToLogicalScreen(dest[2][0], dest[2][1]);

        dest[3][0] = dst.maxX();
        dest[3][1] = dst.maxY();
        mapPointsToLogicalScreen(dest[3][0], dest[3][1]);

        // Analytic rounded-rect clip: the SDF shader clips each fragment to the
        // intersection of all tracked rounded rects; we scissor to their bbox
        // intersection, skipping the mask FBO entirely.
        bool activeClip = lastState.roundedClipChainOk &&
                          lastState.roundedRectClipCount > 0 &&
                          lastState.matrixStaysInRect;

        if (activeClip) {
            visibleArea = toRect(dest);
            for (int i = 0; i < lastState.roundedRectClipCount; i++) {
                const auto& rr = lastState.roundedRectClips[i];
                Unit::Rect rrBox(rr.cx - rr.hx, rr.cy - rr.hy, rr.hx * 2.f,
                                 rr.hy * 2.f);
                visibleArea.intersect(rrBox);
            }
            visibleArea.intersect(lastState.clipRect);
            shouldSkipTexturePainting = visibleArea.isEmpty();
            gl()->enable(GL_SCISSOR_TEST);
            scissor(visibleArea.x(), visibleArea.y(), visibleArea.width(),
                    visibleArea.height());
            scissorClippingEnabled = true;
        } else if (lastState.abbreviatedClipPaths.size() == 0 &&
                   lastState.matrixStaysInRect) {
            visibleArea = toRect(dest);
            visibleArea.intersect(lastState.clipRect);
            shouldSkipTexturePainting = visibleArea.isEmpty();
            gl()->enable(GL_SCISSOR_TEST);
            scissor(visibleArea.x(), visibleArea.y(), visibleArea.width(),
                    visibleArea.height());
            scissorClippingEnabled = true;
        } else {
            visibleArea = Unit::Rect(0, 0, 0, 0);
            auto result = computeClippath(dest);
            if (result.size()) {
                if (isRectangleClipPath(result)) {
                    visibleArea = toRect(result[0]);
                    gl()->enable(GL_SCISSOR_TEST);
                    scissor(visibleArea.x(), visibleArea.y(),
                            visibleArea.width(), visibleArea.height());
                    scissorClippingEnabled = true;
                } else {
                    float screenDest[4][2];
                    screenDest[0][0] = 0;
                    screenDest[0][1] = 0;

                    screenDest[1][0] = 0;
                    screenDest[1][1] = screenHeight;

                    screenDest[2][0] = screenWidth;
                    screenDest[2][1] = 0;

                    screenDest[3][0] = screenWidth;
                    screenDest[3][1] = screenHeight;

                    Clipper2Lib::PathsD screenResult;
                    Clipper2Lib::RectD clipBound;
                    screenResult = computeClippath(screenDest);
                    clipBound = Clipper2Lib::GetBounds(screenResult);
                    visibleArea =
                        Unit::Rect(clipBound.left, clipBound.top,
                                   clipBound.Width(), clipBound.Height());
                    Unit::Rect pixelSnappedVisibleArea = visibleArea;
                    float nx = std::floor(pixelSnappedVisibleArea.x());
                    float ny = std::floor(pixelSnappedVisibleArea.y());

                    pixelSnappedVisibleArea.setWidth(
                        std::ceil(pixelSnappedVisibleArea.width() +
                                  pixelSnappedVisibleArea.x() - nx));
                    pixelSnappedVisibleArea.setHeight(
                        std::ceil(pixelSnappedVisibleArea.height() +
                                  pixelSnappedVisibleArea.y() - ny));
                    pixelSnappedVisibleArea.setX(nx);
                    pixelSnappedVisibleArea.setY(ny);
                    if (!visibleArea.isEmpty()) {
                        // Try to get mask texture from cache first
                        GLenum maskFormat = GL_RGBA;
                        if (g_isOpenGLES3) {
                            maskFormat = GL_RED;
                        }
                        GLuint cachedMaskTexture =
                            m_compositorContext->takeMaskTextureFromCache(
                                pixelSnappedVisibleArea.width(),
                                pixelSnappedVisibleArea.height(), maskFormat,
                                screenResult, visibleArea);

                        if (cachedMaskTexture) {
                            maskFBO.fboTex = cachedMaskTexture;
                            maskFBO.textureSize =
                                Unit::IntSize(pixelSnappedVisibleArea.width(),
                                              pixelSnappedVisibleArea.height());
                            maskFBO.fboId = 0;
                            maskFBO.viewport = LayoutRect();
                            maskFBO.textureFormat = maskFormat;
                        } else {
                            // Create mask texture using FBO
                            // Draw clipping polygon with white color to create
                            // alpha mask
                            auto rw = pixelSnappedVisibleArea.width();
                            auto rh = pixelSnappedVisibleArea.height();
                            auto fboViewport = LayoutRect(
                                0, rh - pixelSnappedVisibleArea.height(),
                                pixelSnappedVisibleArea.width(),
                                pixelSnappedVisibleArea.height());
                            pushFBOContext(rw, rh, fboViewport, maskFormat);

                            gl()->clearColor(0, 0, 0, 0);
                            gl()->clear(GL_COLOR_BUFFER_BIT);

                            // Translate clip paths to FBO coordinates
                            SkMatrix fboMatrix;
                            fboMatrix.reset();
                            fboMatrix.postTranslate(-visibleArea.x(),
                                                    -visibleArea.y());
                            drawTessellatedPolygon(
                                screenResult, Unit::Color(255, 255, 255, 255),
                                1.0f, true, &fboMatrix, visibleArea.width(),
                                visibleArea.height());

                            // Get the mask texture from FBO
                            maskFBO = popFBOContext(false);

                            if (g_isOpenGLES3) {
                                // Set texture swizzle so that reading alpha
                                // channel returns red channel value
                                gl()->bindTexture(GL_TEXTURE_2D,
                                                  maskFBO.fboTex);
                                gl()->texParameteri(GL_TEXTURE_2D,
                                                    GL_TEXTURE_SWIZZLE_A,
                                                    GL_RED);
                                gl()->bindTexture(GL_TEXTURE_2D, 0);
                            }
                            gl()->deleteFramebuffers(1, &maskFBO.fboId);
                            m_compositorContext->putMaskTextureToCache(
                                maskFBO.fboTex, maskFBO.textureSize.width(),
                                maskFBO.textureSize.height(),
                                maskFBO.textureFormat, screenResult,
                                visibleArea);
                        }

                        auto clipArea = toRect(dest);
                        clipArea.intersect(lastState.clipRect);
                        gl()->enable(GL_SCISSOR_TEST);
                        scissor(clipArea.x(), clipArea.y(), clipArea.width(),
                                clipArea.height());
                        scissorClippingEnabled = true;
                    } else {
                        shouldSkipTexturePainting = true;
                    }
                }
            } else {
                shouldSkipTexturePainting = true;
            }
        }

        if (!shouldSkipTexturePainting) {
            if (csGL->m_isEGLImageExternal) {
                float texPosition[8];
                float clipPosition[8];
                computeTexturePosition(dst, ctm, screenMatrix, screenWidth,
                                       screenHeight, texPosition,
                                       activeClip ? clipPosition : nullptr);
                drawTexture(
                    csGL, texPosition, csGL->m_textureFragments[0].textureID,
#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
                    GL_TEXTURE_2D, -1,
#else
                    GL_TEXTURE_EXTERNAL_OES, -1,
#endif
                    csGL->m_bufferWidth, csGL->m_bufferHeight, maskFBO.fboTex,
                    maskUV, activeClip ? clipPosition : nullptr,
                    activeClip ? lastState.roundedRectClips : nullptr,
                    activeClip ? lastState.roundedRectClipCount : 0);
            } else {
                size_t coveredRowsCount = 0;
                size_t i = 0;
                for (size_t y = 0; y < csGL->hTextureCount(); y++) {
                    size_t coveredColsCount = 0;
                    for (size_t x = 0; x < csGL->wTextureCount(); x++) {
                        auto& fragment = textureInfo[i];

                        if (fragment.textureID) {
                            size_t texureDataX = coveredColsCount;
                            size_t texureDataY = coveredRowsCount;
                            size_t texureDataWidth = fragment.textureWidth;
                            size_t texureDataHeight = fragment.textureHeight;

                            float newDest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

                            float oldW = dst.width();
                            float oldH = dst.height();
                            Unit::Rect newDst(oldW * fragment.srcX + dst.x(),
                                              oldH * fragment.srcY + dst.y(),
                                              oldW * fragment.srcWidth,
                                              oldH * fragment.srcHeight);

                            SkPoint pt;
                            pt = SkPoint::Make(newDst.x(), newDst.y());

                            ctm.mapPoints(&pt, 1);
                            newDest[0][0] = pt.x();
                            newDest[0][1] = pt.y();

                            pt = SkPoint::Make(newDst.x(), newDst.maxY());
                            ctm.mapPoints(&pt, 1);
                            newDest[1][0] = pt.x();
                            newDest[1][1] = pt.y();

                            pt = SkPoint::Make(newDst.maxX(), newDst.y());
                            ctm.mapPoints(&pt, 1);
                            newDest[2][0] = pt.x();
                            newDest[2][1] = pt.y();

                            pt = SkPoint::Make(newDst.maxX(), newDst.maxY());
                            ctm.mapPoints(&pt, 1);
                            newDest[3][0] = pt.x();
                            newDest[3][1] = pt.y();

                            float minX = newDest[0][0], minY = newDest[0][1],
                                  maxX = newDest[0][0], maxY = newDest[0][1];

                            for (size_t i = 1; i < 4; i++) {
                                minX = std::min(newDest[i][0], minX);
                                minY = std::min(newDest[i][1], minY);
                                maxX = std::max(newDest[i][0], maxX);
                                maxY = std::max(newDest[i][1], maxY);
                            }

                            Unit::Rect screenBoundingRect(
                                minX, minY, std::abs(maxX - minX),
                                std::abs(maxY - minY));

                            if (screenBoundingRect.intersects(visibleArea)) {
                                GLuint tid = (GLuint)fragment.textureID;
                                float texPosition[8];
                                float clipPosition[8];
                                computeTexturePosition(
                                    newDst, ctm, screenMatrix, screenWidth,
                                    screenHeight, texPosition,
                                    activeClip ? clipPosition : nullptr);

                                if (maskFBO.fboTex) {
                                    auto w = visibleArea.width();
                                    auto h = visibleArea.height();
                                    float fw = w / maskFBO.textureSize.width();
                                    float fh = h / maskFBO.textureSize.height();
                                    maskUV[0] =
                                        (minX - visibleArea.x()) / w * fw;
                                    maskUV[1] =
                                        (minY - visibleArea.y()) / h * fh;
                                    maskUV[2] = (maxX - minX) / w * fw;
                                    maskUV[3] = (maxY - minY) / h * fh;
                                }

                                drawTexture(
                                    csGL, texPosition, tid, GL_TEXTURE_2D,
                                    GL_TEXTURE0, texureDataWidth,
                                    texureDataHeight, maskFBO.fboTex, maskUV,
                                    activeClip ? clipPosition : nullptr,
                                    activeClip ? lastState.roundedRectClips
                                               : nullptr,
                                    activeClip ? lastState.roundedRectClipCount
                                               : 0);
                            }
                        }
                        i++;
                        coveredColsCount += csGL->textureTileSize();
                    }

                    coveredRowsCount += csGL->textureTileSize();
                }
            }
        }

        if (scissorClippingEnabled) {
            gl()->disable(GL_SCISSOR_TEST);
        }
    }

    virtual void postMatrix(const SkMatrix& matrix) override
    {
        auto& lastState = m_state.back();
        lastState.matrix.preConcat(matrix);

        if (!lastState.matrix.rectStaysRect()) {
            lastState.matrixStaysInRect = false;
        }
    }

    SkMatrix currentTransformMatrix()
    {
        return m_state.back().matrix;
    }

    virtual void applyMatrixTo(LayoutLocation& lp) override
    {
        SkPoint point = SkPoint::Make((float)lp.x(), (float)lp.y());
        m_state.back().matrix.mapPoints(&point, 1);
        lp.setX(point.x());
        lp.setY(point.y());
    }

    virtual void applyMatrixTo(LayoutRect& lp) override
    {
        SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)lp.x()),
                                      SkFloatToScalar((float)lp.y()),
                                      SkFloatToScalar((float)lp.width()),
                                      SkFloatToScalar((float)lp.height()));
        m_state.back().matrix.mapRect(&sss);
        sss.sort();
        lp.setX(sss.x());
        lp.setY(sss.y());
        lp.setWidth(sss.width());
        lp.setHeight(sss.height());
    }

    virtual void resetMatrixAndClip() override
    {
        auto& lastState = m_state.back();
        lastState.matrix = SkMatrix::I();
        lastState.matrixStaysInRect = true;
        lastState.clipRect = Unit::Rect(0, 0, screenWidth(), screenHeight());
        lastState.abbreviatedClipPaths.clear();
        lastState.pathCommands.clear();
        lastState.roundedRectClipCount = 0;
        lastState.roundedClipChainOk = true;
        applyDevicePixelRatio();
    }

    virtual void resetClip()
    {
        auto& lastState = m_state.back();
        lastState.clipRect = Unit::Rect(0, 0, screenWidth(), screenHeight());
        lastState.abbreviatedClipPaths.clear();
        lastState.pathCommands.clear();
        lastState.roundedRectClipCount = 0;
        lastState.roundedClipChainOk = true;
    }

    static void addToPath(Clipper2Lib::PathD& path, const SkMatrix& matrix,
                          float x, float y)
    {
        SkPoint pt = SkPoint::Make(x, y);
        matrix.mapPoints(&pt, 1);
        path.emplace_back(Clipper2Lib::PointD(pt.x(), pt.y()));
    }

    virtual void moveTo(float x, float y) override
    {
        addToPath(m_abbreviatedPath, m_state.back().matrix, x, y);
        m_pathCommands.push_back(
            { CompositorImplGLState::PathCommand::Command::MoveTo,
              x,
              y,
              { 0, 0, 0 },
              m_state.back().matrix });
    }

    virtual void lineTo(float x, float y) override
    {
        addToPath(m_abbreviatedPath, m_state.back().matrix, x, y);
        m_pathCommands.push_back(
            { CompositorImplGLState::PathCommand::Command::LineTo,
              x,
              y,
              { 0, 0, 0 },
              m_state.back().matrix });
    }

    virtual void arcNegative(double cx, double cy, double radius, double angle1,
                             double angle2) override
    {
        m_pathCommands.push_back(
            { CompositorImplGLState::PathCommand::Command::ArcNegative,
              (float)cx,
              (float)cy,
              { (float)radius, (float)angle1, (float)angle2 },
              m_state.back().matrix });

        float angleDiff = angle2 - angle1;
        if (std::abs(angleDiff) >= M_PI * 2) {
            angleDiff = -M_PI * 2;
        } else {
            while (angleDiff > 0.0f) {
                angleDiff -= M_PI * 2;
            }
        }

        float scale = m_state.back().matrix.getScaleX() *
                      m_state.back().matrix.getScaleY();
        float arcLength = std::abs(radius * angleDiff * scale);
        size_t c = std::min(static_cast<size_t>(arcLength / 2.0f),
                            static_cast<size_t>(8));
        c = std::max(c, static_cast<size_t>(2));
        m_abbreviatedPath.reserve(m_abbreviatedPath.size() + c + 1);
        float step = 1.0f / static_cast<float>(c);
        for (size_t i = 0; i <= c; i++) {
            float t = i * step;
            float a = angle1 + angleDiff * t;
            float dx = cos(a);
            float dy = sin(a);
            float x = cx + dx * radius;
            float y = cy + dy * radius;
            addToPath(m_abbreviatedPath, m_state.back().matrix, x, y);
        }
    }
    // Decide PURELY from geometry whether a clip path is an axis-aligned
    // rounded rectangle, and if so recover its center/half-size/radius (all in
    // the path's own space). This is independent of how the path was built
    // (arcs, beziers, line segments) - it estimates the corner radius from the
    // 45-degree diagonal extreme of each corner and then VERIFIES every point
    // lies on that rounded-rect boundary (rounded-box SDF ~ 0). A rotated,
    // sheared, elliptical, or non-rectangular shape fails the verification and
    // falls back to the mask path. `path` is the clip outline already mapped to
    // logical-screen space.
    bool detectRoundedRectClip(const Clipper2Lib::PathD& path,
                               CompositorImplGLState::RoundedRectClip& out)
    {
        const size_t n = path.size();
        if (n < 8) { // a rounded rect tessellates to clearly more than this
            return false;
        }

        double minX = path[0].x, minY = path[0].y;
        double maxX = path[0].x, maxY = path[0].y;
        for (const auto& p : path) {
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
            maxX = std::max(maxX, p.x);
            maxY = std::max(maxY, p.y);
        }
        double cx = (minX + maxX) * 0.5;
        double cy = (minY + maxY) * 0.5;
        double hx = (maxX - minX) * 0.5;
        double hy = (maxY - minY) * 0.5;
        if (hx <= 0.5 || hy <= 0.5) {
            return false;
        }

        // Farthest extent of each corner along its diagonal. For a rounded rect
        // the (1,1) extreme sits on the corner arc at 45 degrees:
        //   max(px+py) = hx + hy - r*(2 - sqrt2)  =>  r = (hx+hy - s)/(2-sqrt2)
        double spp = -1e30, spm = -1e30, smp = -1e30, smm = -1e30;
        for (const auto& p : path) {
            double px = p.x - cx, py = p.y - cy;
            spp = std::max(spp, px + py);
            spm = std::max(spm, px - py);
            smp = std::max(smp, -px + py);
            smm = std::max(smm, -px - py);
        }
        const double k = 2.0 - std::sqrt(2.0);
        double rEst[4] = { (hx + hy - spp) / k, (hx + hy - spm) / k,
                           (hx + hy - smp) / k, (hx + hy - smm) / k };
        double r = (rEst[0] + rEst[1] + rEst[2] + rEst[3]) * 0.25;
        double rmax = std::min(hx, hy);
        if (r < -1.0 || r > rmax + 1.0) {
            return false;
        }
        r = std::max(0.0, std::min(r, rmax));

        // All four corners must agree (symmetric, circular corners).
        double agreeTol = std::max(1.5, 0.15 * r);
        for (int i = 0; i < 4; i++) {
            if (std::abs(rEst[i] - r) > agreeTol) {
                return false;
            }
        }

        // Verify: every boundary point must lie on the rounded-rect outline.
        // (Coarse arc tessellation sits slightly inside the true arc, so allow
        // a small tolerance; the shader still renders the exact rounded rect.)
        double vtol = std::max(2.0, 0.15 * r + 1.0);
        for (const auto& p : path) {
            double px = std::abs(p.x - cx);
            double py = std::abs(p.y - cy);
            double qx = px - hx + r;
            double qy = py - hy + r;
            double outside = std::sqrt(std::max(qx, 0.0) * std::max(qx, 0.0) +
                                       std::max(qy, 0.0) * std::max(qy, 0.0));
            double sd = std::min(std::max(qx, qy), 0.0) + outside - r;
            if (std::abs(sd) > vtol) {
                return false;
            }
        }

        out.cx = (float)cx;
        out.cy = (float)cy;
        out.hx = (float)hx;
        out.hy = (float)hy;
        out.radius = (float)r;
        return true;
    }

    // True if rounded rect 'inner' is contained in 'outer' (both axis-aligned,
    // logical-screen space). Samples inner's corner arcs against outer's SDF;
    // straight edges between corners are covered by convexity. Lets a chain of
    // nested rounded clips be reduced to its innermost rect.
    static bool roundedRectInside(
        const CompositorImplGLState::RoundedRectClip& outer,
        const CompositorImplGLState::RoundedRectClip& inner)
    {
        auto sdOuter = [&](float x, float y) -> float {
            float qx = std::abs(x - outer.cx) - outer.hx + outer.radius;
            float qy = std::abs(y - outer.cy) - outer.hy + outer.radius;
            float ox = std::max(qx, 0.f), oy = std::max(qy, 0.f);
            return std::min(std::max(qx, qy), 0.f) +
                   std::sqrt(ox * ox + oy * oy) - outer.radius;
        };
        const float tol = 0.75f; // sub-pixel slack
        const float sgn[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };
        for (int c = 0; c < 4; c++) {
            float ccx = inner.cx + sgn[c][0] * (inner.hx - inner.radius);
            float ccy = inner.cy + sgn[c][1] * (inner.hy - inner.radius);
            for (int k = 0; k <= 4; k++) {
                float ang = (float)k * (1.5707963f / 4.f); // 0..pi/2
                float px = ccx + sgn[c][0] * inner.radius * std::cos(ang);
                float py = ccy + sgn[c][1] * inner.radius * std::sin(ang);
                if (sdOuter(px, py) > tol) {
                    return false;
                }
            }
        }
        return true;
    }

    virtual void clipPath() override
    {
        auto& lastState = m_state.back();
        if (m_abbreviatedPath.size()) {
            // Track rounded-rect clips for the analytic SDF shader path.
            // Nested clips (one contained in the other) are compressed: the
            // outer is discarded, keeping only the inner. Non-nested clips are
            // appended up to kMaxAnalyticRoundedClips; overflow breaks the
            // chain and forces the mask-FBO path.
            CompositorImplGLState::RoundedRectClip rr;
            bool detected = detectRoundedRectClip(m_abbreviatedPath, rr);
            if (!lastState.roundedClipChainOk) {
                // already on the mask path; nothing more to track
            } else if (!detected) {
                lastState.roundedClipChainOk = false;
            } else {
                // Try nesting compression: if new and an existing clip are
                // nested, keep the inner (tighter) one and discard the outer.
                bool compressed = false;
                for (int i = 0; i < lastState.roundedRectClipCount; i++) {
                    const auto& cur = lastState.roundedRectClips[i];
                    bool newIsSmaller = rr.hx * rr.hy <= cur.hx * cur.hy;
                    const auto& inner = newIsSmaller ? rr : cur;
                    const auto& outer = newIsSmaller ? cur : rr;
                    if (roundedRectInside(outer, inner)) {
                        lastState.roundedRectClips[i] = inner;
                        compressed = true;
                        break;
                    }
                }
                if (!compressed) {
                    if (lastState.roundedRectClipCount <
                        CompositorImplGLState::kMaxAnalyticRoundedClips) {
                        lastState.roundedRectClips
                            [lastState.roundedRectClipCount++] = rr;
                    } else {
                        lastState.roundedClipChainOk = false;
                    }
                }
            }
            lastState.abbreviatedClipPaths.push_back(
                std::move(m_abbreviatedPath));
            lastState.pathCommands.push_back(std::move(m_pathCommands));
            lastState.computedPathCommands = Optional<Clipper2Lib::PathsD>();
        }
    }

    virtual void enableBlurEffect(float blurRadius) override
    {
        m_state.back().blurRadius = blurRadius;
    }

    struct FBOState {
        GLuint fboId = 0;
        GLuint fboTex = 0;
        LayoutRect viewport;
        GLenum textureFormat = 0;
        Unit::IntSize textureSize;
    };

    void pushFBOContext(size_t width, size_t height, LayoutRect viewport,
                        GLenum textureFormat = GL_RGBA)
    {
        m_seenFBOUsage = true;

        if (!m_gotBaseFBORBOId) {
            m_gotBaseFBORBOId = true;
            GLint fbo = 0, rbo = 0;
            gl()->getIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
            gl()->getIntegerv(GL_RENDERBUFFER_BINDING, &rbo);
            m_baseFBOId = static_cast<GLuint>(fbo);
            m_baseRBOId = static_cast<GLuint>(rbo);
        }

        FBOState newFBOState;
        newFBOState.textureFormat = textureFormat;
        newFBOState.textureSize = Unit::IntSize(width, height);

        CompositorContextGL::FBOCacheEntry cachedFBO;
        bool tookFromFBOCache = m_compositorContext->takeFBOFromCache(
            width, height, textureFormat, cachedFBO);

        if (tookFromFBOCache) {
            newFBOState.fboId = cachedFBO.fboId;
            newFBOState.fboTex = cachedFBO.textureId;

            gl()->bindFramebuffer(GL_FRAMEBUFFER, newFBOState.fboId);
            checkError(gl());
        } else {
            gl()->genFramebuffers(1, &newFBOState.fboId);
            checkError(gl());

            bool tookFromCache = true;
            newFBOState.fboTex =
                m_compositorContext->takeGenericTextureFromCache(width, height,
                                                                 textureFormat);
            if (newFBOState.fboTex == 0) {
                gl()->genTextures(1, &newFBOState.fboTex);
                tookFromCache = false;
                checkError(gl());
            }

            gl()->bindFramebuffer(GL_FRAMEBUFFER, newFBOState.fboId);
            checkError(gl());

            if (!tookFromCache) {
                gl()->bindTexture(GL_TEXTURE_2D, newFBOState.fboTex);
                checkError(gl());
                gl()->texImage2D(
                    GL_TEXTURE_2D, 0,
                    textureFormat == GL_RED ? GL_R8 : textureFormat, width,
                    height, 0, textureFormat, GL_UNSIGNED_BYTE, nullptr);
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE);
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE);
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
                gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
                checkError(gl());
                gl()->bindTexture(GL_TEXTURE_2D, 0);
            }

            gl()->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D, newFBOState.fboTex, 0);
            checkError(gl());
        }

        newFBOState.viewport = viewport;
        gl()->viewport(viewport.x(), viewport.y(), viewport.width(),
                       viewport.height());

        m_fboState.push_back(newFBOState);
    }

    FBOState popFBOContext(bool putFBOToCache = true) // returns texture
    {
        FBOState lastState = m_fboState.back();
        m_fboState.pop_back();

        if (m_fboState.size()) {
            auto& s = m_fboState.back();
            gl()->bindFramebuffer(GL_FRAMEBUFFER, s.fboId);

            gl()->viewport(s.viewport.x(), s.viewport.y(), s.viewport.width(),
                           s.viewport.height());
        } else {
            if (m_baseRBOId) {
                gl()->bindRenderbuffer(GL_RENDERBUFFER, m_baseRBOId);
            }
            gl()->bindFramebuffer(GL_FRAMEBUFFER, m_baseFBOId);
            setViewport();
        }

        if (putFBOToCache) {
            m_compositorContext->putFBOToCache(
                lastState.fboId, lastState.fboTex,
                lastState.textureSize.width(), lastState.textureSize.height(),
                lastState.textureFormat);
        }
        return lastState;
    }

protected:
    bool m_seenFBOUsage;
    bool m_gotBaseFBORBOId;
    float m_globalScale;
    size_t m_screenWidth;
    size_t m_screenHeight;
    WebView* m_webView;
    CompositorContextGL* m_compositorContext;
    std::vector<CompositorImplGLState> m_state;
    std::vector<FBOState> m_fboState;
    GLuint m_baseFBOId;
    GLuint m_baseRBOId;

    Clipper2Lib::PathD m_abbreviatedPath;
    std::vector<CompositorImplGLState::PathCommand> m_pathCommands;
    SkMatrix m_screenMatrix;

    // Lazy full-screen clear: clearColor() defers the glClear; it is dropped if
    // the next screen op is a full-screen opaque rect that fully covers it
    // (the clear would be redundant), otherwise flushed before that op. Only
    // ever a default-framebuffer clear (FBO clears use gl()->clear directly).
    bool m_pendingClear;
    Unit::Color m_pendingClearColor;
};

Compositor* CompositorFactory::create3dGl(WebView* webView,
                                          CompositorContext* ctx)
{
    return new CompositorImplGL(webView, ctx);
}

Compositor* CompositorFactory::create2dGl(WebView* webView,
                                          CompositorContext* ctx,
                                          CanvasSurface* surface)
{
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

bool CompositorFactory::supportsFilterEffectGl(size_t textureWidth,
                                               size_t textureHeight)
{
    if (g_needsRGBShuffle) {
        return false;
    }
    if (textureWidth > g_maxTextureSize || textureHeight > g_maxTextureSize) {
        return false;
    }
    return true;
}

#if defined(STARFISH_ENABLE_TEST)
#if defined(PORT_CANVAS_BACKEND_CAIRO)

// Dump OpenGL texture to PNG file for debugging
// This function reads texture data using FBO and saves it as PNG
// Parameters:
//   gl - GL interface pointer
//   textureId - OpenGL texture ID to dump
//   width - texture width
//   height - texture height
//   path - output PNG file path
//   textureTarget - GL_TEXTURE_2D or GL_TEXTURE_EXTERNAL_OES (default:
//   GL_TEXTURE_2D)
void dumpTextureToPNG(GL* gl, GLuint textureId, int width, int height,
                      const char* path, GLenum textureTarget)
{
    if (!gl || !textureId || !path || width <= 0 || height <= 0) {
        STARFISH_LOG_ERROR("dumpTextureToPNG: Invalid parameters");
        return;
    }

    STARFISH_LOG_DEBUG("dumpTextureToPNG: textureId=%u, size=%dx%d, path=%s",
                       textureId, width, height, path);

    // Save current FBO and texture bindings
    GLint oldFBO = 0;
    GLint oldTexture = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);

    // Create FBO for reading texture
    GLuint fbo = 0;
    gl->genFramebuffers(1, &fbo);
    gl->bindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Attach texture to FBO
    gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             textureTarget, textureId, 0);

    // Check framebuffer status
    GLenum status = gl->checkFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        STARFISH_LOG_ERROR(
            "dumpTextureToPNG: Framebuffer not complete, status=0x%x", status);
        gl->bindFramebuffer(GL_FRAMEBUFFER, oldFBO);
        gl->deleteFramebuffers(1, &fbo);
        return;
    }

    // Wait for all GL operations to complete
    gl->finish();

    // Allocate buffer for texture data
    int rowLength = width * 4;
    int dataLength = rowLength * height;
    uint8_t* buffer = new uint8_t[dataLength];

    // Set pixel alignment
    gl->pixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->pixelStorei(GL_PACK_ALIGNMENT, 1);

    // Read pixels from texture
    gl->readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    // Convert RGBA to BGRA for PNG (cairo format)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t* pixel = &buffer[rowLength * y + x * 4];
            std::swap(pixel[0], pixel[2]); // swap R and B
        }
    }

    // Flip vertically (OpenGL has origin at bottom-left, PNG at top-left)
    for (int y = 0; y < height / 2; y++) {
        uint32_t* row1 = (uint32_t*)&buffer[rowLength * y];
        uint32_t* row2 = (uint32_t*)&buffer[rowLength * (height - y - 1)];
        for (int x = 0; x < width; x++) {
            std::swap(row1[x], row2[x]);
        }
    }

    // Write to PNG file using cairo
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        buffer, CAIRO_FORMAT_ARGB32, width, height, rowLength);
    cairo_status_t result = cairo_surface_write_to_png(surface, path);
    cairo_surface_destroy(surface);

    if (result != CAIRO_STATUS_SUCCESS) {
        STARFISH_LOG_ERROR("dumpTextureToPNG: Failed to write PNG: %s",
                           cairo_status_to_string(result));
    } else {
        STARFISH_LOG_DEBUG("dumpTextureToPNG: Successfully saved to %s", path);
    }

    // Cleanup
    delete[] buffer;
    gl->bindFramebuffer(GL_FRAMEBUFFER, oldFBO);
    gl->deleteFramebuffers(1, &fbo);

    // Note: We don't restore texture binding for GL_TEXTURE_EXTERNAL_OES
    // since we only saved GL_TEXTURE_BINDING_2D
    if (textureTarget == GL_TEXTURE_2D) {
        gl->bindTexture(GL_TEXTURE_2D, oldTexture);
    }
}

void screenShotImpl(Renderer* renderer, const char* path,
                    std::function<void()> callback)
{
    GL* gl = renderer->gl();
    gl->finish();

    auto deviceWidth = renderer->width();
    auto deviceHeight = renderer->height();
    auto rowLength = deviceWidth * 4;

    auto dataLength = rowLength * deviceHeight;

    gl->pixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->pixelStorei(GL_PACK_ALIGNMENT, 1);
    uint8_t* buffer = new uint8_t[dataLength];
    gl->readPixels(0, 0, deviceWidth, deviceHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                   buffer);

    // convert to rgba to bgra for cairo
    for (uint32_t y = 0; y < deviceHeight; y++) {
        for (uint32_t x = 0; x < deviceWidth; x++) {
            uint8_t* head = &buffer[rowLength * y + x * 4];
            std::swap(head[0], head[2]);
        }
    }

    // flip W
    /*
        for (uint32_t y = 0; y < deviceHeight; y++) {
            uint32_t* head = (uint32_t*)&buffer[rowLength * y];
            for (uint32_t x = 0; x < deviceWidth / 2; x++) {
                std::swap(head[x], head[deviceWidth - x - 1]);
            }
        }
    */
    // flip H
    for (uint32_t y = 0; y < deviceHeight / 2; y++) {
        uint32_t* head = (uint32_t*)&buffer[rowLength * y];
        uint32_t* head2 =
            (uint32_t*)&buffer[rowLength * (deviceHeight - y - 1)];
        for (uint32_t x = 0; x < deviceWidth; x++) {
            std::swap(head[x], head2[x]);
        }
    }

    cairo_surface_t* png_buffer;
    png_buffer = cairo_image_surface_create_for_data(
        (unsigned char*)buffer, CAIRO_FORMAT_ARGB32, deviceWidth, deviceHeight,
        rowLength);

    cairo_surface_write_to_png(png_buffer, path);
    cairo_surface_destroy(png_buffer);

    delete[] buffer;
    callback();
}
#endif
#endif

} // namespace Starfish

#endif
