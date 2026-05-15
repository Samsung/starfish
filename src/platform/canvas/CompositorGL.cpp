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

#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
#include "platform/multimedia/MediaPlayerLinux.h"
#endif

#include <array>
#include <clipper2/clipper.h>

#if defined(STARFISH_ENABLE_TEST)
#include <dlfcn.h>
#endif

namespace std {
template <>
struct tuple_size<Clipper2Lib::PointD> : integral_constant<size_t, 2> {
};

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
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
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
static bool g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory = false;
static bool g_isSupportExtensionEGLImageExternal = false;
static bool g_isSupportBGRATexture = false;
static bool g_isSupportTextureSwizzle = false;
static bool g_shouldUseEGLImageOnPlainSurface = true;
static bool g_needsRGBShuffle = true;
static bool g_isSupported_EGL_NATIVE_SURFACE_TIZEN = false;
static bool g_isSupportStandardDerivatives = false;

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
    GLuint m_polygonShaderProgram;
    GLint m_polygonShaderProgramPosition;
    GLint m_polygonShaderProgramColor;

    // Anti-aliased line shader for polygon outlines
    GLuint m_lineVertexShader;
    GLuint m_lineFragmentShader;
    GLuint m_lineShaderProgram;
    GLint m_lineShaderProgramPosition;
    GLint m_lineShaderProgramColor;
    GLint m_lineShaderProgramEdgeDistance;
    GLint m_lineShaderProgramLineWidth;

    GLuint m_rectVertexShader;
    GLuint m_pixelFragmentShader;
    GLuint m_rectShaderProgram;
    GLint m_rectShaderProgramPosition;
    GLint m_rectShaderProgramColor;
    GLint m_rectShaderProgramTexIdx;

    GLuint m_texVertexShader;
    GLuint m_texFragmentShader;
    GLuint m_texShaderProgram;
    GLint m_texShaderProgramTexPos;
    GLint m_texShaderProgramTexIdx;
    GLint m_texShaderProgramPosition;
    GLint m_texShaderProgramTexture;
    GLint m_texShaderProgramAlpha;

    GLuint m_texFragmentShaderEGLImageExternal;
    GLuint m_texShaderProgramEGLImageExternal;
    GLint m_texShaderProgramEGLImageExternalTexPos;
    GLint m_texShaderProgramEGLImageExternalTexIdx;
    GLint m_texShaderProgramEGLImageExternalPosition;
    GLint m_texShaderProgramEGLImageExternalTexture;
    GLint m_texShaderProgramEGLImageExternalAlpha;

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

    GLuint m_texBlurShaderProgramH;
    GLint m_texBlurShaderProgramHTexPos;
    GLint m_texBlurShaderProgramHTexIdx;
    GLint m_texBlurShaderProgramHPosition;
    GLint m_texBlurShaderProgramHTexture;
    GLint m_texBlurShaderProgramHBlurRadius;
    GLint m_texBlurShaderProgramHTextureWidth;
    GLint m_texBlurShaderProgramHTextureHeight;
    GLint m_texBlurShaderProgramHAlpha;
    GLint m_texBlurShaderProgramHAlphaMask;

    GLuint m_texTexPosBuffer;
    GLuint m_texIdxBuffer;

    GLuint m_lastProgram;

    std::vector<std::tuple<size_t, size_t, GLuint>> m_cachedTextures;

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
        m_polygonShaderProgramPosition = 0;
        m_polygonShaderProgramColor = 0;

        // Anti-aliased line shader
        m_lineVertexShader = m_lineFragmentShader = m_lineShaderProgram = 0;
        m_lineShaderProgramPosition = 0;
        m_lineShaderProgramColor = 0;
        m_lineShaderProgramEdgeDistance = 0;
        m_lineShaderProgramLineWidth = 0;
        m_rectVertexShader = m_pixelFragmentShader = m_rectShaderProgram = 0;
        m_rectShaderProgramPosition = 0;
        m_rectShaderProgramColor = 0;
        m_rectShaderProgramTexIdx = 0;
        m_texShaderProgramPosition = 0;
        m_texShaderProgramTexture = 0;
        m_texShaderProgramAlpha = 0;
        m_texShaderProgramEGLImageExternalPosition = 0;
        m_texShaderProgramEGLImageExternalTexture = 0;
        m_texShaderProgramEGLImageExternalAlpha = 0;
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
        m_texBlurShaderProgramHAlphaMask = 0;

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
    }

    ~CompositorContextGL()
    {
        STARFISH_LOG_INFO("CompositorContextGL::~CompositorContextGL");
        gl()->useProgram(0);

        cleanUpTextureCache();
        cleanUpGLPrograms();

        gl()->deleteBuffers(1, &m_texTexPosBuffer);
        gl()->deleteBuffers(1, &m_texIdxBuffer);

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
        std::vector<std::tuple<size_t, size_t, GLuint>>().swap(
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
            gl()->detachShader(m_polygonShaderProgram, m_pixelFragmentShader);
            gl()->deleteProgram(m_polygonShaderProgram);
            gl()->deleteShader(m_polygonVertexShader);
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

        if (m_texShaderProgram) {
            gl()->detachShader(m_texShaderProgram, m_texVertexShader);
            gl()->detachShader(m_texShaderProgram, m_texFragmentShader);
            gl()->deleteProgram(m_texShaderProgram);
        }

        if (m_texVertexShader) {
            gl()->deleteShader(m_texVertexShader);
        }

        if (m_texFragmentShader) {
            gl()->deleteShader(m_texFragmentShader);
        }
        clearGLProgramVariables();
    }

    void putGenericTextureToCache(GLuint textureID, size_t textureDataWidth,
                                  size_t textureDataHeight)
    {
        m_cachedTextures.push_back(
            std::make_tuple(textureDataWidth, textureDataHeight, textureID));
    }

    GLuint takeGenericTextureFromCache(size_t textureDataWidth,
                                       size_t textureDataHeight)
    {
        for (size_t i = 0; i < m_cachedTextures.size(); i++) {
            if (std::get<0>(m_cachedTextures[i]) == textureDataWidth &&
                std::get<1>(m_cachedTextures[i]) == textureDataHeight) {
                GLuint textureID = std::get<2>(m_cachedTextures[i]);
                m_cachedTextures.erase(m_cachedTextures.begin() + i);
                return textureID;
            }
        }
        return 0;
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
            GLchar polygonVertexSource[] =
                "attribute vec2 aPosition;\n"
                "void main() {\n"
                "  gl_Position = vec4(aPosition.xy, 0.0, 1.0);\n"
                "}";

            m_polygonVertexShader =
                loadShader(gl(), GL_VERTEX_SHADER, polygonVertexSource);
            checkError(gl());

            ensurePixelFragmentShader();

            m_polygonShaderProgram = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_polygonShaderProgram, m_polygonVertexShader);
            checkError(gl());
            gl()->attachShader(m_polygonShaderProgram, m_pixelFragmentShader);
            checkError(gl());

            gl()->linkProgram(m_polygonShaderProgram);
            checkError(gl());

            m_lastProgram = m_polygonShaderProgram;
            gl()->useProgram(m_polygonShaderProgram);

            m_polygonShaderProgramPosition =
                gl()->getAttribLocation(m_polygonShaderProgram, "aPosition");
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

    // Anti-aliased line shader for polygon outlines
    GLuint lineProgram()
    {
        if (!m_lineShaderProgram) {
            // Vertex shader with edge distance for anti-aliased lines
            GLchar lineVertexSource[] =
                "attribute vec2 aPosition;\n"
                "attribute float aEdgeDistance;\n"
                "varying float vEdgeDistance;\n"
                "void main() {\n"
                "  gl_Position = vec4(aPosition.xy, 0.0, 1.0);\n"
                "  vEdgeDistance = aEdgeDistance;\n"
                "}";

            m_lineVertexShader =
                loadShader(gl(), GL_VERTEX_SHADER, lineVertexSource);
            checkError(gl());

            // Fragment shader with anti-aliasing using fwidth()
            // vEdgeDistance: 0 = center, negative = left edge, positive = right
            // edge Smooth falloff on both sides for softer anti-aliasing
            const GLchar* lineFragmentSource =
                "#ifdef GL_ES\n"
                "  #extension GL_OES_standard_derivatives : enable\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform vec4 uColor;\n"
                "uniform float uLineWidth;\n"
                "varying float vEdgeDistance;\n"
                "void main(void)\n"
                "{\n"
                "#ifdef GL_OES_standard_derivatives\n"
                "  float halfWidth = uLineWidth * 0.5;\n"
                "  float dist = abs(vEdgeDistance - halfWidth);\n"
                "  float fw = fwidth(vEdgeDistance);\n"
                "  // Wider feathering range (1.5x) for smoother curves\n"
                "  float feather = fw * 1.5;\n"
                "  float edgeAlpha = 1.0 - smoothstep(halfWidth - feather, "
                "halfWidth + feather * 0.5, dist);\n"
                "#else\n"
                "  float edgeAlpha = 1.0;\n"
                "#endif\n"
                "  gl_FragColor = uColor * edgeAlpha;\n"
                "}";

            if (g_needsRGBShuffle) {
                lineFragmentSource =
                    "#ifdef GL_ES\n"
                    "  #extension GL_OES_standard_derivatives : enable\n"
                    "  precision mediump float;\n"
                    "#endif\n"
                    "uniform vec4 uColor;\n"
                    "uniform float uLineWidth;\n"
                    "varying float vEdgeDistance;\n"
                    "void main(void)\n"
                    "{\n"
                    "#ifdef GL_OES_standard_derivatives\n"
                    "  float halfWidth = uLineWidth * 0.5;\n"
                    "  float dist = abs(vEdgeDistance - halfWidth);\n"
                    "  float fw = fwidth(vEdgeDistance);\n"
                    "  // Wider feathering range (1.5x) for smoother curves\n"
                    "  float feather = fw * 1.5;\n"
                    "  float edgeAlpha = 1.0 - smoothstep(halfWidth - feather, "
                    "halfWidth + feather * 0.5, dist);\n"
                    "#else\n"
                    "  float edgeAlpha = 1.0;\n"
                    "#endif\n"
                    "  gl_FragColor.r = uColor[2] * edgeAlpha;\n"
                    "  gl_FragColor.g = uColor[1] * edgeAlpha;\n"
                    "  gl_FragColor.b = uColor[0] * edgeAlpha;\n"
                    "  gl_FragColor.a = uColor[3] * edgeAlpha;\n"
                    "}";
            }

            m_lineFragmentShader =
                loadShader(gl(), GL_FRAGMENT_SHADER, lineFragmentSource);
            checkError(gl());

            m_lineShaderProgram = gl()->createProgram();
            checkError(gl());

            gl()->attachShader(m_lineShaderProgram, m_lineVertexShader);
            checkError(gl());
            gl()->attachShader(m_lineShaderProgram, m_lineFragmentShader);
            checkError(gl());

            gl()->linkProgram(m_lineShaderProgram);
            checkError(gl());

            m_lastProgram = m_lineShaderProgram;
            gl()->useProgram(m_lineShaderProgram);

            m_lineShaderProgramPosition =
                gl()->getAttribLocation(m_lineShaderProgram, "aPosition");
            m_lineShaderProgramColor =
                gl()->getUniformLocation(m_lineShaderProgram, "uColor");
            m_lineShaderProgramEdgeDistance =
                gl()->getAttribLocation(m_lineShaderProgram, "aEdgeDistance");
            m_lineShaderProgramLineWidth =
                gl()->getUniformLocation(m_lineShaderProgram, "uLineWidth");
        } else {
            if (m_lastProgram != m_lineShaderProgram) {
                m_lastProgram = m_lineShaderProgram;
                gl()->useProgram(m_lineShaderProgram);
            }
        }

        return m_lineShaderProgram;
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

    GLuint texShaderProgramEGLImageExternal()
    {
        if (!m_texShaderProgramEGLImageExternal) {
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
                "  vec4 texColor = texture2D(uTexture, vTexPos);\n"
                "  gl_FragColor = texColor * uAlpha;\n"
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
                    "  vec4 texData = texture2D(uTexture, vTexPos);\n"
                    "  texData = texData * uAlpha;\n"
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

    GLuint texShaderProgram()
    {
        if (!m_texShaderProgram) {
            // We only Support OpenGL ES 2.0+ context
            // but some develoment environment only support desktop context
            // so we add `#ifdef GL_ES` for debug purpose
            const GLchar* texFragmentSource =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform sampler2D uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  vec4 texColor = texture2D(uTexture, vTexPos);\n"
                "  gl_FragColor = texColor * uAlpha;\n"
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
                    "  vec4 texData = texture2D(uTexture, vTexPos);\n"
                    "  texData = texData * uAlpha;\n"
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
        bool isEGLImage, bool addColorAlign = false)
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
        ss << "uniform sampler2D uAlphaMask;\n";

        ss << "uniform float uTextureWidth;\n";
        ss << "uniform float uTextureHeight;\n";
        if (addColorAlign) {
            ss << "uniform float uAlpha;\n";
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

        if (addColorAlign) {
            ss << "  float maskAlpha = texture2D(uAlphaMask, vTexPos).a;\n";
            ss << "  gl_FragColor = total * uAlpha * maskAlpha;\n";
        } else {
            ss << "  gl_FragColor = total;\n";
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
                bindTexIdx(m_texBlurShaderProgramWTexIdx, true);
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
        m_texBlurShaderProgramHAlphaMask =
            gl()->getUniformLocation(m_texBlurShaderProgramH, "uAlphaMask");

        gl()->uniform1i(m_texBlurShaderProgramHTexture, 0);
        gl()->uniform1i(m_texBlurShaderProgramHAlphaMask, 1);
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
            g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory = true;
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
            g_isSupportStandardDerivatives =
                strstr(ex, "GL_OES_standard_derivatives") != nullptr;
        } else {
            STARFISH_LOG_INFO("GL_EXTENSIONS -> returns null...");
        }

#if (!defined(STARFISH_TIZEN) && !defined(STARFISH_ANDROID)) || \
    (defined(STARFISH_ANDROID) && !defined(USE_EGLIMAGE_EXT_ANDROID))
        g_isSupportExtensionEGLImageExternal = false;
#endif

#if defined(STARFISH_TIZEN)
        if (g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory) {
            g_shouldUseEGLImageOnPlainSurface = false;
        }
#endif

        // if efl enabled, there is no way to support egl image with evasgl
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
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

        if (g_isSupportStandardDerivatives) {
            STARFISH_LOG_INFO("support GL_OES_standard_derivatives!");
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
#if defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
        STARFISH_RELEASE_ASSERT(g_isSupported_EGL_NATIVE_SURFACE_TIZEN);
#endif
        g_needsCheckCompatibility = false;
        checkError(gl);
    }

    gl->enable(GL_BLEND);
    gl->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl->activeTexture(GL_TEXTURE0);

    gl->genBuffers(1, &compositorContext->m_texTexPosBuffer);
    gl->genBuffers(1, &compositorContext->m_texIdxBuffer);

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
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                CanvasSurfaceGL* s = (CanvasSurfaceGL*)obj;
                s->detachNativeBuffer();
            },
            NULL, NULL, NULL);
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
                    (CompositorContextGL*)m_renderer->compostiorContext();
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
                                m_textureFragments[i].textureHeight);
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
        }
    }

    bool attachNativeBuffer(size_t w, size_t h, CanvasSurfaceFlag flag) override
    {
        if (m_width != w || m_height != h) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;
            m_flag = flag;

            float devicePixelRatio =
                m_renderer->webView()->screenInfo().devicePixelRatio;

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
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
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
                                     -1, NULL, (void**)&m_buffer);
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
                                        m_renderer->compostiorContext();
                                if (ctx) {
                                    fragment.textureID =
                                        ctx->takeGenericTextureFromCache(
                                            fragment.textureWidth,
                                            fragment.textureHeight);
                                }
                            }

                            gl()->pixelStorei(GL_UNPACK_ALIGNMENT, 1);

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

#if defined(PORT_PIXEL_ORDER_BGRA)
                                if (g_isSupportTextureSwizzle &&
                                    !g_needsRGBShuffle) {
                                    GLint swizzleMask[] = { GL_BLUE, GL_GREEN,
                                                            GL_RED, GL_ALPHA };
                                    gl()->texParameteriv(GL_TEXTURE_2D,
                                                         TEXTURE_SWIZZLE_RGBA,
                                                         swizzleMask);
                                }
#endif
                            }

                            auto bData = m_buffer;
                            auto bStride = bufferStride();
                            auto kind = textureFormat();

                            bool updateWholeTexture = textureJustCreated;
                            if (!updateWholeTexture && xx == 0 && yy == 0 &&
                                xxEnd == fragment.textureWidth &&
                                yyEnd == fragment.textureHeight) {
                                updateWholeTexture = true;
                            }

                            gl()->bindTexture(GL_TEXTURE_2D,
                                              fragment.textureID);
                            checkError(gl());

                            if (g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory) {
                                gl()->pixelStorei(GL_UNPACK_ROW_LENGTH,
                                                  bufferWidth());
                                gl()->pixelStorei(GL_UNPACK_SKIP_PIXELS, xx);
                                gl()->pixelStorei(GL_UNPACK_SKIP_ROWS, yy);

                                auto data = bData;
                                data += textureDataY * bStride;
                                data += textureDataX * 4;
                                if (updateWholeTexture) {
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
                                if (updateWholeTexture) {
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
    size_t m_wTextureCount;
    size_t m_hTextureCount;
    size_t m_textureTileSize;
    GCAtomicVector<CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment>
        m_textureFragments;

    bool m_isFrameBuffer{ false };
    bool m_isEGLImageExternal;
    bool m_isEGLBufferOwner;
#if defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
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
        m_webView = webView;
        m_globalScale = m_webView->glCompsitorScale();
        m_screenWidth = m_webView->renderer()->width();
        m_screenHeight = m_webView->renderer()->height();
        m_compositorContext = (CompositorContextGL*)compositorContext;
        TransformationMatrix m = m_webView->renderer()->screenMatrix();
        m_screenMatrix.setAll(m.scaleX, m.skewX, m.translateX, m.skewY,
                              m.scaleY, m.translateY, m.perspectiveX,
                              m.perspectiveY, m.perspectiveScale);
        setViewport();

        gl()->disable(GL_CULL_FACE);

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
        // LongTaskFinder p("CompositorImplGL::clearColor", 1);
        gl()->clearColor(clr.R(), clr.G(), clr.B(), clr.A());
        gl()->clear(GL_COLOR_BUFFER_BIT);
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

    virtual void setFillColor(const Unit::Color& clr) override
    {
        m_state.back().color = clr;
    }

    virtual void punchHole(const Unit::Rect& rt) override
    {
        save();
        setFillColor(Unit::Color(0, 0, 0, 0));
        gl()->blendFunc(GL_ONE, GL_ZERO);
        drawRect(rt);
        updateBlendMode();
        restore();
    }

    virtual void drawRect(const Unit::Rect& rt) override
    {
        INSTALL_PROFILE_TIMER("CompositorGL::drawRect");
        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

        auto& lastState = m_state.back();
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

        auto currentColor = lastState.color;

        if (lastState.matrixStaysInRect &&
            lastState.abbreviatedClipPaths.size() == 0) {
            m_compositorContext->rectProgram();
            Unit::Rect drawRect = lastState.clipRect;
            drawRect.intersect(toRect(dest));

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
            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
            checkError(gl());
        } else {
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
                    gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    checkError(gl());
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
        std::vector<std::pair<N, N>> pointPerIndex;
        for (size_t i = 0; i < paths.size(); i++) {
            for (size_t j = 0; j < paths[i].size(); j++) {
                pointPerIndex.push_back({ i, j });
            }
            count += paths[i].size();
        }

        if (sizeof(N) != sizeof(size_t) &&
            count > std::numeric_limits<N>::max()) {
            STARFISH_LOG_ERROR("Too many vertices for drawTessellatedPolygon");
            return;
        }

        std::vector<N> indices = mapbox::earcut<N>(paths);

        std::vector<float> position;
        position.reserve((indices.size() / 3) * 6);

        // Use custom parameters if provided, otherwise use defaults
        const SkMatrix& screenMatrix =
            customScreenMatrix ? *customScreenMatrix : m_screenMatrix;
        size_t sw = customScreenWidth ? customScreenWidth : screenWidth();
        size_t sh = customScreenHeight ? customScreenHeight : screenHeight();

        size_t triangleCount = 0;
        for (size_t i = 0; i < indices.size(); i += 3) {
            const auto& p1 = pointPerIndex[indices[i]];
            const auto& p2 = pointPerIndex[indices[i + 1]];
            const auto& p3 = pointPerIndex[indices[i + 2]];

            float trianglePoints[6] = { (float)paths[p1.first][p1.second].x,
                                        (float)paths[p1.first][p1.second].y,
                                        (float)paths[p2.first][p2.second].x,
                                        (float)paths[p2.first][p2.second].y,
                                        (float)paths[p3.first][p3.second].x,
                                        (float)paths[p3.first][p3.second].y };

            // Map points using the appropriate screen matrix
            float x, y;
            x = trianglePoints[0];
            y = trianglePoints[1];
            mapPointsByMatrix(x, y, screenMatrix);
            trianglePoints[0] = x;
            trianglePoints[1] = y;

            x = trianglePoints[2];
            y = trianglePoints[3];
            mapPointsByMatrix(x, y, screenMatrix);
            trianglePoints[2] = x;
            trianglePoints[3] = y;

            x = trianglePoints[4];
            y = trianglePoints[5];
            mapPointsByMatrix(x, y, screenMatrix);
            trianglePoints[4] = x;
            trianglePoints[5] = y;

            float hw = 2.f / sw;
            float hh = -2.f / sh;
            position.push_back(trianglePoints[0] * hw - 1);
            position.push_back(trianglePoints[1] * hh + 1);
            position.push_back(trianglePoints[2] * hw - 1);
            position.push_back(trianglePoints[3] * hh + 1);
            position.push_back(trianglePoints[4] * hw - 1);
            position.push_back(trianglePoints[5] * hh + 1);
            triangleCount += 3;
        }

        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);
        gl()->vertexAttribPointer(
            m_compositorContext->m_polygonShaderProgramPosition, 2, GL_FLOAT,
            false, 0, position.data());
        gl()->enableVertexAttribArray(
            m_compositorContext->m_polygonShaderProgramPosition);

        gl()->uniform4f(m_compositorContext->m_polygonShaderProgramColor,
                        opacity * color.R(), opacity * color.G(),
                        opacity * color.B(), opacity * color.A());

        gl()->drawArrays(GL_TRIANGLES, 0, triangleCount);

        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);

        checkError(gl());
        gl()->disableVertexAttribArray(
            m_compositorContext->m_polygonShaderProgramPosition);

        if (drawOutline && g_isSupportStandardDerivatives) {
            drawPolygonOutlineAA(paths, color, opacity, customScreenMatrix,
                                 customScreenWidth, customScreenHeight);
        }
    }

    // Draw anti-aliased lines along polygon outline edges
    void drawPolygonOutlineAA(const Clipper2Lib::PathsD& paths,
                              const Unit::Color& color, float opacity,
                              const SkMatrix* customScreenMatrix = nullptr,
                              size_t customScreenWidth = 0,
                              size_t customScreenHeight = 0)
    {
        m_compositorContext->lineProgram();

        std::vector<float> position;
        std::vector<float> edgeDistances;

        // Line thickness in pixels (for anti-aliasing)
        float lineWidth = 1.0f;

        // Use custom parameters if provided, otherwise use defaults
        const SkMatrix& screenMatrix =
            customScreenMatrix ? *customScreenMatrix : m_screenMatrix;
        size_t sw = customScreenWidth ? customScreenWidth : screenWidth();
        size_t sh = customScreenHeight ? customScreenHeight : screenHeight();

        for (const auto& path : paths) {
            if (path.size() < 2)
                continue;

            for (size_t i = 0; i < path.size(); i++) {
                size_t nextIdx = (i + 1) % path.size();

                float x1 = (float)path[i].x;
                float y1 = (float)path[i].y;
                float x2 = (float)path[nextIdx].x;
                float y2 = (float)path[nextIdx].y;

                // Map points using the appropriate screen matrix
                mapPointsByMatrix(x1, y1, screenMatrix);
                mapPointsByMatrix(x2, y2, screenMatrix);

                // Calculate line direction
                float dx = x2 - x1;
                float dy = y2 - y1;
                float len = sqrt(dx * dx + dy * dy);
                if (len < 0.001f)
                    continue;

                // Normalize direction
                dx /= len;
                dy /= len;

                // Calculate normal (perpendicular) vector
                float nx = -dy;
                float ny = dx;

                // Extend line endpoints to cover corners
                float extend = lineWidth * 0.5f;
                float ex1 = x1 - dx * extend;
                float ey1 = y1 - dy * extend;
                float ex2 = x2 + dx * extend;
                float ey2 = y2 + dy * extend;

                float halfWidth = lineWidth * 0.5f;

                float hw = 2.f / sw;
                float hh = -2.f / sh;

                float v0x = (ex1 - nx * halfWidth) * hw - 1;
                float v0y = (ey1 - ny * halfWidth) * hh + 1;
                float v1x = (ex1 + nx * halfWidth) * hw - 1;
                float v1y = (ey1 + ny * halfWidth) * hh + 1;
                float v2x = (ex2 - nx * halfWidth) * hw - 1;
                float v2y = (ey2 - ny * halfWidth) * hh + 1;
                float v3x = (ex2 + nx * halfWidth) * hw - 1;
                float v3y = (ey2 + ny * halfWidth) * hh + 1;

                // Triangle 1: v0, v1, v2
                position.push_back(v0x);
                position.push_back(v0y);
                edgeDistances.push_back(0.0f);

                position.push_back(v1x);
                position.push_back(v1y);
                edgeDistances.push_back(lineWidth);

                position.push_back(v2x);
                position.push_back(v2y);
                edgeDistances.push_back(0.0f);

                // Triangle 2: v1, v3, v2
                position.push_back(v1x);
                position.push_back(v1y);
                edgeDistances.push_back(lineWidth);

                position.push_back(v3x);
                position.push_back(v3y);
                edgeDistances.push_back(lineWidth);

                position.push_back(v2x);
                position.push_back(v2y);
                edgeDistances.push_back(0.0f);
            }
        }

        if (position.empty())
            return;

        gl()->bindBuffer(GL_ARRAY_BUFFER, 0);

        // Position attribute
        gl()->vertexAttribPointer(
            m_compositorContext->m_lineShaderProgramPosition, 2, GL_FLOAT,
            false, 0, position.data());
        gl()->enableVertexAttribArray(
            m_compositorContext->m_lineShaderProgramPosition);

        // Edge distance attribute
        gl()->vertexAttribPointer(
            m_compositorContext->m_lineShaderProgramEdgeDistance, 1, GL_FLOAT,
            false, 0, edgeDistances.data());
        gl()->enableVertexAttribArray(
            m_compositorContext->m_lineShaderProgramEdgeDistance);

        // Set color
        gl()->uniform4f(m_compositorContext->m_lineShaderProgramColor,
                        opacity * color.R(), opacity * color.G(),
                        opacity * color.B(), opacity * color.A());

        // Set line width uniform
        gl()->uniform1f(m_compositorContext->m_lineShaderProgramLineWidth,
                        lineWidth);

        // Draw
        gl()->drawArrays(GL_TRIANGLES, 0, position.size() / 2);

        checkError(gl());
        gl()->disableVertexAttribArray(
            m_compositorContext->m_lineShaderProgramPosition);
        gl()->disableVertexAttribArray(
            m_compositorContext->m_lineShaderProgramEdgeDistance);
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
                             size_t textureHeight)
    {
        auto& lastState = m_state.back();

        // Use FBO in order to 2-pass blur
        pushFBOContext(textureWidth, textureHeight, false,
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

            gl()->enableVertexAttribArray(*positionPos);
            gl()->enableVertexAttribArray(*texIdx);

            gl()->uniform2fv(*positionPos, 4, position);

            gl()->uniform1f(*width, textureWidth);
            gl()->uniform1f(*height, textureHeight);
            gl()->uniform2f(*blurRadius, blurMainRadius, blurSubRadius);

            gl()->activeTexture(GL_TEXTURE0);
            gl()->bindTexture(textureKind, textureID);

            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);

            gl()->disableVertexAttribArray(*positionPos);
            gl()->disableVertexAttribArray(*texIdx);
        }

        GLuint fboTex = popFBOContext();
        checkError(gl());

        if (isScissorEnabled) {
            gl()->enable(GL_SCISSOR_TEST);
        }

        // blur H
        {
            m_compositorContext->texBlurShaderProgramH();

            gl()->enableVertexAttribArray(
                m_compositorContext->m_texBlurShaderProgramHTexPos);
            gl()->enableVertexAttribArray(
                m_compositorContext->m_texBlurShaderProgramHTexIdx);

            gl()->uniform2fv(
                m_compositorContext->m_texBlurShaderProgramHPosition, 4,
                position);

            gl()->activeTexture(GL_TEXTURE0);
            gl()->bindTexture(GL_TEXTURE_2D, fboTex);

            gl()->uniform1f(
                m_compositorContext->m_texBlurShaderProgramHTextureWidth,
                textureWidth);
            gl()->uniform1f(
                m_compositorContext->m_texBlurShaderProgramHTextureHeight,
                textureHeight);

            gl()->uniform2f(
                m_compositorContext->m_texBlurShaderProgramHBlurRadius,
                blurSubRadius, blurMainRadius);
            float a = lastState.opacity;
            if (a != 1) {
                gl()->uniform1f(
                    m_compositorContext->m_texBlurShaderProgramHAlpha, a);
            }
            if (UNLIKELY(cs->isFlipYNeeded())) {
                m_compositorContext->bindTexPos(
                    m_compositorContext->m_texBlurShaderProgramHTexPos, true);
            }

            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);

#if !defined(NDEBUG)
            if (gl()->getError() == 1286) {
                STARFISH_LOG_ERROR("drawFilteredTexture got error 1286");
            }
#endif

            gl()->disableVertexAttribArray(
                m_compositorContext->m_texBlurShaderProgramHTexPos);
            gl()->disableVertexAttribArray(
                m_compositorContext->m_texBlurShaderProgramHTexIdx);

            if (a != 1) {
                gl()->uniform1f(
                    m_compositorContext->m_texBlurShaderProgramHAlpha, 1);
            }
            if (UNLIKELY(cs->isFlipYNeeded())) {
                m_compositorContext->bindTexPos(
                    m_compositorContext->m_texBlurShaderProgramHTexPos, false);
            }
            checkError(gl());
        }

        m_compositorContext->putGenericTextureToCache(fboTex, textureWidth,
                                                      textureHeight);
        checkError(gl());
    }

    void drawTexture(CanvasSurfaceGL* cs, float position[8], GLuint textureID,
                     GLenum textureKind, GLenum textureBindNumber,
                     size_t textureWidth, size_t textureHeight)
    {
        auto& lastState = m_state.back();
        if (lastState.blurRadius) {
            drawFilteredTexture(cs, position, textureID, textureKind,
                                textureBindNumber, textureWidth, textureHeight);
            return;
        }
        bool isEGLImage = textureKind != GL_TEXTURE_2D;
        if (isEGLImage) {
            m_webView->renderer()->mayNeedsSync();
            m_compositorContext->texShaderProgramEGLImageExternal();
        } else {
            m_compositorContext->texShaderProgram();
        }

        gl()->activeTexture(GL_TEXTURE0);
        gl()->bindTexture(textureKind, textureID);

        GLint* positionPos;
        GLint* alphaPos;
        GLint* texPos;
        GLint* texIdx;

        float a = lastState.opacity;
        if (isEGLImage) {
            positionPos = &m_compositorContext
                               ->m_texShaderProgramEGLImageExternalPosition;
            alphaPos =
                &m_compositorContext->m_texShaderProgramEGLImageExternalAlpha;
            texPos =
                &m_compositorContext->m_texShaderProgramEGLImageExternalTexPos;
            texIdx =
                &m_compositorContext->m_texShaderProgramEGLImageExternalTexIdx;
        } else {
            positionPos = &m_compositorContext->m_texShaderProgramPosition;
            alphaPos = &m_compositorContext->m_texShaderProgramAlpha;
            texPos = &m_compositorContext->m_texShaderProgramTexPos;
            texIdx = &m_compositorContext->m_texShaderProgramTexIdx;
        }

        gl()->enableVertexAttribArray(*texPos);
        gl()->enableVertexAttribArray(*texIdx);

        gl()->uniform2fv(*positionPos, 4, position);

        if (a != 1) {
            gl()->uniform1f(*alphaPos, a);
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
                                float (&position)[8])
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

        bool scissorClippingEnabled = false;
        bool alphaTextureClippingEnabled = false;
        bool shouldSkipTexturePainting = false;
        Unit::Rect visibleArea =
            Unit::Rect(0, 0, screenWidth(), screenHeight());
        float diffXDueToAlphaTextureCliping = 0;
        float diffYDueToAlphaTextureCliping = 0;

        SkMatrix ctm = lastState.matrix;
        SkMatrix screenMatrix = m_screenMatrix;
        size_t screenWidth = this->screenWidth();
        size_t screenHeight = this->screenHeight();

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

        if (lastState.abbreviatedClipPaths.size() == 0 &&
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
                    for (size_t i = 0; i < result.size(); i++) {
                        visibleArea.unite(boundingRect(result[i]));
                    }

                    diffXDueToAlphaTextureCliping = -visibleArea.x();
                    diffYDueToAlphaTextureCliping = -visibleArea.y();
                    alphaTextureClippingEnabled = true;

                    pushFBOContext(visibleArea.width(), visibleArea.height(),
                                   false,
                                   LayoutRect(0, 0, visibleArea.width(),
                                              visibleArea.height()));

                    screenWidth = visibleArea.width();
                    screenHeight = visibleArea.height();

                    ctm.postTranslate(diffXDueToAlphaTextureCliping,
                                      diffYDueToAlphaTextureCliping);

                    visibleArea.setX(0);
                    visibleArea.setY(0);

                    gl()->clearColor(0, 0, 0, 0);
                    gl()->clear(GL_COLOR_BUFFER_BIT);

                    // reset screen matrix while draw fbo on screen
                    screenMatrix.reset();

                    // Draw clipping polygon with white color to create
                    // alpha mask Using drawTessellatedPolygon for
                    // anti-aliased edges
                    for (auto& path : result) {
                        for (auto& pt : path) {
                            pt.x += diffXDueToAlphaTextureCliping;
                            pt.y += diffYDueToAlphaTextureCliping;
                        }
                    }
                    drawTessellatedPolygon(
                        result, Unit::Color(255, 255, 255, 255), 1.0f, true,
                        &screenMatrix, screenWidth, screenHeight);

                    // Set blending to use alpha mask: result = src *
                    // dst_alpha
                    gl()->blendFunc(GL_DST_ALPHA, GL_ZERO);
                    gl()->blendEquation(GL_FUNC_ADD);
                }
            } else {
                shouldSkipTexturePainting = true;
            }
        }

        if (!shouldSkipTexturePainting) {
            if (csGL->m_isEGLImageExternal) {
                float eglDest[4][2] = {
                    { dest[0][0] + diffXDueToAlphaTextureCliping,
                      dest[0][1] + diffYDueToAlphaTextureCliping },
                    { dest[1][0] + diffXDueToAlphaTextureCliping,
                      dest[1][1] + diffYDueToAlphaTextureCliping },
                    { dest[2][0] + diffXDueToAlphaTextureCliping,
                      dest[2][1] + diffYDueToAlphaTextureCliping },
                    { dest[3][0] + diffXDueToAlphaTextureCliping,
                      dest[3][1] + diffYDueToAlphaTextureCliping }
                };
                Unit::Rect screenBoundingRect = toRect(eglDest);
                if (screenBoundingRect.intersects(visibleArea)) {
                    float texPosition[8];
                    computeTexturePosition(dst, ctm, screenMatrix, screenWidth,
                                           screenHeight, texPosition);
                    drawTexture(csGL, texPosition,
                                csGL->m_textureFragments[0].textureID,
#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
                                GL_TEXTURE_2D, -1,
#else
                                GL_TEXTURE_EXTERNAL_OES, -1,
#endif
                                csGL->m_bufferWidth, csGL->m_bufferHeight);
                }

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
                                computeTexturePosition(
                                    newDst, ctm, screenMatrix, screenWidth,
                                    screenHeight, texPosition);
                                drawTexture(csGL, texPosition, tid,
                                            GL_TEXTURE_2D, GL_TEXTURE0,
                                            texureDataWidth, texureDataHeight);
                            }
                        }
                        i++;
                        coveredColsCount += csGL->textureTileSize();
                    }

                    coveredRowsCount += csGL->textureTileSize();
                }
            }
        }

        if (alphaTextureClippingEnabled) {
            // Restore blend mode before drawing FBO texture to screen
            updateBlendMode();

            GLuint fboTex = popFBOContext();

            m_compositorContext->texShaderProgram();

            float dest[4][2]; // order is LB, LT, RB, RT
            dest[0][0] = visibleArea.x() - diffXDueToAlphaTextureCliping;
            dest[0][1] = visibleArea.maxY() - diffYDueToAlphaTextureCliping;
            dest[1][0] = visibleArea.x() - diffXDueToAlphaTextureCliping;
            dest[1][1] = visibleArea.y() - diffYDueToAlphaTextureCliping;
            dest[2][0] = visibleArea.maxX() - diffXDueToAlphaTextureCliping;
            dest[2][1] = visibleArea.maxY() - diffYDueToAlphaTextureCliping;
            dest[3][0] = visibleArea.maxX() - diffXDueToAlphaTextureCliping;
            dest[3][1] = visibleArea.y() - diffYDueToAlphaTextureCliping;

            screenMatrix = m_screenMatrix;

            mapPointsByMatrix(dest[0][0], dest[0][1], screenMatrix);
            mapPointsByMatrix(dest[1][0], dest[1][1], screenMatrix);
            mapPointsByMatrix(dest[2][0], dest[2][1], screenMatrix);
            mapPointsByMatrix(dest[3][0], dest[3][1], screenMatrix);

            float hw = 2.f / this->screenWidth();
            float hh = -2.f / this->screenHeight();

            float position[8];
            position[0] = dest[0][0] * hw - 1;
            position[1] = dest[0][1] * hh + 1;

            position[2] = dest[1][0] * hw - 1;
            position[3] = dest[1][1] * hh + 1;

            position[4] = dest[2][0] * hw - 1;
            position[5] = dest[2][1] * hh + 1;

            position[6] = dest[3][0] * hw - 1;
            position[7] = dest[3][1] * hh + 1;

            // Alpha mask position: map screen coordinates to FBO texture
            // coordinates The FBO covers visibleArea, so we need to map screen
            // position to [0,1] range relative to visibleArea
            float alphaMaskPosition[8];

            // Map visibleArea corners to FBO NDC coordinates
            // The FBO texture coordinates should match vTexPos from bindTexPos
            // (flipY=false) vTexPos order: (0,0), (0,1), (1,0), (1,1) for LB,
            // LT, RB, RT NDC (-1,-1) maps to texture coord (0,0), NDC (1,1)
            // maps to texture coord (1,1) So we need to map each corner
            // correctly: LB (aTexIdx=0): vTexPos=(0,0) -> NDC (-1,-1) LT
            // (aTexIdx=1): vTexPos=(0,1) -> NDC (-1,1) RB (aTexIdx=2):
            // vTexPos=(1,0) -> NDC (1,-1) RT (aTexIdx=3): vTexPos=(1,1) -> NDC
            // (1,1)
            alphaMaskPosition[0] = -1.0f; // LB -> NDC (-1, -1)
            alphaMaskPosition[1] = -1.0f;

            alphaMaskPosition[2] = -1.0f; // LT -> NDC (-1, 1)
            alphaMaskPosition[3] = 1.0f;

            alphaMaskPosition[4] = 1.0f; // RB -> NDC (1, -1)
            alphaMaskPosition[5] = -1.0f;

            alphaMaskPosition[6] = 1.0f; // RT -> NDC (1, 1)
            alphaMaskPosition[7] = 1.0f;

            gl()->activeTexture(GL_TEXTURE0);
            gl()->bindTexture(GL_TEXTURE_2D, fboTex);
            checkError(gl());

            gl()->enableVertexAttribArray(
                m_compositorContext->m_texShaderProgramTexPos);
            gl()->enableVertexAttribArray(
                m_compositorContext->m_texShaderProgramTexIdx);

            gl()->uniform2fv(m_compositorContext->m_texShaderProgramPosition, 4,
                             position);

            gl()->drawArrays(GL_TRIANGLE_STRIP, 0, 4);

            gl()->disableVertexAttribArray(
                m_compositorContext->m_texShaderProgramTexPos);
            gl()->disableVertexAttribArray(
                m_compositorContext->m_texShaderProgramTexIdx);

            m_compositorContext->putGenericTextureToCache(
                fboTex, visibleArea.width(), visibleArea.height());
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
        applyDevicePixelRatio();
    }

    virtual void resetClip()
    {
        auto& lastState = m_state.back();
        lastState.clipRect = Unit::Rect(0, 0, screenWidth(), screenHeight());
        lastState.abbreviatedClipPaths.clear();
        lastState.pathCommands.clear();
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
    virtual void clipPath() override
    {
        auto& lastState = m_state.back();
        if (m_abbreviatedPath.size()) {
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
        GLuint fboId;
        GLuint fboTex;
        GLuint fboSupportTex;
        GLuint renderBufferId;
        LayoutRect viewport;
    };

    void pushFBOContext(size_t width, size_t height, bool needsStencilDepth,
                        LayoutRect viewport)
    {
        m_seenFBOUsage = true;

        FBOState newFBOState;

        // generate FBO
        gl()->genFramebuffers(1, &newFBOState.fboId);
        checkError(gl());

        // generate texture
        newFBOState.fboTex =
            m_compositorContext->takeGenericTextureFromCache(width, height);
        if (newFBOState.fboTex == 0) {
            gl()->genTextures(1, &newFBOState.fboTex);
            checkError(gl());
        }

        // generate render buffer
        gl()->genRenderbuffers(1, &newFBOState.renderBufferId);
        checkError(gl());

        // Bind Frame buffer
        gl()->bindFramebuffer(GL_FRAMEBUFFER, newFBOState.fboId);
        checkError(gl());

        // Bind texture
        gl()->bindTexture(GL_TEXTURE_2D, newFBOState.fboTex);
        checkError(gl());

        // Define texture parameters
        gl()->texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, nullptr);
        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl()->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        checkError(gl());

        // Bind render buffer and define buffer dimension
        gl()->bindRenderbuffer(GL_RENDERBUFFER, newFBOState.renderBufferId);

        // Attach texture FBO color attachment
        gl()->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, newFBOState.fboTex, 0);
        checkError(gl());

        if (needsStencilDepth) {
            gl()->genTextures(1, &newFBOState.fboSupportTex);
            gl()->bindTexture(GL_TEXTURE_2D, newFBOState.fboSupportTex);
            gl()->texImage2D(
                GL_TEXTURE_2D, 0,                // target and mipmap level
                GL_DEPTH_STENCIL, width, height, // size of texture
                0,                               // border size
                GL_DEPTH_STENCIL,     // format of of data we are uploading
                                      // to to the texture (ignored)
                GL_UNSIGNED_INT_24_8, // type of of data we are
                                      // uploading to to the texture
                                      // (ignored)
                NULL /* no data uploaded */);
            checkError(gl());
            // attatch the depth/stencil texture to both the stencil and depth
            // render objects.
            gl()->framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                       GL_TEXTURE_2D, newFBOState.fboSupportTex,
                                       0);
            gl()->framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                       GL_TEXTURE_2D, newFBOState.fboSupportTex,
                                       0);
            checkError(gl());
        } else {
            newFBOState.fboSupportTex = 0;
        }

        newFBOState.viewport = viewport;
        gl()->viewport(viewport.x(), viewport.y(), viewport.width(),
                       viewport.height());

        m_fboState.push_back(newFBOState);
    }

    GLuint popFBOContext() // returns texture
    {
        FBOState lastState = m_fboState.back();
        m_fboState.pop_back();

        if (m_fboState.size()) {
            auto& s = m_fboState.back();
            gl()->bindRenderbuffer(GL_RENDERBUFFER, s.renderBufferId);
            gl()->bindFramebuffer(GL_FRAMEBUFFER, s.fboId);

            gl()->viewport(s.viewport.x(), s.viewport.y(), s.viewport.width(),
                           s.viewport.height());
        } else {
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
            if (m_compositorContext->m_mainViewRBO) {
                gl()->bindRenderbuffer(GL_RENDERBUFFER,
                                       m_compositorContext->m_mainViewRBO);
            }
            if (m_compositorContext->m_mainViewFBO) {
                gl()->bindFramebuffer(GL_FRAMEBUFFER,
                                      m_compositorContext->m_mainViewFBO);
            }
#else
            gl()->bindRenderbuffer(GL_RENDERBUFFER, 0);
            gl()->bindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
            setViewport();
        }

        gl()->deleteRenderbuffers(1, &lastState.renderBufferId);
        gl()->deleteFramebuffers(1, &lastState.fboId);
        if (lastState.fboSupportTex) {
            gl()->deleteTextures(1, &lastState.fboSupportTex);
        }
        return lastState.fboTex;
    }

protected:
    bool m_seenFBOUsage;
    float m_globalScale;
    size_t m_screenWidth;
    size_t m_screenHeight;
    WebView* m_webView;
    CompositorContextGL* m_compositorContext;
    std::vector<CompositorImplGLState> m_state;
    std::vector<FBOState> m_fboState;

    Clipper2Lib::PathD m_abbreviatedPath;
    std::vector<CompositorImplGLState::PathCommand> m_pathCommands;
    SkMatrix m_screenMatrix;
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
