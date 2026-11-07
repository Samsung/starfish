/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "core/dom/canvas/webgl/WebGLRenderingContext.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/canvas/ImageData.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/ImageBitmap.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/canvas/webgl/WebGLExtensions.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/image/SVGNativeImageData.h"
#include "core/util/debug/Trace.h"
#include "core/util/String.h"
#include "core/dom/canvas/webgl/gl/GLContext.h"
#include "core/dom/canvas/webgl/WebGLActiveInfo.h"
#include "core/dom/canvas/webgl/WebGLBuffer.h"
#include "core/dom/canvas/webgl/WebGLShader.h"
#include "core/dom/canvas/webgl/WebGLProgram.h"
#include "core/dom/canvas/webgl/WebGLTexture.h"
#include "core/dom/canvas/webgl/WebGLFramebuffer.h"
#include "core/dom/canvas/webgl/WebGLRenderbuffer.h"
#include "core/dom/canvas/webgl/WebGLUniformLocation.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2D.h"
#include "core/dom/canvas/webgl/WebGLRenderingContextState.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/generated/Float32ArrayOrSequenceOfGLfloatUnion.h"
#include "binding/generated/Int32ArrayOrSequenceOfGLintUnion.h"
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include "binding/generated/ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElementUnion.h"
#include <regex>
#include <array>
#include <limits>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>
#include <unordered_map>
#include "core/dom/canvas/webgl/WebGLOES_VertexArrayObject.h"
#include "core/dom/canvas/webgl/WebGLShaderPrecisionFormat.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"

#include "platform/canvas/gl/IncludeGL.h"
#include "platform/canvas/gl/GL.h"
#include "core/dom/DOMException.h"
#include "core/page/WebBase.h"
#include "PlatformIntegrationData.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "platform/multimedia/MediaPlayer.h"
#endif

#include <EscargotPublic.h>
#include <sstream>
#include <iomanip>

#define kMaximumUniformAndAttributeLocationLengths 256
#define kMaximumSupportedStride 255

/* WebGL-specific enums */
static const GLenum kUNPACK_FLIP_Y_WEBGL = 0x9240;
static const GLenum kUNPACK_PREMULTIPLY_ALPHA_WEBGL = 0x9241;
static const GLenum kCONTEXT_LOST_WEBGL = 0x9242;
static const GLenum kUNPACK_COLORSPACE_CONVERSION_WEBGL = 0x9243;
static const GLenum kBROWSER_DEFAULT_WEBGL = 0x9244;
static const GLenum kIMPLEMENTATION_COLOR_READ_TYPE = 0x8B9A;
static const GLenum kIMPLEMENTATION_COLOR_READ_FORMAT = 0x8B9B;

/* WebGL constants */
static constexpr char kShadingLanguageVersion[] = "WebGL GLSL ES 1.0";
static constexpr char kVersion[] = "WebGL 1.0";

namespace Starfish {

inline static std::string hex(GLenum name)
{
    return StringUtils::formatString("0x%04X", name);
}

WebGLRenderingContext::WebGLRenderingContext(HTMLCanvasElement* canvasElement)
    : WebGLRenderingContextBaseMixIn(canvasElement)
{
    m_unpackFlipY = false;
    m_unpackPremultiplyAlpha = false;
    m_unpackColorspaceConversion = kBROWSER_DEFAULT_WEBGL;
    m_activeTextureUnit = GL_TEXTURE0;
    m_isContextLost = false;
    m_hasPendingJobsBetweenFrames = false;
    m_unpackColorSpace = String::createASCIIString("srgb");
    m_drawingBufferColorSpace = String::createASCIIString("srgb");
    m_state = new WebGLRenderingContextState();
    m_gl = m_ownerHTMLCanvasElement->webView()->renderer()->gl();
}

void WebGLRenderingContext::releaseOutOfHeapMembers()
{
    GLErrorSet().swap(m_GLErrors);
    GLTextureMap().swap(m_boundTextures);
}

WebGLRenderingContext::~WebGLRenderingContext()
{
}

ScriptBindingInstance* WebGLRenderingContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void WebGLRenderingContext::preInitialize(ScriptValue contextAttributes)
{
    STARFISH_ASSERT(m_isContextAttributesChecked == false);

    // Check context attributes
    m_isContextAttributesChecked = true;

    if (contextAttributes->isObject()) {
#define SET_ATTRIBUTE(str, attribute)                           \
    {                                                           \
        auto key = StringRef::createFromASCII(#str);            \
        if (object->has(state, key)) {                          \
            ValueRef* value = object->get(state, key);          \
            if (value->isBoolean()) {                           \
                STARFISH_LOG_INFO("%s : %d", #str,              \
                                  value->asBoolean() ? 1 : 0);  \
                attributes->set##attribute(value->asBoolean()); \
            }                                                   \
        }                                                       \
    }

        Evaluator::EvaluatorResult evaluated = Evaluator::execute(
            scriptBindingInstance()->scriptContext(),
            [](ExecutionStateRef* state, ObjectRef* object,
               WebGLContextAttributes* attributes) -> ValueRef* {
                // powerPreference: string
                auto key = StringRef::createFromASCII("powerPreference");
                if (object->has(state, key)) {
                    ValueRef* value = object->get(state, key);
                    if (value->isString()) {
                        std::string powerPreference =
                            value->asString()->toStdUTF8String();
                        STARFISH_LOG_INFO("powerPreference : %s",
                                          powerPreference.c_str());
                        attributes->setPowerPreference(String::fromUTF8(
                            powerPreference.c_str(), powerPreference.length()));
                    }
                }

                // the others: bool
                SET_ATTRIBUTE(alpha, Alpha);
                SET_ATTRIBUTE(depth, Depth);
                SET_ATTRIBUTE(stencil, Stencil);
                SET_ATTRIBUTE(antialias, Antialias);
                SET_ATTRIBUTE(premultipliedAlpha, PremultipliedAlpha);
                SET_ATTRIBUTE(preserveDrawingBuffer, PreserveDrawingBuffer);
                SET_ATTRIBUTE(desynchronized, Desynchronized);
                SET_ATTRIBUTE(failIfMajorPerformanceCaveat,
                              FailIfMajorPerformanceCaveat);
                return ValueRef::createUndefined();
            },
            contextAttributes->asObject(), &m_attributes);

#undef SET_ATTRIBUTE
        m_frameBufferAttributes.alpha = m_attributes.alpha();
        m_frameBufferAttributes.antialias = m_attributes.antialias();
        m_frameBufferAttributes.depth = m_attributes.depth();
        m_frameBufferAttributes.stencil = m_attributes.stencil();
    }

    // In WebGL 1.0 antialias is only a request, and getContextAttributes()
    // reports the drawing buffer actually created (§5.2), which is never
    // multisampled. Deviation: WebGL 2.0 must obey the request (§3.2), but a
    // multisampled default framebuffer is not implemented, so a WebGL 2
    // context reports the requested value while its drawing buffer stays
    // single-sampled (SAMPLES 0).
    if (webGLVersion() == 1) {
        m_attributes.setAntialias(false);
    }
}

void WebGLRenderingContext::initialize()
{
    WebGLRenderingContextBaseMixIn::initialize();

    STARFISH_ASSERT(m_canvasSurface != nullptr);

    // 2.3 The WebGL Viewport
    //
    // Upon creation of WebGL context context, the viewport is initialized to a
    // rectangle with origin at (0, 0) and width and height equal to
    // (context.drawingBufferWidth, context.drawingBufferHeight).

    viewport(0, 0, drawingBufferWidth(), drawingBufferHeight());
    scissor(0, 0, drawingBufferWidth(), drawingBufferHeight());

    // bind default frame buffer
    GLContextScope contextScope(m_context);

    // The registry reads GL_EXTENSIONS, so it can only be built while a
    // context is current. The constructor runs before m_context exists, and
    // on ports where nothing is current at that point (ANGLE on Windows)
    // getString returned null there and left the registry empty -- every
    // getExtension() answered null and getSupportedExtensions() was empty.
    if (!WebGLExtensionRegistry::instance().isInitialized()) {
        WebGLExtensionRegistry::instance().initialize(m_gl);
    }

    m_gl->bindFramebuffer(GL_FRAMEBUFFER, m_framebufferTexture->fbo());

    // WebGL 2.0, "PRIMITIVE_RESTART_FIXED_INDEX is always enabled"; the cap
    // is not exposed to enable() or disable().
    if (isWebGL2RenderingContext()) {
        m_gl->enable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
}

void WebGLRenderingContext::flushDrawingCommands()
{
    GLRevertableContextScope scope(
        m_context, executionContext()->webBase()->asWebView()->renderer());
    // we need to bind 0(screen) buffer for sending commands to gpu
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, getCurrentFBO());
}

void WebGLRenderingContext::flushForReadback()
{
    if (!m_canvasSurface) {
        return;
    }

    WebGLRenderingContextBaseMixIn::flushForReadback();

    flushDrawingCommands();

    GLRevertableContextScope scope(
        m_context, executionContext()->webBase()->asWebView()->renderer());

    size_t width = m_canvasSurface->bufferWidth();
    size_t height = m_canvasSurface->bufferHeight();
    size_t stride = m_canvasSurface->bufferStride();
    STARFISH_ASSERT(stride % 4 == 0 && stride >= width * 4);

    uint8_t* buffer = m_canvasSurface->mapBuffer();
    readDrawingBuffer(buffer, stride);

#if defined(PORT_PIXEL_ORDER_BGRA)
    for (size_t row = 0; row < height; ++row) {
        uint8_t* p = buffer + row * stride;
        for (size_t col = 0; col < width; ++col) {
            std::swap(p[col * 4 + 0], p[col * 4 + 2]);
        }
    }
#endif
}

void WebGLRenderingContext::readDrawingBuffer(uint8_t* pixels, size_t stride)
{
    GLint prevReadFbo = 0;
    m_gl->getIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
    m_gl->bindFramebuffer(GL_READ_FRAMEBUFFER, m_framebufferTexture->fbo());
    // A WebGL 2 page may have set the default framebuffer's read buffer to
    // NONE; that limits the page's own reads, not the canvas's readback.
    GLint prevReadBuffer = GL_COLOR_ATTACHMENT0;
    if (isWebGL2RenderingContext()) {
        m_gl->getIntegerv(GL_READ_BUFFER, &prevReadBuffer);
        if (prevReadBuffer == GL_NONE) {
            m_gl->readBuffer(GL_COLOR_ATTACHMENT0);
        }
    }

    size_t width = m_canvasSurface->bufferWidth();
    size_t height = m_canvasSurface->bufferHeight();
    STARFISH_ASSERT(stride % 4 == 0 && stride >= width * 4);

    GLint prevPackAlignment = 4;
    GLint prevPackRowLength = 0;
    GLint prevPackSkipPixels = 0;
    GLint prevPackSkipRows = 0;
    GLint prevPixelPackBuffer = 0;
    m_gl->getIntegerv(GL_PACK_ALIGNMENT, &prevPackAlignment);
    m_gl->getIntegerv(GL_PACK_ROW_LENGTH, &prevPackRowLength);
    m_gl->getIntegerv(GL_PACK_SKIP_PIXELS, &prevPackSkipPixels);
    m_gl->getIntegerv(GL_PACK_SKIP_ROWS, &prevPackSkipRows);
    m_gl->getIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prevPixelPackBuffer);
    m_gl->pixelStorei(GL_PACK_ALIGNMENT, 1);
    m_gl->pixelStorei(GL_PACK_ROW_LENGTH, static_cast<GLint>(stride / 4));
    m_gl->pixelStorei(GL_PACK_SKIP_PIXELS, 0);
    m_gl->pixelStorei(GL_PACK_SKIP_ROWS, 0);
    m_gl->bindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    m_gl->readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    m_gl->pixelStorei(GL_PACK_ALIGNMENT, prevPackAlignment);
    m_gl->pixelStorei(GL_PACK_ROW_LENGTH, prevPackRowLength);
    m_gl->pixelStorei(GL_PACK_SKIP_PIXELS, prevPackSkipPixels);
    m_gl->pixelStorei(GL_PACK_SKIP_ROWS, prevPackSkipRows);
    m_gl->bindBuffer(GL_PIXEL_PACK_BUFFER,
                     static_cast<GLuint>(prevPixelPackBuffer));

    if (prevReadBuffer == GL_NONE) {
        m_gl->readBuffer(GL_NONE);
    }
    m_gl->bindFramebuffer(GL_READ_FRAMEBUFFER,
                          static_cast<GLuint>(prevReadFbo));

    for (size_t top = 0; top < height / 2; ++top) {
        size_t bot = height - 1 - top;
        uint8_t* pTop = pixels + top * stride;
        uint8_t* pBot = pixels + bot * stride;
        std::swap_ranges(pTop, pTop + width * 4, pBot);
    }
}

// A presented drawing buffer without preserveDrawingBuffer reads cleared
// (WebGL 1.0 §2.2), but it stays on screen until the page draws again, so a
// reader gets cleared pixels and the next WebGL call clears the buffer itself
// (completePendingJobs()).
bool WebGLRenderingContext::readClearedDrawingBuffer(uint8_t* pixels,
                                                     size_t stride)
{
    if (!m_hasPendingJobsBetweenFrames ||
        m_attributes.preserveDrawingBuffer()) {
        return false;
    }
    const size_t width = m_canvasSurface->bufferWidth();
    const size_t height = m_canvasSurface->bufferHeight();
    for (size_t row = 0; row < height; ++row) {
        uint8_t* p = pixels + row * stride;
        memset(p, 0, width * 4);
        if (!m_attributes.alpha()) {
            for (size_t col = 0; col < width; ++col) {
                p[col * 4 + 3] = 255;
            }
        }
    }
    return true;
}

Optional<NativeImageData*> WebGLRenderingContext::copyDrawingBuffer()
{
    // A context whose document is gone has no drawing buffer left.
    if (!m_canvasSurface) {
        return Optional<NativeImageData*>();
    }

    // shortcut: the drawing buffer size stands for the canvas size; they
    // differ only for a clamped canvas.
    const size_t width = m_canvasSurface->bufferWidth();
    const size_t height = m_canvasSurface->bufferHeight();
    BufferedNativeImageData* image =
        BufferedNativeImageData::create(width, height);
    // Cleared pixels are the same in every pixel order and alpha form.
    if (readClearedDrawingBuffer(image->data(), image->stride())) {
        return Optional<NativeImageData*>(image);
    }

    GLRevertableContextScope scope(
        m_context, executionContext()->webBase()->asWebView()->renderer());
    // Fail closed: reading with another context current would copy that
    // context's framebuffer. The read follows this context's own draws, so
    // nothing needs flushing first.
    if (!m_context.setCurrent()) {
        delete image;
        return Optional<NativeImageData*>();
    }
    readDrawingBuffer(image->data(), image->stride());

    // A 2D canvas composites premultiplied colors.
    if (m_attributes.alpha() && !m_attributes.premultipliedAlpha()) {
        convertImageBufferAsPremultipliedAlphaIfNeeds(image->data(), width,
                                                      image->stride(), height);
    }
#if defined(PORT_PIXEL_ORDER_BGRA)
    for (size_t row = 0; row < height; ++row) {
        uint8_t* p = image->data() + row * image->stride();
        for (size_t col = 0; col < width; ++col) {
            std::swap(p[col * 4 + 0], p[col * 4 + 2]);
        }
    }
#endif
    return Optional<NativeImageData*>(image);
}

void WebGLRenderingContext::flushForCompositing()
{
    // Unlike other rendering contexts, do not flushForReadback() here.
    // The compositor consumes the GPU texture directly, and the CPU
    // readback in flushForReadback() is too expensive to run every frame.
    flushDrawingCommands();

    // NOTE: According to the specification, by default the contents of
    // the drawing buffer shall be cleared to their default values after
    // compositing.
    // However, as of now, it's difficult to know when compositing and GL
    // rendering actually end, so we leave the clearing as a pending job
    // and let it be done lazily. The assumption here is that the engine will
    // invoke this `flushForCompositing()` every frame after we call
    // `setNeedsComposite()`.

    m_hasPendingJobsBetweenFrames = true;
}

void WebGLRenderingContext::onResize()
{
    WebGLRenderingContextBaseMixIn::onResize();

    // Resizing replaces only the drawing buffer; the page's framebuffer
    // bindings must survive it (WebGL 1.0 §2.2), but the default framebuffer
    // has a new GL name, so rebind what each target refers to.
    GLContextScope contextScope(m_context);
    for (GLenum target : { GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER }) {
        Optional<WebGLFramebuffer*> framebuffer = getBoundFramebuffer(target);
        m_gl->bindFramebuffer(target, framebuffer
                                          ? framebuffer.value()->glObject()
                                          : m_framebufferTexture->fbo());
    }
}

#define ENTER_CONTEXT_SCOPE_IMPL(bailoutValue, ...) \
    GLContextScope contextScope_(m_context);        \
    if (contextScope_.hasError()) {                 \
        TRACE(WEBGL,                                \
              "\033[33m"                            \
              "GL Context error detected."          \
              "\033[0m");                           \
        return bailoutValue;                        \
    }

#if defined(NDEBUG) and !defined(ENABLE_TRACE)
#define ENTER_CONTEXT_SCOPE(bailoutValue, ...) \
    ENTER_CONTEXT_SCOPE_IMPL(bailoutValue);
#else
#define ENTER_CONTEXT_SCOPE(bailoutValue, ...)       \
    ENTER_CONTEXT_SCOPE_IMPL(bailoutValue);          \
    auto onScopeLeave = OnScopeLeave::create([&]() { \
        if (hasNewGLError()) {                       \
            TRACE(WEBGL,                             \
                  "\033[33m"                         \
                  "GL error detected."               \
                  "\033[0m");                        \
        }                                            \
    });
#endif

GLenum WebGLRenderingContext::getError()
{
    GLContextScope contextScope(m_context);

    hasNewGLError();

    GLenum code = GL_NO_ERROR;
    if (!m_GLErrors.empty()) {
        code = *m_GLErrors.begin();
        m_GLErrors.erase(m_GLErrors.begin());
    }
    return code;
}

GLsizei WebGLRenderingContext::drawingBufferWidth() const
{
    return m_canvasSurface->bufferWidth();
}

GLsizei WebGLRenderingContext::drawingBufferHeight() const
{
    return m_canvasSurface->bufferHeight();
}

static bool isPredefinedColorSpace(String* value)
{
    STARFISH_ASSERT(value != nullptr);
    // Refs: https://html.spec.whatwg.org/multipage/canvas.html#2dcontext
    // enum PredefinedColorSpace { "srgb", "display-p3" };
    if (value->equals("srgb") || value->equals("display-p3")) {
        return true;
    }
    return false;
}

String* WebGLRenderingContext::drawingBufferColorSpace()
{
    return m_drawingBufferColorSpace;
}

void WebGLRenderingContext::setDrawingBufferColorSpace(String* value)
{
    if (isPredefinedColorSpace(value)) {
        m_drawingBufferColorSpace = value;
    }
}

String* WebGLRenderingContext::unpackColorSpace()
{
    return m_unpackColorSpace;
}

void WebGLRenderingContext::setUnpackColorSpace(String* value)
{
    if (isPredefinedColorSpace(value)) {
        m_unpackColorSpace = value;
    }
}

Optional<WebGLContextAttributes> WebGLRenderingContext::getContextAttributes()
{
    ENTER_CONTEXT_SCOPE(Optional<WebGLContextAttributes>());

    return m_attributes;
}

Optional<GCVector<String*>> WebGLRenderingContext::getSupportedExtensions()
{
    ENTER_CONTEXT_SCOPE(Optional<GCVector<String*>>());

    return WebGLExtensionRegistry::instance().getSupportedExtensions(
        isWebGL2RenderingContext());
}

bool WebGLRenderingContext::isContextLost()
{
    return m_isContextLost;
}

Optional<ScriptObject> WebGLRenderingContext::getExtension(
    String* requestedName)
{
    ENTER_CONTEXT_SCOPE(Optional<ScriptObject>());

    // TODO: An attempt to use any features of an extension without first
    // calling getExtension to enable it must generate an appropriate GL
    // error and must not make use of the feature.

    std::string name = requestedName->toUTF8NonGCString();
    const auto& iter = m_enabledExtensions.find(name);
    if (iter != m_enabledExtensions.end()) {
        // Multiple calls to getExtension with the same extension
        // string, taking into account case-insensitive comparison, must
        // return the same object as long as the extension is enabled.
        return iter->second;
    }

    Optional<ExtensionGenerator> maybeGenerator =
        WebGLExtensionRegistry::instance().getGenerator(
            name, isWebGL2RenderingContext());

    if (!maybeGenerator.hasValue()) {
        return Optional<ScriptObject>();
    }

    ScriptObject object = maybeGenerator.value()(scriptBindingInstance(), this);
    m_enabledExtensions.insert({ name, object });
    // EXT_color_buffer_float changes what a framebuffer is complete with.
    invalidateFramebufferStatus();
    return object;
}

void WebGLRenderingContext::activeTexture(GLenum texture)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->activeTexture(texture);

    if (!hasNewGLError()) {
        // Texture bindings are per unit; TEXTURE_BINDING_2D and friends must
        // report the object bound to the unit selected here.
        m_activeTextureUnit = texture;
    }
}

void WebGLRenderingContext::attachShader(WebGLProgram* program,
                                         WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(program) || !validateProgramOrShader(shader)) {
        return;
    }

    hasNewGLError(); // Keep an earlier error from failing this call.
    m_gl->attachShader(program->glObject(), shader->glObject());
    if (hasNewGLError()) {
        return;
    }
    program->addAttachedShader(shader);
    shader->incrementAttachCount();
}

void WebGLRenderingContext::bindAttribLocation(WebGLProgram* program,
                                               GLuint index, String* name)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(program)) {
        return;
    }

    // Names with a reserved WebGL prefix are an error here, unlike in the
    // location queries (WebGL 1.0 "GLSL Constructs").
    if (name->startsWith("webgl_") || name->startsWith("_webgl_")) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (!checkAttribOrUniformName(name)) {
        return;
    }

    m_gl->bindAttribLocation(program->glObject(), index, CSTR(name));
}

void WebGLRenderingContext::bindBuffer(GLenum target,
                                       Optional<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        // Binding a deleted buffer leaves the binding untouched (WebGL 1.0
        // §5.14.5).
        if (value->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->target() != GL_NONE && value->target() != target) {
            // An attempt to bind a buffer object to the other target will
            // generate an INVALID_OPERATION error, and the current binding will
            // remain untouched. (Note: This isn't a GLES Spec., but WebGL one.
            // We need to set it directly, not use a retrieved value.)
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        m_gl->bindBuffer(target, value->glObject());
        m_state->setBoundBuffer(target, value);

        // A given WebGLBuffer object may only be bound to one of the
        // ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER target in its lifetime.
        value->setTargetOnce(target);
    } else {
        // If the buffer is null then any buffer currently bound is unbound.
        m_gl->bindBuffer(target, 0);
        m_state->setBoundBuffer(target, nullptr);
    }
}

void WebGLRenderingContext::bindFramebuffer(
    GLenum target, Optional<WebGLFramebuffer*> maybeFramebuffer)
{
    ENTER_CONTEXT_SCOPE();

    // OpenGL ES 2.0 §4.4.1 has only FRAMEBUFFER; READ_FRAMEBUFFER and
    // DRAW_FRAMEBUFFER are WebGL 2's (WebGL2RenderingContext overrides this).
    if (target != GL_FRAMEBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (maybeFramebuffer.hasValue()) {
        WebGLFramebuffer* frameBuffer = maybeFramebuffer.value();

        if (!isFromCurrentContext(frameBuffer)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (frameBuffer->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        m_gl->bindFramebuffer(target, frameBuffer->glObject());
        m_state->setWebGLFramebuffer(frameBuffer);
    } else {
        m_gl->bindFramebuffer(target, m_framebufferTexture->fbo());
        m_state->setWebGLFramebuffer(nullptr);
    }
}

void WebGLRenderingContext::bindRenderbuffer(
    GLenum target, Optional<WebGLRenderbuffer*> maybeRenderbuffer)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeRenderbuffer.hasValue()) {
        WebGLRenderbuffer* renderBuffer = maybeRenderbuffer.value();

        if (!isFromCurrentContext(renderBuffer)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (renderBuffer->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (target != GL_RENDERBUFFER) {
            // NOTE: how to handle this case is not found in the specification.
            return;
        }

        m_gl->bindRenderbuffer(target, renderBuffer->glObject());
        m_boundRenderbuffer = renderBuffer;
    } else {
        m_gl->bindRenderbuffer(target, 0);
        m_boundRenderbuffer.reset();
    }
}

void WebGLRenderingContext::bindTexture(GLenum target,
                                        Optional<WebGLTexture*> maybeTexture)
{
    ENTER_CONTEXT_SCOPE();

    // OpenGL ES 2.0 §3.7.13; WebGL2RenderingContext checks its own targets.
    if (webGLVersion() == 1 && target != GL_TEXTURE_2D &&
        target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (maybeTexture.hasValue()) {
        WebGLTexture* texture = maybeTexture.value();

        if (!isFromCurrentContext(texture)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (texture->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        // OpenGL ES 2.0 §3.7.13: a texture keeps the target it was first
        // bound to, and the failed bind changes no binding.
        if (texture->target() != 0 && texture->target() != target) {
            setGLError(GL_INVALID_OPERATION,
                       "the texture was bound to another target");
            return;
        }

        texture->setTarget(target);
        m_gl->bindTexture(target, texture->glObject());
        m_boundTextures[target] = texture->glObject();
        m_boundTextureObjects[textureBindingKey(m_activeTextureUnit, target)] =
            texture;
    } else {
        m_gl->bindTexture(target, 0);
        m_boundTextures.erase(target);
        m_boundTextureObjects.erase(
            textureBindingKey(m_activeTextureUnit, target));
    }
}

void WebGLRenderingContext::blendColor(GLclampf red, GLclampf green,
                                       GLclampf blue, GLclampf alpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->blendColor(red, green, blue, alpha);
}

// MIN and MAX are core in WebGL 2 and come with EXT_blend_minmax in WebGL 1;
// the driver checks every other mode.
bool WebGLRenderingContext::isBlendEquationEnabled(GLenum mode)
{
    return (mode != GL_MIN && mode != GL_MAX) || webGLVersion() >= 2 ||
           isExtensionEnabled("EXT_blend_minmax");
}

void WebGLRenderingContext::blendEquation(GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    if (!isBlendEquationEnabled(mode)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    m_gl->blendEquation(mode);
}

void WebGLRenderingContext::blendEquationSeparate(GLenum modeRGB,
                                                  GLenum modeAlpha)
{
    ENTER_CONTEXT_SCOPE();

    if (!isBlendEquationEnabled(modeRGB) ||
        !isBlendEquationEnabled(modeAlpha)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    m_gl->blendEquationSeparate(modeRGB, modeAlpha);
}

// Constant color and constant alpha cannot be used together as source and
// destination factors (WebGL 1.0 "Blending With Constant Color").
static bool mixesConstantColorAndAlpha(GLenum src, GLenum dst)
{
    auto isColor = [](GLenum f) {
        return f == GL_CONSTANT_COLOR || f == GL_ONE_MINUS_CONSTANT_COLOR;
    };
    auto isAlpha = [](GLenum f) {
        return f == GL_CONSTANT_ALPHA || f == GL_ONE_MINUS_CONSTANT_ALPHA;
    };
    return (isColor(src) && isAlpha(dst)) || (isAlpha(src) && isColor(dst));
}

void WebGLRenderingContext::blendFunc(GLenum sfactor, GLenum dfactor)
{
    ENTER_CONTEXT_SCOPE();

    if (mixesConstantColorAndAlpha(sfactor, dfactor)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->blendFunc(sfactor, dfactor);
}

void WebGLRenderingContext::blendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
                                              GLenum srcAlpha, GLenum dstAlpha)
{
    ENTER_CONTEXT_SCOPE();

    if (mixesConstantColorAndAlpha(srcRGB, dstRGB)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GLenum WebGLRenderingContext::checkFramebufferStatus(GLenum target)
{
    ENTER_CONTEXT_SCOPE(GL_FRAMEBUFFER_UNSUPPORTED);

    if (m_isContextLost) {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }

    bool isWebGL2 = isWebGL2RenderingContext();
    if (isWebGL2) {
        if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
            target != GL_DRAW_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return 0;
        }
    } else {
        if (target != GL_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return 0;
        }
    }

    Optional<WebGLFramebuffer*> maybeFb = getBoundFramebuffer(target);
    if (maybeFb.hasValue() && maybeFb.value()) {
        GLenum webglStatus = maybeFb.value()->webGLStatus();
        if (webglStatus != GL_FRAMEBUFFER_COMPLETE) {
            return webglStatus;
        }
        return m_gl->checkFramebufferStatus(target);
    }

    return GL_FRAMEBUFFER_COMPLETE;
}

void WebGLRenderingContext::clear(uint32_t mask)
{
    ENTER_CONTEXT_SCOPE();

    if (mask &
        ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        setGLError(GL_INVALID_VALUE, "invalid clear mask");
        return;
    }
    if (!validateFramebufferComplete(GL_DRAW_FRAMEBUFFER)) {
        return;
    }

    completePendingJobs();

    m_gl->clear(mask);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::clearColor(float red, float green, float blue,
                                       float alpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->clearColor(red, green, blue, alpha);

    TRACE(WEBGL_V, KV(red), KV(green), KV(blue), KV(alpha));
}

void WebGLRenderingContext::clearDepth(GLclampf depth)
{
    ENTER_CONTEXT_SCOPE();

    // shortcut: with depth false and stencil true the drawing buffer still
    // has the depth of its DEPTH24_STENCIL8, and only the clear value 1 keeps
    // it passing the depth test as if absent (OpenGL ES 3.0.6 §4.1.5); the
    // value is lost for framebuffer objects too. Drop the gate once the
    // default framebuffer emulates the absent depth buffer.
    if (!m_attributes.depth()) {
        return;
    }

    m_gl->clearDepthf(depth);

    TRACE(WEBGL_V, KV(depth));
}

void WebGLRenderingContext::clearStencil(GLint s)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->clearStencil(s);
    TRACE(WEBGL_V, KV(s));
}

void WebGLRenderingContext::colorMask(GLboolean red, GLboolean green,
                                      GLboolean blue, GLboolean alpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->colorMask(red, green, blue, alpha);
}

// The deepest expression nesting compileShader() accepts. 256 is the limit
// the web has converged on (ANGLE's MaxExpressionComplexity, enforced by
// the other browsers). Shader source is web content, so a shader that can kill
// the engine fails to compile here instead of reaching the driver.
static constexpr size_t kMaxShaderExpressionDepth = 256;

// The deepest call stack compileShader() accepts, in frames (main() is
// one). 256 is ANGLE's MaxCallStackDepth, which the other browsers enforce:
// a driver inlines every call, and Mesa needs gigabytes to link a chain of
// thousands of functions.
static constexpr size_t kMaxShaderCallDepth = 256;

// Precedence of a GLSL binary operator token (higher binds tighter).
// The conditional and assignment operators are right associative and sit
// lowest.
static int binaryOperatorPrecedence(const char* op, size_t length)
{
    if (length == 1) {
        switch (op[0]) {
        case '*':
        case '/':
        case '%':
            return 13;
        case '+':
        case '-':
            return 12;
        case '<':
        case '>':
            return 10;
        case '&':
            return 8;
        case '^':
            return 7;
        case '|':
            return 6;
        case '?':
        case ':':
            return 2;
        case '=':
            return 1;
        default:
            return 0;
        }
    }
    if (op[length - 1] == '=') {
        if (length == 2 && (op[0] == '<' || op[0] == '>')) {
            return 10;
        }
        if (length == 2 && (op[0] == '=' || op[0] == '!')) {
            return 9;
        }
        return 1; // compound assignment
    }
    if (op[0] == '<' || op[0] == '>') {
        return 11;
    }
    if (op[0] == '&') {
        return 5;
    }
    if (op[0] == '^') {
        return 4;
    }
    if (op[0] == '|') {
        return 3;
    }
    return 0;
}

// Estimates how deeply the deepest expression in a GLSL source nests: every
// binary, unary, conditional, call, index, field selection and parenthesis
// level counts one, the way a driver's parser builds its tree. Only
// expression structure is tracked; declarations and statements are not
// parsed and preprocessor lines are skipped, so an expression a macro
// expands to is not seen. The scan stops once the estimate exceeds `limit`
// (which also bounds its bookkeeping) and then returns `limit + 1`.
static size_t shaderExpressionDepth(const std::string& source, size_t limit)
{
    struct Frame {
        size_t extra;       // depth the frame applies to: the callee or indexed
                            // operand, or a unary prefix on a parenthesis
        size_t maxArg;      // deepest completed sub-expression inside
        size_t operandBase; // stack sizes when the frame opened
        size_t operatorBase;
    };
    std::vector<size_t> operands; // depth of each pending operand
    std::vector<int> operators;   // precedence of each pending binary operator
    std::vector<Frame> frames;    // open parentheses and brackets
    size_t statementMax = 0; // deepest completed expression of the statement
    size_t deepest = 0;
    size_t pendingUnary = 0; // prefix operators awaiting their operand
    bool prevIsOperand = false;

    auto operandBase = [&]() {
        return frames.empty() ? 0 : frames.back().operandBase;
    };
    auto operatorBase = [&]() {
        return frames.empty() ? 0 : frames.back().operatorBase;
    };
    auto reduceTop = [&]() {
        operators.pop_back();
        size_t depth = 0;
        for (int side = 0; side < 2 && operands.size() > operandBase();
             side++) {
            depth = std::max(depth, operands.back());
            operands.pop_back();
        }
        operands.push_back(depth + 1);
    };
    // Reduces the pending expression of the innermost frame and folds its
    // depth into that frame (or the statement) maximum.
    auto foldExpression = [&]() {
        while (operators.size() > operatorBase()) {
            reduceTop();
        }
        size_t depth = pendingUnary;
        while (operands.size() > operandBase()) {
            depth = std::max(depth, operands.back());
            operands.pop_back();
        }
        size_t& target = frames.empty() ? statementMax : frames.back().maxArg;
        target = std::max(target, depth);
        pendingUnary = 0;
        prevIsOperand = false;
    };
    auto closeFrame = [&]() {
        foldExpression();
        Frame frame = frames.back();
        frames.pop_back();
        operands.push_back(frame.extra + 1 + frame.maxArg);
        prevIsOperand = true;
    };
    auto endStatement = [&]() {
        while (!frames.empty()) {
            closeFrame();
        }
        foldExpression();
        deepest = std::max(deepest, statementMax);
        statementMax = 0;
    };
    auto pushBinary = [&](int precedence) {
        bool rightAssociative = precedence <= 2;
        while (operators.size() > operatorBase() &&
               (operators.back() > precedence ||
                (operators.back() == precedence && !rightAssociative))) {
            reduceTop();
        }
        operators.push_back(precedence);
        prevIsOperand = false;
    };
    auto openFrame = [&](bool appliesToOperand) {
        size_t extra = pendingUnary;
        if (appliesToOperand && operands.size() > operandBase()) {
            extra += operands.back();
            operands.pop_back();
        }
        frames.push_back({ extra, 0, operands.size(), operators.size() });
        pendingUnary = 0;
        prevIsOperand = false;
    };
    // A lower bound of the depth the pending structure reduces to, checked
    // after every token so the scan can stop early.
    auto exceedsLimit = [&]() {
        size_t bound = frames.size() + operators.size() + pendingUnary;
        for (const Frame& frame : frames) {
            bound += frame.extra;
        }
        if (operands.size() > operandBase()) {
            bound += operands.back();
        }
        return std::max(bound, statementMax) > limit;
    };
    auto pushOperand = [&]() {
        if (!prevIsOperand) {
            operands.push_back(pendingUnary);
            pendingUnary = 0;
            prevIsOperand = true;
        }
    };

    size_t n = source.size();
    size_t i = 0;
    while (i < n) {
        char c = source[i];
        if (isspace(c)) {
            i++;
            continue;
        }
        if (c == '#') {
            // A preprocessor directive runs to the line break. The source is
            // the preprocessor view, so a CR also ends the line and there are
            // no line continuations; the backslash case stays harmless.
            while (i < n && source[i] != '\n' && source[i] != '\r') {
                if (source[i] == '\\' && i + 1 < n &&
                    (source[i + 1] == '\n' || source[i + 1] == '\r')) {
                    i++;
                }
                i++;
            }
            continue;
        }
        if (c == '/' && i + 1 < n && source[i + 1] == '/') {
            while (i < n && source[i] != '\n') {
                i++;
            }
            continue;
        }
        if (c == '/' && i + 1 < n && source[i + 1] == '*') {
            size_t end = source.find("*/", i + 2);
            i = (end == std::string::npos) ? n : end + 2;
            continue;
        }
        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            while (i < n && (isalnum(static_cast<unsigned char>(source[i])) ||
                             source[i] == '_')) {
                i++;
            }
            pushOperand();
        } else if (isdigit(static_cast<unsigned char>(c)) ||
                   (c == '.' && i + 1 < n &&
                    isdigit(static_cast<unsigned char>(source[i + 1])))) {
            char prev = 0;
            while (i < n) {
                char d = source[i];
                bool exponentSign =
                    (d == '+' || d == '-') && (prev == 'e' || prev == 'E');
                if (!(isalnum(static_cast<unsigned char>(d)) || d == '.' ||
                      exponentSign)) {
                    break;
                }
                prev = d;
                i++;
            }
            pushOperand();
        } else {
            // punctuator, longest match first
            size_t length = 1;
            if (i + 2 < n && source[i + 2] == '=' &&
                ((c == '<' && source[i + 1] == '<') ||
                 (c == '>' && source[i + 1] == '>'))) {
                length = 3;
            } else if (i + 1 < n) {
                char d = source[i + 1];
                if ((d == c && strchr("+-<>&|^=", c)) ||
                    (d == '=' && strchr("+-*/%&|^<>!", c))) {
                    length = 2;
                }
            }
            const char* op = source.data() + i;
            i += length;
            if (length == 2 && (c == '+' || c == '-') && op[1] == c) {
                if (prevIsOperand) {
                    operands.back()++; // postfix ++ / --
                } else {
                    pendingUnary++;
                }
            } else if (length == 1 &&
                       (c == '!' || c == '~' ||
                        ((c == '+' || c == '-') && !prevIsOperand))) {
                pendingUnary++;
            } else if (length == 1 && c == '.') {
                if (prevIsOperand) {
                    operands.back()++; // field selection or swizzle
                }
            } else if (length == 1 && (c == '(' || c == '[')) {
                openFrame(prevIsOperand);
            } else if (length == 1 && (c == ')' || c == ']')) {
                if (!frames.empty()) {
                    closeFrame();
                }
            } else if (length == 1 &&
                       (c == ',' || (c == ';' && !frames.empty()))) {
                // argument, declarator or for-header separator
                foldExpression();
            } else if (length == 1 && (c == ';' || c == '{' || c == '}')) {
                endStatement();
            } else {
                int precedence = binaryOperatorPrecedence(op, length);
                if (precedence > 0) {
                    pushBinary(precedence);
                }
            }
        }
        if (exceedsLimit()) {
            return limit + 1;
        }
    }
    endStatement();
    return std::min(deepest, limit + 1);
}

// The length of the line break at `i`, 0 if there is none: CR, LF, and CR
// and LF together in either order, each end one line (GLSL ES 3.00 §3.2).
static size_t lineBreakLength(const std::string& s, size_t i)
{
    if (i >= s.size() || (s[i] != '\r' && s[i] != '\n')) {
        return 0;
    }
    return s.compare(i, 2, "\r\n") == 0 || s.compare(i, 2, "\n\r") == 0 ? 2 : 1;
}

// The source as the preprocessor reads it, which is also the text the driver
// compiles: a backslash before a line break joins the lines (GLSL ES 3.00
// §3.2; Mesa does so for GLSL ES 1.00 too) and a comment is one space
// (GLSL ES 1.00 and 3.00 §3.4). Lines keep their numbers: the line breaks a
// join takes away follow the line it made, as Mesa's preprocessor adds them;
// those of a comment stay in place, unless it is in a directive or a '#'
// follows it (a line break would end the directive or start one), and then
// they follow the line too. False when a comment is not terminated.
static bool preprocessorView(const std::string& src, std::string& view)
{
    std::string joined;
    joined.reserve(src.size());
    size_t pending = 0;
    // Before the line breaks of a line that lost some: a space keeps the
    // first from pairing with a CR or LF before it into one line break, and
    // from following a backslash (a continuation the source does not have).
    auto separate = [&joined](char next) {
        const char last = joined.empty() ? 0 : joined.back();
        if (last == '\\' || ((last == '\r' || last == '\n') && last != next)) {
            joined += ' ';
        }
    };
    for (size_t i = 0; i < src.size();) {
        const size_t joinLength = lineBreakLength(src, i + 1);
        if (src[i] == '\\' && joinLength) {
            i += 1 + joinLength;
            pending++;
            continue;
        }
        const size_t length = lineBreakLength(src, i);
        if (!length) {
            joined += src[i++];
            continue;
        }
        if (pending) {
            separate(src[i]);
        }
        for (size_t copies = 0; copies <= pending; copies++) {
            joined.append(src, i, length);
        }
        pending = 0;
        i += length;
    }
    if (pending) {
        separate('\n');
        joined.append(pending, '\n');
    }

    view.clear();
    view.reserve(joined.size());
    pending = 0;
    bool lineHasToken = false;
    bool lineIsDirective = false;
    for (size_t i = 0; i < joined.size();) {
        if (joined.compare(i, 2, "//") == 0) {
            // A // comment keeps the line break that ends it.
            i = std::min(joined.find_first_of("\r\n", i), joined.size());
            view += ' ';
            continue;
        }
        if (joined.compare(i, 2, "/*") == 0) {
            const size_t end = joined.find("*/", i + 2);
            if (end == std::string::npos) {
                return false;
            }
            size_t breaks = 0;
            for (size_t k = i + 2; k < end;) {
                const size_t length = lineBreakLength(joined, k);
                breaks += length ? 1 : 0;
                k += std::max<size_t>(1, length);
            }
            i = end + 2;
            view += ' ';
            if (!breaks) {
                continue;
            }
            const size_t next = joined.find_first_not_of(" \t\v\f", i);
            if (lineIsDirective ||
                (next < joined.size() && joined[next] == '#')) {
                pending += breaks;
            } else {
                // The spaces keep these from pairing with a CR or LF next to
                // them into one line break.
                view.append(breaks, '\n');
                view += ' ';
            }
            continue;
        }
        const size_t length = lineBreakLength(joined, i);
        if (length) {
            for (size_t copies = 0; copies <= pending; copies++) {
                view.append(joined, i, length);
            }
            pending = 0;
            lineHasToken = lineIsDirective = false;
            i += length;
            continue;
        }
        const char c = joined[i++];
        if (!lineHasToken && c != ' ' && c != '\t' && c != '\v' && c != '\f') {
            lineHasToken = true;
            lineIsDirective = c == '#';
        }
        view += c;
    }
    if (pending) {
        if (view.back() == '\\') {
            view += ' ';
        }
        view.append(pending, '\n');
    }
    return true;
}

static bool isIdentifierCharacter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

// The end of the identifier characters from `i`.
static size_t identifierEnd(const std::string& s, size_t i)
{
    while (i < s.size() && isIdentifierCharacter(s[i])) {
        i++;
    }
    return i;
}

// The GLSL ES 1.00 and 3.00 built-ins (§7); every other gl_ name is
// reserved (GLSL ES 1.00 §3.7, 3.00 §3.8) and undeclared. Mesa also declares
// desktop GLSL ones, such as gl_Color, in GLSL ES 1.00.
static bool isGLSLESBuiltin(const std::string& name)
{
    static const char* const kBuiltins[] = {
        "gl_Position",
        "gl_PointSize",
        "gl_FragCoord",
        "gl_FrontFacing",
        "gl_FragColor",
        "gl_FragData",
        "gl_PointCoord",
        "gl_DepthRange",
        "gl_DepthRangeParameters",
        "gl_MaxVertexAttribs",
        "gl_MaxVertexUniformVectors",
        "gl_MaxVaryingVectors",
        "gl_MaxVertexTextureImageUnits",
        "gl_MaxCombinedTextureImageUnits",
        "gl_MaxTextureImageUnits",
        "gl_MaxFragmentUniformVectors",
        "gl_MaxDrawBuffers",
        // GLSL ES 3.00 only.
        "gl_VertexID",
        "gl_InstanceID",
        "gl_FragDepth",
        "gl_MaxVertexOutputVectors",
        "gl_MaxFragmentInputVectors",
        "gl_MinProgramTexelOffset",
        "gl_MaxProgramTexelOffset",
    };
    for (const char* builtin : kBuiltins) {
        if (name == builtin) {
            return true;
        }
    }
    return false;
}

// Names no WebGL shader may use: the webgl_ and _webgl_ prefixes (WebGL 1.0
// and 2.0 "Supported GLSL Constructs") and gl_ names that are not built-ins.
static bool isReservedShaderName(const std::string& name)
{
    if (name.compare(0, 6, "webgl_") == 0 ||
        name.compare(0, 7, "_webgl_") == 0) {
        return true;
    }
    return name.compare(0, 3, "gl_") == 0 && !isGLSLESBuiltin(name);
}

// Identifiers with two consecutive underscores are reserved (GLSL ES 1.00
// §3.7, 3.00 §3.8) and fail in WebGL, but a macro may have such a name
// (GLSL ES 3.00 §3.5).
static bool hasReservedUnderscores(const std::string& name)
{
    return name.find("__") != std::string::npos && name != "__LINE__" &&
           name != "__FILE__" && name != "__VERSION__";
}

// The shader extensions of the WebGL 1 extensions (WebGL extension
// registry). WebGL 2 has their features in core and enables none.
static const struct {
    const char* webgl;
    const char* shader;
} kShaderExtensions[] = {
    { "OES_standard_derivatives", "GL_OES_standard_derivatives" },
    { "EXT_frag_depth", "GL_EXT_frag_depth" },
    { "WEBGL_draw_buffers", "GL_EXT_draw_buffers" },
    { "EXT_shader_texture_lod", "GL_EXT_shader_texture_lod" },
};

// Whether shaders of this context may use the shader extension `name`
// (WebGL 1.0 "Detecting and enabling extensions").
bool WebGLRenderingContext::isShaderExtensionEnabled(const std::string& name)
{
    if (webGLVersion() >= 2) {
        return false;
    }
    for (const auto& extension : kShaderExtensions) {
        if (name == extension.shader) {
            return isExtensionEnabled(extension.webgl);
        }
    }
    return false;
}

// The macro a driver predefines for a shader extension it supports:
// GL_<VENDOR>_<name> (Khronos extension naming). GL_FRAGMENT_PRECISION_HIGH
// has that shape but is core.
static bool isExtensionMacroName(const std::string& name)
{
    if (name.compare(0, 3, "GL_") != 0 ||
        name == "GL_FRAGMENT_PRECISION_HIGH") {
        return false;
    }
    size_t i = 3;
    while (i < name.size() && ((name[i] >= 'A' && name[i] <= 'Z') ||
                               (name[i] >= '0' && name[i] <= '9'))) {
        i++;
    }
    return i > 3 && i < name.size() && name[i] == '_';
}

// Appends the line the driver gets for the #extension directive
// view[line, end), whose text after "extension" starts at `rest` (GLSL ES
// 1.00 and 3.00 §3.4). It stays for an extension the context enabled; of any
// other one, `require` fails, and `enable`, `warn` and `disable` only drop
// the line, so that the driver does not enable the extension.
void WebGLRenderingContext::appendExtensionDirective(const std::string& view,
                                                     size_t line, size_t rest,
                                                     size_t end,
                                                     std::string& out)
{
    auto skipSpaces = [&view, end](size_t i) {
        while (i < end && (view[i] == ' ' || view[i] == '\t' ||
                           view[i] == '\v' || view[i] == '\f')) {
            i++;
        }
        return i;
    };
    const size_t nameBegin = skipSpaces(rest);
    const size_t nameEnd = std::min(identifierEnd(view, nameBegin), end);
    const std::string name = view.substr(nameBegin, nameEnd - nameBegin);
    size_t i = skipSpaces(nameEnd);
    std::string behavior;
    if (i < end && view[i] == ':') {
        const size_t behaviorBegin = skipSpaces(i + 1);
        i = std::min(identifierEnd(view, behaviorBegin), end);
        behavior = view.substr(behaviorBegin, i - behaviorBegin);
        i = skipSpaces(i);
    }
    if (name == "all") {
        // `all : warn` makes Mesa enable every extension it supports; the
        // driver reports every other behavior that is an error for `all`.
        if (behavior != "warn" || i != end) {
            out.append(view, line, end - line);
        } else {
            // A dropped line keeps one space so the line breaks on its two
            // sides cannot fuse into one (line numbers stay).
            out += ' ';
        }
        return;
    }
    if (name.empty() || i != end ||
        (behavior != "require" && behavior != "enable" && behavior != "warn" &&
         behavior != "disable")) {
        out += "#error malformed #extension directive";
    } else if (isShaderExtensionEnabled(name)) {
        out.append(view, line, end - line);
    } else if (behavior == "require") {
        out += "#error extension " + name + " is not supported";
    } else {
        // Dropped (enable/warn/disable of an unsupported extension): one space
        // keeps the surrounding line breaks apart.
        out += ' ';
    }
}

// GLSL ES 3.00 §3.4: "#version 300 es" is the first line of a GLSL ES 3.00
// shader; compileShader() and the driver reject it elsewhere.
static bool isESSL3Source(const std::string& view)
{
    const size_t hash = view.find_first_not_of(" \t\v\f\r\n");
    if (hash == std::string::npos || view[hash] != '#') {
        return false;
    }
    const size_t end = std::min(view.find_first_of("\r\n", hash), view.size());
    std::istringstream directive(view.substr(hash + 1, end - hash - 1));
    std::string name, number, profile;
    directive >> name >> number >> profile;
    return name == "version" && number == "300" && profile == "es";
}

// Reserved for GLSL ES 3.00 in WebGL 2 (the conformance suite's keywords
// test; ANGLE does the same) though GLSL ES 3.00.6 §3.8 no longer lists
// them, so a conforming driver accepts them.
static bool isWebGLReservedESSL3Word(const std::string& name)
{
    return name == "image1DShadow" || name == "image2DShadow" ||
           name == "image1DArrayShadow" || name == "image2DArrayShadow";
}

// The precedence of the preprocessor binary operator at text[i] (GLSL ES
// 1.00 §3.4, 3.00 §3.5; higher binds tighter) and its length, or 0.
static int integerOperatorPrecedence(const std::string& text, size_t i,
                                     size_t end, size_t& length)
{
    const char c = text[i];
    const char d = i + 1 < end ? text[i + 1] : '\0';
    length = 2;
    if ((c == '+' || c == '-') && d == c) {
        return 0; // the ++ and -- tokens
    }
    if (c == '|' && d == '|') {
        return 1;
    }
    if (c == '&' && d == '&') {
        return 2;
    }
    if ((c == '=' || c == '!') && d == '=') {
        return 6;
    }
    if ((c == '<' || c == '>') && d == c) {
        return 8;
    }
    if ((c == '<' || c == '>') && d == '=') {
        return 7;
    }
    length = 1;
    switch (c) {
    case '|':
        return 3;
    case '^':
        return 4;
    case '&':
        return 5;
    case '<':
    case '>':
        return 7;
    case '+':
    case '-':
        return 9;
    case '*':
    case '/':
    case '%':
        return 10;
    default:
        return 0;
    }
}

static bool readIntegerExpression(const std::string& text, size_t& i,
                                  size_t end, size_t depth, int precedence,
                                  int64_t& value);

// An operand of readIntegerExpression(): an integer literal (GLSL ES 1.00
// §4.1.3, 3.00 §4.1.3, without suffix), a parenthesized expression or an
// operand after a unary + - ~ !.
static bool readIntegerOperand(const std::string& text, size_t& i, size_t end,
                               size_t depth, int64_t& value)
{
    i = std::min(text.find_first_not_of(" \t\v\f", i), end);
    if (i == end || depth >= kMaxShaderExpressionDepth) {
        return false;
    }
    const char c = text[i];
    if (c == '(') {
        i++;
        if (!readIntegerExpression(text, i, end, depth + 1, 1, value)) {
            return false;
        }
        i = std::min(text.find_first_not_of(" \t\v\f", i), end);
        if (i == end || text[i] != ')') {
            return false;
        }
        i++;
        return true;
    }
    if (c == '+' || c == '-' || c == '~' || c == '!') {
        if ((c == '+' || c == '-') && i + 1 < end && text[i + 1] == c) {
            return false; // the ++ and -- tokens
        }
        i++;
        if (!readIntegerOperand(text, i, end, depth + 1, value)) {
            return false;
        }
        if (c == '-') {
            value = -value;
        } else if (c == '~') {
            value = ~value;
        } else if (c == '!') {
            value = !value;
        }
        return value >= INT32_MIN && value <= INT32_MAX;
    }
    if (c < '0' || c > '9') {
        return false;
    }
    const size_t literalEnd = std::min(identifierEnd(text, i), end);
    int base = 10;
    size_t k = i;
    if (c == '0') {
        base = 8;
        if (k + 1 < literalEnd && (text[k + 1] == 'x' || text[k + 1] == 'X')) {
            base = 16;
            k += 2;
            if (k == literalEnd) {
                return false;
            }
        }
    }
    value = 0;
    for (; k < literalEnd; k++) {
        const char d = text[k];
        int digit = base;
        if (d >= '0' && d <= '9') {
            digit = d - '0';
        } else if (d >= 'a' && d <= 'f') {
            digit = d - 'a' + 10;
        } else if (d >= 'A' && d <= 'F') {
            digit = d - 'A' + 10;
        }
        if (digit >= base) {
            return false;
        }
        value = value * base + digit;
        if (value > INT32_MAX) {
            return false;
        }
    }
    i = literalEnd;
    return true;
}

// Reads into `value` the longest expression at text[i, end) of integer
// literals, parentheses and the preprocessor's operators (GLSL ES 1.00 §3.4,
// 3.00 §3.5) whose binary operators bind at least as tight as `precedence`,
// evaluated as C does, and moves `i` past it. False, so that the caller
// leaves the text to the driver, for any other text, a value outside int32
// on the way, a division by zero, a shift outside [0, 31], or nesting deeper
// than kMaxShaderExpressionDepth.
static bool readIntegerExpression(const std::string& text, size_t& i,
                                  size_t end, size_t depth, int precedence,
                                  int64_t& value)
{
    if (!readIntegerOperand(text, i, end, depth, value)) {
        return false;
    }
    while (true) {
        const size_t op = std::min(text.find_first_not_of(" \t\v\f", i), end);
        size_t length = 0;
        const int opPrecedence =
            op < end ? integerOperatorPrecedence(text, op, end, length) : 0;
        if (opPrecedence < precedence) {
            return true;
        }
        i = op + length;
        int64_t rhs = 0;
        if (!readIntegerExpression(text, i, end, depth, opPrecedence + 1,
                                   rhs)) {
            return false;
        }
        const char c = text[op];
        const bool twoChars = length == 2;
        if (c == '*') {
            value *= rhs;
        } else if (c == '/' || c == '%') {
            if (!rhs) {
                return false;
            }
            value = c == '/' ? value / rhs : value % rhs;
        } else if (c == '+') {
            value += rhs;
        } else if (c == '-') {
            value -= rhs;
        } else if (opPrecedence == 8) {
            if (rhs < 0 || rhs > 31) {
                return false;
            }
            value = c == '<' ? value * (int64_t(1) << rhs) : value >> rhs;
        } else if (c == '<') {
            value = twoChars ? value <= rhs : value < rhs;
        } else if (c == '>') {
            value = twoChars ? value >= rhs : value > rhs;
        } else if (c == '=') {
            value = value == rhs;
        } else if (c == '!') {
            value = value != rhs;
        } else if (c == '&') {
            value = twoChars ? (value && rhs) : (value & rhs);
        } else if (c == '|') {
            value = twoChars ? (value || rhs) : (value | rhs);
        } else {
            value ^= rhs;
        }
        if (value < INT32_MIN || value > INT32_MAX) {
            return false;
        }
    }
}

// Appends `#line line [source-string-number]` for the #line directive whose
// text after "line" is view[i, end), when its constant integer expressions
// hold only literals and operators (GLSL ES 1.00 §3.4, 3.00 §3.5): a driver's
// preprocessor may take literals only (Mesa's does). False, with nothing
// appended, for any other form, which the driver gets as written.
static bool appendLineDirective(const std::string& view, size_t i, size_t end,
                                std::string& out)
{
    int64_t line = 0;
    int64_t source = -1;
    if (!readIntegerExpression(view, i, end, 0, 1, line) || line < 0) {
        return false;
    }
    i = std::min(view.find_first_not_of(" \t\v\f", i), end);
    if (i < end &&
        (!readIntegerExpression(view, i, end, 0, 1, source) || source < 0 ||
         std::min(view.find_first_not_of(" \t\v\f", i), end) < end)) {
        return false;
    }
    out += "#line " + std::to_string(line);
    if (source >= 0) {
        out += " " + std::to_string(source);
    }
    return true;
}

// Whether text[i, end) starts with `[`, an expression readIntegerExpression()
// reads to a nonzero value, and `]`.
static bool isNonZeroConstantIndex(const std::string& text, size_t i,
                                   size_t end)
{
    i = std::min(text.find_first_not_of(" \t\v\f", i), end);
    if (i == end || text[i] != '[') {
        return false;
    }
    i++;
    int64_t index = 0;
    if (!readIntegerExpression(text, i, end, 0, 1, index)) {
        return false;
    }
    i = std::min(text.find_first_not_of(" \t\v\f", i), end);
    return index && i < end && text[i] == ']';
}

// The text the driver compiles: `view` with the names of the shader
// extensions the context did not enable hidden, and their #extension
// directives replaced. Every identifier is checked, as the driver sees them
// all: a macro expansion makes no new one, as token pasting fails here.
bool WebGLRenderingContext::shaderSourceForDriver(const std::string& view,
                                                  std::string& out,
                                                  std::string& code,
                                                  std::string& error)
{
    static const char kSpaces[] = " \t\v\f";
    const bool hidesMaxDrawBuffers =
        webGLVersion() == 1 && !isShaderExtensionEnabled("GL_EXT_draw_buffers");
    const bool essl3 = webGLVersion() >= 2 && isESSL3Source(view);
    // GLSL ES 1.00 has one gl_FragData element without draw buffers: §7.4 in
    // WebGL 1, WebGL 2.0 "GLSL ES 1.00 Fragment Shader Output" in WebGL 2.
    const bool singleFragData =
        !essl3 && !isShaderExtensionEnabled("GL_EXT_draw_buffers");
    // Neither #if nor macros are evaluated here, so the language rules below
    // reject a token only where the compiler surely sees it: outside every
    // #if group, on a line that is no directive, not named by an earlier
    // #define, and before any function-like macro, which may drop its
    // arguments.
    size_t conditionalDepth = 0;
    std::unordered_set<std::string> macroNames;
    bool hasFunctionLikeMacro = false;
    // The names that are surely object-like macros: defined outside every #if
    // group and not #undef'd since. True for one whose body has `defined`,
    // which in an #if is an error in WebGL (KhronosGroup/WebGL#1523;
    // undefined behavior in C++ [cpp.cond]).
    std::unordered_map<std::string, bool> topLevelMacros;
    // The same for function-like macros.
    std::unordered_set<std::string> topLevelFunctionMacros;
    out.clear();
    out.reserve(view.size());
    code.clear();
    for (size_t line = 0; line < view.size();) {
        const size_t end =
            std::min(view.find_first_of("\r\n", line), view.size());
        std::string directive;
        size_t afterDirective = line;
        const size_t hash = view.find_first_not_of(kSpaces, line);
        const bool isDirective = hash < end && view[hash] == '#';
        if (isDirective) {
            const size_t name =
                std::min(view.find_first_not_of(kSpaces, hash + 1), end);
            afterDirective = std::min(identifierEnd(view, name), end);
            directive = view.substr(name, afterDirective - name);
        }

        if (directive == "extension") {
            appendExtensionDirective(view, line, afterDirective, end, out);
        } else if (directive == "line" &&
                   appendLineDirective(view, afterDirective, end, out)) {
            // replaced by its value
        } else {
            for (size_t k = afterDirective;
                 directive == "define" && k + 1 < end; k++) {
                if (view[k] == '#' && view[k + 1] == '#') {
                    error = "Token pasting (##) is not allowed";
                    return false;
                }
            }
            bool isMacroName = directive == "define";
            std::string definedName;
            bool isObjectLike = false;
            bool bodyHasDefined = false;
            const bool isSure = !conditionalDepth && !hasFunctionLikeMacro;
            const bool isCode = !isDirective && isSure;
            const bool isTopLevelIf = directive == "if" && isSure;
            bool isDefinedOperand = false;
            for (size_t i = line; i < end;) {
                const size_t tokenEnd = identifierEnd(view, i);
                if (tokenEnd == i) {
                    code += isCode ? view[i] : ' ';
                    out += view[i++];
                    continue;
                }
                const std::string token = view.substr(i, tokenEnd - i);
                if (isReservedShaderName(token)) {
                    error = "Reserved identifier " + token;
                    return false;
                }
                // The name a #define defines keeps its spelling, so that the
                // driver still rejects defining a reserved GL_ name.
                const bool isDefined = isMacroName && i >= afterDirective;
                if (isDefined) {
                    isMacroName = false;
                    definedName = token;
                    isObjectLike = tokenEnd == end || view[tokenEnd] != '(';
                } else if (directive == "define" && token == "defined") {
                    bodyHasDefined = true;
                } else if (directive == "undef" && i >= afterDirective) {
                    topLevelMacros.erase(token);
                    topLevelFunctionMacros.erase(token);
                }
                const bool isMacro = macroNames.count(token);
                // shaderLimitationError() judges only what the compiler
                // surely sees: a macro or a name on any other line may
                // stand for other tokens.
                if (!isCode || isMacro) {
                    code += '#';
                }
                code += token;
                // Code may use a macro with such a name while it is surely
                // defined, as no macro body can bring the name back, a
                // function-like one where its `(` follows on the line; other
                // directives never pass a name to the compiler.
                const size_t afterToken =
                    std::min(view.find_first_not_of(kSpaces, tokenEnd), end);
                const bool isCalled = topLevelFunctionMacros.count(token) &&
                                      afterToken < end &&
                                      view[afterToken] == '(';
                if (!isDefined && hasReservedUnderscores(token) &&
                    (directive == "define" ||
                     (!isDirective && !topLevelMacros.count(token) &&
                      !isCalled))) {
                    error = "Reserved identifier " + token;
                    return false;
                }
                if (isCode && !isMacro) {
                    if (!essl3 && (token == "while" || token == "do")) {
                        // WebGL 1.0 "Supported GLSL Constructs" (GLSL ES
                        // 1.00 Appendix A §4).
                        error = "while and do-while loops are not allowed";
                        return false;
                    }
                    if (essl3 && isWebGLReservedESSL3Word(token)) {
                        error = "Reserved word " + token;
                        return false;
                    }
                    if (singleFragData && token == "gl_FragData" &&
                        isNonZeroConstantIndex(view, tokenEnd, end)) {
                        error = "gl_FragData index must be 0";
                        return false;
                    }
                }
                if (isTopLevelIf && i >= afterDirective) {
                    // The operand of `defined` is not expanded.
                    if (isDefinedOperand) {
                        isDefinedOperand = false;
                    } else if (token == "defined") {
                        isDefinedOperand = true;
                    } else if (topLevelMacros.count(token) &&
                               topLevelMacros[token]) {
                        error = "'defined' produced by macro expansion in #if";
                        return false;
                    }
                }
                const bool isName =
                    !isDefined && (token[0] < '0' || token[0] > '9');
                if (isName && isExtensionMacroName(token) &&
                    !isShaderExtensionEnabled(token)) {
                    // Hidden behind a name no shader can define (see
                    // isReservedShaderName()), it is undefined.
                    // shortcut: a user name shaped like an extension macro
                    // (GL_ABC_x) is renamed too; resolve names after
                    // preprocessing to keep it.
                    out += "webgl_" + token;
                } else if (isName && hidesMaxDrawBuffers &&
                           token == "gl_MaxDrawBuffers" &&
                           (!isDirective || directive == "define")) {
                    // GLSL ES 1.00 §7.4: 1 without WEBGL_draw_buffers.
                    out += '1';
                } else if (isName && essl3 &&
                           (token == "gl_MinProgramTexelOffset" ||
                            token == "gl_MaxProgramTexelOffset") &&
                           (!isDirective || directive == "define")) {
                    // The range getParameter() reports, the GLSL ES 3.00 §7.3
                    // minimums.
                    out += token == "gl_MinProgramTexelOffset" ? "(-8)" : "7";
                } else {
                    out += token;
                }
                i = tokenEnd;
            }
            if (!definedName.empty()) {
                if (!conditionalDepth && isObjectLike) {
                    topLevelMacros[definedName] = bodyHasDefined;
                } else if (!conditionalDepth) {
                    // Without its parentheses a function-like macro's name is
                    // no expansion: it reaches the compiler.
                    topLevelMacros.erase(definedName);
                } else if (topLevelMacros.count(definedName)) {
                    topLevelMacros[definedName] = false;
                }
                if (!conditionalDepth && !isObjectLike) {
                    topLevelFunctionMacros.insert(definedName);
                } else {
                    topLevelFunctionMacros.erase(definedName);
                }
                macroNames.insert(definedName);
                hasFunctionLikeMacro |= !isObjectLike;
            }
        }
        if (directive == "if" || directive == "ifdef" ||
            directive == "ifndef") {
            conditionalDepth++;
        } else if (directive == "endif" && conditionalDepth) {
            conditionalDepth--;
        }
        const size_t next =
            std::min(view.find_first_not_of("\r\n", end), view.size());
        out.append(view, end, next - end);
        code += '\n';
        line = next;
    }
    return true;
}

// The frames of the deepest call stack the driver can reach in `view`
// (preprocessorView()), or a value above `limit` when it provably exceeds
// `limit` or the text cannot be bounded soundly. A text scanner neither
// evaluates #if nor expands macros, so the result must never be below a depth
// the driver can see whatever the preprocessor does.
//
// Sound brace bound first: every function body is one `{` and GLSL ES forbids
// recursion, so a stack of d frames needs d `{` in the compiled text. B is
// the `{` the driver can see -- every `{` in a non-directive line (dead #if
// branches included, which only over-count) plus, for every macro name in a
// non-directive line, the `{` its expansion adds (the `{` in its body and,
// transitively, those of the macros its body names; a cycle saturates). If
// B <= limit the depth is at most B, so the shader passes with no graph.
//
// Otherwise a per-definition call graph gives a tighter bound, but only when
// the token structure can be trusted: braces and parentheses nest in order in
// the code, no macro body holds a brace or an unbalanced parenthesis, and no
// macro sits where it could reshape a function header (as the name before a
// top-level `(`, between a header's `)` and `{`, or before a top-level `{`).
// Then macro expansion changes neither the `(){}` structure nor a header's
// name, every definition is seen as the driver sees it, a call to a name
// reaches every definition of it (overloads included) through a frameless
// name node, and a cyclic component weighs its definitions; a real chain of
// distinct definitions maps to a path whose weight is at least its length, so
// the depth can only come out too high. When the structure is not trusted the
// shader fails conservatively. Time and memory are linear in the source.
// shortcut: a shader the scanner cannot bound or trust fails even if its real
// call depth is shallow; a translator that expanded macros could pass it.
static size_t shaderCallDepth(const std::string& view, size_t limit)
{
    const size_t kSaturated = limit + 1;
    // Prescan the macros: id, the `{` in the body, whether the body holds any
    // brace or an unbalanced paren, and the macro names the body references.
    // A macro defined more than once weighs the most `{` of its bodies plus
    // every macro any of them names, so a redefinition cannot drop a brace.
    // Every structure here is linear in the source: one entry per #define
    // line and one reference per identifier in a body.
    std::unordered_map<std::string, uint32_t> macroId;
    std::vector<size_t> macroOwnBraces;
    std::vector<std::vector<uint32_t>> macroRefs;
    bool macroHasBrace = false;
    bool macroHasUnbalancedParen = false;
    struct MacroBody {
        uint32_t id;
        size_t begin;
        size_t end;
    };
    std::vector<MacroBody> bodies; // every definition, in source order
    auto macroNode = [&](const std::string& name) {
        auto it = macroId.emplace(name, static_cast<uint32_t>(macroId.size()));
        if (it.second) {
            macroOwnBraces.push_back(0);
            macroRefs.emplace_back();
        }
        return it.first->second;
    };
    for (size_t line = 0; line < view.size();) {
        const size_t end =
            std::min(view.find_first_of("\r\n", line), view.size());
        const size_t hash = view.find_first_not_of(" \t\v\f", line);
        if (hash < end && view[hash] == '#') {
            const size_t d =
                std::min(view.find_first_not_of(" \t\v\f", hash + 1), end);
            if (identifierEnd(view, d) == d + 6 &&
                view.compare(d, 6, "define") == 0) {
                const size_t name =
                    std::min(view.find_first_not_of(" \t\v\f", d + 6), end);
                const size_t nameEnd = std::min(identifierEnd(view, name), end);
                if (nameEnd > name) {
                    const uint32_t id =
                        macroNode(view.substr(name, nameEnd - name));
                    size_t open = 0;
                    int paren = 0;
                    for (size_t k = nameEnd; k < end; k++) {
                        const char c = view[k];
                        if (c == '{') {
                            open++;
                            macroHasBrace = true;
                        } else if (c == '}') {
                            macroHasBrace = true;
                        } else if (c == '(') {
                            paren++;
                        } else if (c == ')' && --paren < 0) {
                            macroHasUnbalancedParen = true;
                            paren = 0;
                        }
                    }
                    if (paren != 0) {
                        macroHasUnbalancedParen = true;
                    }
                    macroOwnBraces[id] = std::max(macroOwnBraces[id], open);
                    bodies.push_back({ id, nameEnd, end });
                }
            }
        }
        line = std::min(view.find_first_not_of("\r\n", end), view.size());
    }
    // Resolve the macro names each body references (now that all are known).
    for (const MacroBody& body : bodies) {
        for (size_t k = body.begin; k < body.end;) {
            const size_t e = identifierEnd(view, k);
            if (e > k) {
                auto found = macroId.find(view.substr(k, e - k));
                if (found != macroId.end() && found->second != body.id) {
                    macroRefs[body.id].push_back(found->second);
                }
                k = e;
            } else {
                k++;
            }
        }
    }
    // weight[id] = the `{` the macro's expansion adds, saturating; a cyclic
    // dependency (illegal, infinite expansion) saturates. Memoized, explicit
    // stack, no recursion.
    const size_t macros = macroOwnBraces.size();
    std::vector<size_t> weight(macros, 0);
    std::vector<uint8_t> macroState(macros, 0); // 0 new, 1 on stack, 2 done
    for (uint32_t start = 0; start < macros; start++) {
        if (macroState[start] != 0) {
            continue;
        }
        std::vector<std::pair<uint32_t, size_t>> stack;
        stack.push_back({ start, 0 });
        macroState[start] = 1;
        while (!stack.empty()) {
            uint32_t u = stack.back().first;
            size_t& ci = stack.back().second;
            if (ci < macroRefs[u].size()) {
                const uint32_t v = macroRefs[u][ci++];
                if (macroState[v] == 0) {
                    macroState[v] = 1;
                    stack.push_back({ v, 0 });
                } else if (macroState[v] == 1) {
                    weight[v] = kSaturated; // cycle
                }
                continue;
            }
            size_t w = macroOwnBraces[u];
            for (uint32_t v : macroRefs[u]) {
                w += weight[v];
            }
            weight[u] =
                std::min(weight[u] == kSaturated ? kSaturated : w, kSaturated);
            macroState[u] = 2;
            stack.pop_back();
        }
    }

    // The call graph: a node per function definition (one frame) and a node
    // per name (no frame). A call to X is one edge to name X, which has an
    // edge to every definition named X and, when X is a macro, to the names
    // its body uses; so every definition of X stays reachable from every call
    // of X. Nodes are at most definitions + names and edges at most call
    // tokens + definitions: linear in the source.
    std::unordered_map<std::string, uint32_t> nameNodes;
    std::vector<bool> isFunction;
    std::vector<std::vector<uint32_t>> callees;
    std::vector<uint32_t> lastCaller; // per name: drops repeated calls
    std::vector<uint32_t> defOrder;   // definition nodes in source order
    auto newNode = [&](bool function) {
        const uint32_t id = static_cast<uint32_t>(callees.size());
        isFunction.push_back(function);
        callees.emplace_back();
        lastCaller.push_back(UINT32_MAX);
        return id;
    };
    auto nameNode = [&](const std::string& name) {
        auto found = nameNodes.find(name);
        if (found != nameNodes.end()) {
            return found->second;
        }
        const uint32_t id = newNode(false);
        nameNodes.emplace(name, id);
        return id;
    };
    auto makeDef = [&](const std::string& name) {
        const uint32_t named = nameNode(name);
        const uint32_t id = newNode(true);
        callees[named].push_back(id);
        defOrder.push_back(id);
        return id;
    };
    auto addCall = [&](uint32_t caller, const std::string& name) {
        auto found = nameNodes.find(name);
        if (found == nameNodes.end() || lastCaller[found->second] == caller) {
            return;
        }
        lastCaller[found->second] = caller;
        callees[caller].push_back(found->second);
    };

    size_t braceBound = 0; // B
    bool trusted = !macroHasBrace && !macroHasUnbalancedParen;
    size_t defCursor = 0;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            if (braceBound <= limit) {
                return braceBound; // sound brace bound
            }
            if (!trusted) {
                return kSaturated;
            }
            defCursor = 0;
        }
        int braceBalance = 0;
        int parenBalance = 0;
        size_t braces = 0;
        size_t parens = 0;
        std::string last;
        bool lastIsMacro = false;
        std::string candidate;
        bool afterParameters = false;
        bool current = false;
        uint32_t currentNode = 0;
        bool lineStart = true;
        for (size_t i = 0; i < view.size();) {
            const char c = view[i];
            if (c == '\r' || c == '\n') {
                lineStart = true;
                i++;
                continue;
            }
            if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
                i++;
                continue;
            }
            if (lineStart && c == '#') {
                const size_t end =
                    std::min(view.find_first_of("\r\n", i), view.size());
                const size_t directive =
                    std::min(view.find_first_not_of(" \t\v\f", i + 1), end);
                const size_t name = std::min(
                    view.find_first_not_of(" \t\v\f", directive + 6), end);
                if (identifierEnd(view, directive) == directive + 6 &&
                    view.compare(directive, 6, "define") == 0 &&
                    identifierEnd(view, name) > name) {
                    const size_t nameEnd =
                        std::min(identifierEnd(view, name), end);
                    const uint32_t macro =
                        nameNode(view.substr(name, nameEnd - name));
                    for (size_t k = nameEnd; pass == 1 && k < end;) {
                        const size_t e = std::min(identifierEnd(view, k), end);
                        if (e > k) {
                            addCall(macro, view.substr(k, e - k));
                        }
                        k = std::max(e, k + 1);
                    }
                }
                i = end;
                continue;
            }
            lineStart = false;
            const size_t e = identifierEnd(view, i);
            if (e > i) {
                const std::string token = view.substr(i, e - i);
                auto m = macroId.find(token);
                const bool isMacro = m != macroId.end();
                if (c < '0' || c > '9') {
                    if (!braces) {
                        last = token;
                        lastIsMacro = isMacro;
                        if (pass == 0 && afterParameters && isMacro) {
                            trusted = false; // macro between ) and {
                        }
                    } else if (pass == 1 && current) {
                        addCall(currentNode, token);
                    }
                }
                if (pass == 0 && isMacro) {
                    braceBound =
                        std::min(braceBound + weight[m->second], kSaturated);
                }
                afterParameters = false;
                i = e;
                continue;
            }
            i++;
            if (c == '{') {
                if (pass == 0) {
                    braceBound = std::min(braceBound + 1, kSaturated);
                    braceBalance++;
                }
            } else if (c == '}') {
                if (pass == 0 && --braceBalance < 0) {
                    trusted = false;
                    braceBalance = 0;
                }
            }
            if (braces) {
                if (c == '{') {
                    braces++;
                } else if (c == '}' && --braces == 0) {
                    current = false;
                }
                continue;
            }
            if (c == '(') {
                if (pass == 0) {
                    parenBalance++;
                }
                if (parens++ == 0) {
                    candidate = last;
                    if (pass == 0 && lastIsMacro) {
                        trusted = false; // macro as the name before a header (
                    }
                }
            } else if (c == ')') {
                if (pass == 0 && --parenBalance < 0) {
                    trusted = false;
                    parenBalance = 0;
                }
                if (parens && --parens == 0) {
                    afterParameters = true;
                    continue;
                }
            } else if (c == '{') {
                if (pass == 0 && lastIsMacro) {
                    trusted = false; // macro right before a top-level {
                }
                braces = 1;
                current = false;
                if (afterParameters && !candidate.empty()) {
                    currentNode =
                        pass == 0 ? makeDef(candidate) : defOrder[defCursor];
                    defCursor += pass == 1 ? 1 : 0;
                    current = true;
                }
            }
            afterParameters = false;
        }
        if (pass == 0 && (braceBalance != 0 || parenBalance != 0)) {
            trusted = false;
        }
    }

    // Deepest stack of the per-definition graph by strongly connected
    // components (Tarjan, explicit stack: no recursion). A component is
    // finished after every one it calls; its frames are its own definitions
    // plus the frames of the deepest component it calls.
    constexpr uint32_t kUnvisited = UINT32_MAX;
    const size_t count = callees.size();
    std::vector<uint32_t> order(count, kUnvisited);
    std::vector<uint32_t> low(count, 0);
    std::vector<uint32_t> component(count, kUnvisited);
    std::vector<size_t> componentFrames;
    std::vector<uint32_t> members;
    struct Entry {
        uint32_t node;
        size_t next;
    };
    std::vector<Entry> path;
    size_t functionsOnPath = 0;
    uint32_t visited = 0;
    size_t deepest = 0;
    auto enter = [&](uint32_t n) {
        order[n] = low[n] = visited++;
        members.push_back(n);
        path.push_back({ n, 0 });
        functionsOnPath += isFunction[n] ? 1 : 0;
    };
    for (uint32_t root = 0; root < count; root++) {
        if (!isFunction[root] || order[root] != kUnvisited) {
            continue;
        }
        enter(root);
        while (!path.empty()) {
            Entry& top = path.back();
            if (top.next < callees[top.node].size()) {
                const uint32_t callee = callees[top.node][top.next++];
                if (order[callee] == kUnvisited) {
                    // The path is a call stack: more than `limit` frames is
                    // too deep. A frame takes a definition and a name entry,
                    // and macros between frames add names; a path of 8 entries
                    // per allowed frame is taken as too deep as well.
                    if (functionsOnPath + (isFunction[callee] ? 1 : 0) >
                            limit ||
                        path.size() >= 8 * limit) {
                        return kSaturated;
                    }
                    enter(callee);
                } else if (component[callee] == kUnvisited) {
                    low[top.node] = std::min(low[top.node], order[callee]);
                }
                continue;
            }
            const uint32_t n = top.node;
            path.pop_back();
            functionsOnPath -= isFunction[n] ? 1 : 0;
            if (!path.empty()) {
                low[path.back().node] = std::min(low[path.back().node], low[n]);
            }
            if (low[n] != order[n]) {
                continue;
            }
            const uint32_t id = static_cast<uint32_t>(componentFrames.size());
            size_t first = members.size();
            do {
                component[members[--first]] = id;
            } while (members[first] != n);
            size_t frames = 0;
            size_t below = 0;
            for (size_t k = first; k < members.size(); k++) {
                frames += isFunction[members[k]] ? 1 : 0;
                for (uint32_t callee : callees[members[k]]) {
                    if (component[callee] != id) {
                        below =
                            std::max(below, componentFrames[component[callee]]);
                    }
                }
            }
            members.resize(first);
            frames += below;
            if (frames > limit) {
                return kSaturated;
            }
            componentFrames.push_back(frames);
            deepest = std::max(deepest, frames);
        }
    }
    return deepest;
}

// The columns and rows a GLSL ES 1.00 variable of `type` takes in the
// Appendix A §7 grid (a mat2 takes two full rows); false for the types not
// counted (samplers, structures).
static bool packingSize(const std::string& type, int64_t& columns,
                        int64_t& rows)
{
    rows = 1;
    columns = 1;
    if (type == "float" || type == "int" || type == "bool") {
        return true;
    }
    const char n = type.empty() ? '\0' : type.back();
    if (n < '2' || n > '4') {
        return false;
    }
    const std::string base = type.substr(0, type.size() - 1);
    if (base == "mat") {
        rows = n - '0';
        columns = n == '3' ? 3 : 4;
        return true;
    }
    columns = n - '0';
    return base == "vec" || base == "ivec" || base == "bvec";
}

// The GLSL ES 1.00 words that stand before the type of a declaration
// (qualifiers), or before a name they do not declare.
static bool isDeclarationPrefix(const std::string& token)
{
    static const char* const kWords[] = {
        "const", "uniform",   "attribute", "varying", "invariant",
        "in",    "out",       "inout",     "lowp",    "mediump",
        "highp", "precision", "struct",    "return",  "else",
    };
    for (const char* word : kWords) {
        if (token == word) {
            return true;
        }
    }
    return false;
}

// The GLSL ES 1.00 Appendix A §4, §5 and §7 rules and the WebGL 1.0 "Maximum
// Nesting of Structures" a driver may let through, checked on `code`
// (shaderSourceForDriver()), where every name the preprocessor may change
// or hide reads `#name`. Only tokens the compiler surely sees break a rule,
// and a name is surely not a constant only if each of its declarations read
// so far surely declares a variable, so a shader fails here only if it
// breaks the rule. The info log, or empty.
static std::string shaderLimitationError(const std::string& code, bool essl3,
                                         bool isVertex, GLint maxUniformVectors,
                                         GLint maxVaryingVectors)
{
    auto tokenEnd = [&code](size_t i) {
        return code[i] == '#' ? identifierEnd(code, i + 1)
                              : std::max(identifierEnd(code, i), i + 1);
    };
    std::vector<size_t> tokens; // where each token starts
    for (size_t i = 0; i < code.size();) {
        if (std::isspace(static_cast<unsigned char>(code[i]))) {
            i++;
            continue;
        }
        tokens.push_back(i);
        i = tokenEnd(i);
    }
    // Token k, empty outside the code.
    auto text = [&](size_t k) {
        return k < tokens.size()
                   ? code.substr(tokens[k], tokenEnd(tokens[k]) - tokens[k])
                   : std::string();
    };
    auto isName = [](const std::string& t) {
        return !t.empty() && isIdentifierCharacter(t[0]) &&
               (t[0] < '0' || t[0] > '9');
    };

    enum : uint8_t {
        kVariable = 1,      // declared without const
        kLoopIndex = 2,     // declared in a for header
        kMaybeConstant = 4, // const, or declared in a way not read here
        kUniform = 8,
        kRedeclared = 16,
    };
    // What the declarations read so far say of each name.
    std::unordered_map<std::string, uint8_t> names;
    auto bitsOf = [&names](const std::string& name) {
        const auto it = names.find(name);
        return it == names.end() ? 0 : it->second;
    };
    std::unordered_set<std::string> used; // names read outside declarations
    std::unordered_map<std::string, int> structDepths;
    struct Variable {
        std::string name;
        int64_t columns;
        int64_t rows;
        bool isUniform;
    };
    // GLSL ES 1.00 Appendix A §7 counts these once a fragment shader reads
    // them.
    std::vector<Variable> packed = { { "gl_FragCoord", 4, 1, false },
                                     { "gl_FrontFacing", 1, 1, false },
                                     { "gl_PointCoord", 2, 1, false } };
    struct Open {
        size_t token;
        int deepestField;
        bool hasVariable;
    };
    std::vector<Open> open;  // the unclosed ( [ {
    size_t cleanFrom = 0;    // the token after the last #name
    uint8_t declaration = 0; // what further declarators of a statement get
    size_t declarationLevel = 0;
    std::string declarationType;
    char storage = 0; // 'u' or 'v' in a uniform or varying declaration

    // Whether token m is a name surely not a constant: every declaration of
    // it read so far, unless `exempt`, is of a variable, and it is no field,
    // call or structure.
    auto isSurelyVariable = [&](size_t m, uint8_t exempt) {
        const uint8_t bits = bitsOf(text(m));
        return (bits & (kVariable | kLoopIndex)) && !(bits & exempt) &&
               text(m - 1) != "." && text(m + 1) != "(" && text(m + 1) != ".";
    };

    for (size_t k = 0; k < tokens.size(); k++) {
        const std::string token = text(k);
        if (token[0] == '#') {
            cleanFrom = k + 1;
            names[token.substr(1)] |= kMaybeConstant;
            // The macro may have started a declaration.
            declaration = kMaybeConstant;
            declarationLevel = open.size();
            continue;
        }
        if (isName(token)) {
            const std::string prev = text(k - 1);
            if (prev == ".") {
                continue; // a field or swizzle
            }
            if (prev[0] == '#') {
                names[token] |= kMaybeConstant;
                continue;
            }
            // The type of a field (GLSL ES 1.00 §4.1.8).
            if ((prev == "{" || prev == ";" || prev == "lowp" ||
                 prev == "mediump" || prev == "highp") &&
                !open.empty() && structDepths.count(token) &&
                !(bitsOf(token) & kMaybeConstant)) {
                open.back().deepestField =
                    std::max(open.back().deepestField, structDepths[token]);
            }
            uint8_t bits = 0;
            if (isName(prev) && !isDeclarationPrefix(prev) &&
                text(k + 1) != "(") {
                // `prev` is the type of a variable or parameter.
                size_t j = k - 2;
                bool isConst = false;
                bool isUniform = false;
                bool isVarying = false;
                for (; isDeclarationPrefix(text(j)); j--) {
                    isConst |= text(j) == "const";
                    isUniform |= text(j) == "uniform";
                    isVarying |= text(j) == "varying";
                }
                const std::string context = text(j);
                const bool isStatement = context.empty() || context == ";" ||
                                         context == "{" || context == "}";
                if (context == "(" && text(j - 1) == "for") {
                    bits = kLoopIndex;
                } else if (isStatement || context == "(" || context == ",") {
                    bits = isConst ? kMaybeConstant
                                   : kVariable | (isUniform ? kUniform : 0);
                } else {
                    bits = kMaybeConstant;
                }
                declaration = isStatement ? bits : 0;
                declarationLevel = open.size();
                declarationType = prev;
                storage = isUniform ? 'u' : isVarying ? 'v' : 0;
            } else if (declaration && (prev == "," || prev == "}") &&
                       open.size() == declarationLevel) {
                bits = declaration; // a further declarator
            } else {
                used.insert(token);
                if (!open.empty() &&
                    isSurelyVariable(k, kMaybeConstant | kLoopIndex)) {
                    open.back().hasVariable = true;
                }
                continue;
            }
            uint8_t& known = names[token];
            known |= bits | (known ? kRedeclared : 0);
            int64_t columns = 0;
            int64_t rows = 0;
            int64_t size = 1;
            if ((declaration & kVariable) && storage &&
                packingSize(declarationType, columns, rows)) {
                if (text(k + 1) == "[") {
                    const std::string length = text(k + 2);
                    size_t i = 0;
                    if (text(k + 3) != "]" ||
                        !readIntegerExpression(length, i, length.size(), 0, 1,
                                               size) ||
                        i != length.size()) {
                        size = 0;
                    }
                }
                if (size > 0) {
                    packed.push_back(
                        { token, columns, rows * size, storage == 'u' });
                }
            }
            continue;
        }
        if (token == ";") {
            declaration = 0;
        }
        if (token == "(" || token == "[" || token == "{") {
            open.push_back({ k, 0, false });
            continue;
        }
        if ((token != ")" && token != "]" && token != "}") || open.empty()) {
            continue;
        }
        const Open o = open.back();
        open.pop_back();
        if (o.hasVariable && !open.empty()) {
            open.back().hasVariable = true;
        }
        const char opener = token == ")" ? '(' : token == "]" ? '[' : '{';
        if (code[tokens[o.token]] != opener) {
            continue;
        }
        const bool isClean = o.token >= cleanFrom;
        if (opener == '[' && isClean && o.hasVariable && !essl3) {
            // Any integer may index a uniform in a vertex shader (Appendix
            // A §5).
            // shortcut: a vertex shader's index is judged only right after a
            // plain name; follow a field, column or call to its root to judge
            // those too.
            const uint8_t target = bitsOf(text(o.token - 1));
            if (!isVertex ||
                (cleanFrom + 1 < o.token && text(o.token - 2) != "." &&
                 (target & kVariable) &&
                 !(target & (kUniform | kMaybeConstant)))) {
                return "Index expression must be a constant-index-expression";
            }
        }
        if (opener == '(' && isClean && text(o.token - 1) == "for" && !essl3) {
            // The loop index is initialized, compared and changed with
            // constant expressions only (Appendix A §4).
            // shortcut: the other §4 forms (no declaration, an assignment in
            // the body) are left to the driver.
            size_t parts[4] = { o.token, 0, 0, k };
            size_t semicolons = 0;
            int depth = 0;
            for (size_t m = o.token + 1; m < k; m++) {
                const std::string t = text(m);
                if (t == "for") {
                    // A syntax error, left to the driver; stopping here also
                    // reads each token in one header only.
                    semicolons = 0;
                    break;
                }
                depth += (t == "(" || t == "[") - (t == ")" || t == "]");
                if (t == ";" && !depth && ++semicolons < 3) {
                    parts[semicolons] = m;
                }
            }
            for (size_t p = 0; semicolons == 2 && p < 3; p++) {
                size_t m = parts[p] + 1;
                while (m < parts[p + 1] && code[tokens[m]] != '<' &&
                       code[tokens[m]] != '>' && code[tokens[m]] != '=' &&
                       code[tokens[m]] != '!') {
                    m++;
                }
                for (m++; m < parts[p + 1]; m++) {
                    if (isSurelyVariable(m, kMaybeConstant)) {
                        return "Loop index can only be initialized, compared "
                               "and changed with constant expressions";
                    }
                }
            }
        }
        const bool isNamedStruct =
            isName(text(o.token - 1)) && text(o.token - 2) == "struct";
        if (opener == '{' && (isNamedStruct || text(o.token - 1) == "struct")) {
            // Unread fields leave a structure at least 1 deep.
            const int depth = isClean ? 1 + o.deepestField : 1;
            if (depth > 4) {
                return "Structures are nested more than 4 levels deep";
            }
            // Its declarators may be const or uniform.
            declaration = kMaybeConstant;
            declarationLevel = open.size();
            if (isNamedStruct) {
                // A name defined again, in an inner scope, counts its
                // shallower definition.
                const auto it =
                    structDepths.emplace(text(o.token - 1), depth).first;
                it->second = std::min(it->second, depth);
            }
        }
    }
    if (essl3) {
        return std::string();
    }

    // Appendix A §7 with the limits getParameter() reports (WebGL 1.0
    // "Packing Restrictions"). As other browsers do, a shader counts the
    // variables it reads and no literal constants, and a fragment shader the
    // varyings it reads, at compile time: the spec counts the varyings both
    // shaders read, at link.
    // shortcut: only conditions the algorithm needs are checked (no variable
    // taller than the grid, 3- and 4-column variables on rows of their own,
    // all registers), so a set that fails only by fragmentation reaches the
    // driver; run the algorithm to catch those.
    auto exceeds = [&](bool uniforms, int64_t maxRows) {
        int64_t wideRows = 0;
        int64_t cells = 0;
        for (const Variable& variable : packed) {
            if (variable.isUniform != uniforms || !used.count(variable.name) ||
                (bitsOf(variable.name) & kRedeclared)) {
                continue;
            }
            if (variable.rows > maxRows) {
                return true;
            }
            wideRows += variable.columns >= 3 ? variable.rows : 0;
            cells += variable.columns * variable.rows;
            if (wideRows > maxRows || cells > 4 * maxRows) {
                return true;
            }
        }
        return false;
    };
    if (maxUniformVectors > 0 && exceeds(true, maxUniformVectors)) {
        return "Uniforms exceed the GLSL ES 1.00 packing limit";
    }
    if (maxVaryingVectors > 0 && exceeds(false, maxVaryingVectors)) {
        return "Varyings exceed the GLSL ES 1.00 packing limit";
    }
    return std::string();
}

void WebGLRenderingContext::compileShader(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(shader)) {
        return;
    }

    shader->setCompileFailed(false);
    shader->setInfoLog("");

    const std::string& src = shader->source();

    // The checks read the source as the preprocessor does, and the driver
    // compiles that same text.
    std::string view;
    if (!preprocessorView(src, view)) {
        shader->setCompileFailed(true);
        shader->setInfoLog("Unterminated comment");
        return;
    }

    // WebGL 1.0 and 2.0 "Maximum GLSL Token Size".
    const size_t maxTokenLength = webGLVersion() >= 2 ? 1024 : 256;
    size_t tokenLen = 0;
    for (char c : view) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') {
            tokenLen++;
            if (tokenLen > maxTokenLength) {
                shader->setCompileFailed(true);
                shader->setInfoLog("Token exceeds " +
                                   std::to_string(maxTokenLength) +
                                   " characters");
                return;
            }
        } else {
            tokenLen = 0;
        }
    }

    if (shaderExpressionDepth(view, kMaxShaderExpressionDepth) >
        kMaxShaderExpressionDepth) {
        shader->setCompileFailed(true);
        shader->setInfoLog("Expression too complex");
        return;
    }

    if (shaderCallDepth(view, kMaxShaderCallDepth) > kMaxShaderCallDepth) {
        shader->setCompileFailed(true);
        shader->setInfoLog("Call stack too deep");
        return;
    }

    std::string driverSource;
    std::string code;
    std::string error;
    if (!shaderSourceForDriver(view, driverSource, code, error)) {
        shader->setCompileFailed(true);
        shader->setInfoLog(error);
        return;
    }

    // WebGL 1.0 and 2.0 "Supported GLSL Constructs": GLSL ES 1.00 is taken,
    // and GLSL ES 3.00 (GLSL ES 3.00 §3.3, "#version 300 es") only by WebGL 2;
    // every other #version directive fails here rather than reach a driver
    // that may accept a newer language. Where it stands is left to the checks
    // below and to the driver.
    for (size_t line = 0; line != std::string::npos;) {
        const size_t end = view.find_first_of("\r\n", line);
        const size_t hash = view.find_first_not_of(" \t\v\f", line);
        if (hash < end && view[hash] == '#') {
            std::istringstream directive(view.substr(hash + 1, end - hash - 1));
            std::string name, number, profile, rest;
            directive >> name >> number >> profile >> rest;
            const bool isSupported =
                rest.empty() &&
                ((number == "100" && profile.empty()) ||
                 (webGLVersion() >= 2 && number == "300" && profile == "es"));
            if (name == "version" && !isSupported) {
                shader->setCompileFailed(true);
                shader->setInfoLog("Unsupported #version directive");
                return;
            }
        }
        line = end == std::string::npos ? end : end + 1;
    }

    GLint shaderType = 0;
    m_gl->getShaderiv(shader->glObject(), GL_SHADER_TYPE, &shaderType);
    const bool essl3 = webGLVersion() >= 2 && isESSL3Source(view);
    const bool isVertex = shaderType != GL_FRAGMENT_SHADER;
    // The GLSL ES 1.00 Appendix A §7 grid has the rows getParameter()
    // reports (WebGL 1.0 "Packing Restrictions").
    GLint maxUniformVectors = 0;
    GLint maxVaryingVectors = 0;
    if (!essl3) {
        m_gl->getIntegerv(isVertex ? GL_MAX_VERTEX_UNIFORM_VECTORS
                                   : GL_MAX_FRAGMENT_UNIFORM_VECTORS,
                          &maxUniformVectors);
        if (!isVertex) {
            m_gl->getIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVectors);
        }
    }
    error = shaderLimitationError(code, essl3, isVertex, maxUniformVectors,
                                  maxVaryingVectors);
    if (!error.empty()) {
        shader->setCompileFailed(true);
        shader->setInfoLog(error);
        return;
    }

    if (webGLVersion() >= 2) {
        // 1. Misplaced version directive: #version 300 es must be on line 1,
        // column 0. This stays on the raw source: it is a byte-0 check, and
        // the view's comment and continuation folding would move the column.
        size_t versionPos = src.find("#version 300 es");
        if (versionPos != std::string::npos && versionPos != 0) {
            shader->setCompileFailed(true);
            shader->setInfoLog("#version directive must be on the first line");
            return;
        }

        // 2. textureOffset out of range: offset must be in [-8, 7]. Read on the
        // view so a comment or a line continuation in the call cannot hide the
        // offset from this scan while the driver still sees it.
        size_t texOffsetPos = 0;
        while ((texOffsetPos = view.find("textureOffset", texOffsetPos)) !=
               std::string::npos) {
            size_t openParen = view.find('(', texOffsetPos);
            if (openParen != std::string::npos) {
                int depth = 1;
                size_t closeParen = openParen + 1;
                while (closeParen < view.length() && depth > 0) {
                    if (view[closeParen] == '(') {
                        depth++;
                    } else if (view[closeParen] == ')') {
                        depth--;
                    }
                    closeParen++;
                }
                if (depth == 0) {
                    std::string callStr =
                        view.substr(openParen, closeParen - openParen);
                    size_t ivecPos = callStr.find("ivec");
                    if (ivecPos != std::string::npos) {
                        size_t comma = callStr.find(',', ivecPos);
                        if (comma != std::string::npos) {
                            const char* numStr = callStr.c_str() + comma + 1;
                            while (*numStr == ' ' || *numStr == '\t') {
                                numStr++;
                            }
                            if (*numStr == '-' ||
                                (*numStr >= '0' && *numStr <= '9')) {
                                char* endPtr = nullptr;
                                long val = strtol(numStr, &endPtr, 10);
                                if (val < -8 || val > 7) {
                                    shader->setCompileFailed(true);
                                    shader->setInfoLog(
                                        "Texture offset out of valid range "
                                        "[-8, 7]");
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            texOffsetPos += 13;
        }

        // 3. Default zero initialization for vertex shader outputs:
        // WebGL 2.0 Transform Feedback specification requires that unwritten
        // output variables default to zero.
        if (shaderType == GL_VERTEX_SHADER &&
            driverSource.find("#version 300 es") != std::string::npos) {
            std::string& compiledSrc = driverSource;
            std::string inits;
            size_t pos = 0;
            while ((pos = compiledSrc.find("out ", pos)) != std::string::npos) {
                if (pos == 0 || isspace(compiledSrc[pos - 1]) ||
                    compiledSrc[pos - 1] == ';') {
                    size_t typeStart = pos + 4;
                    while (typeStart < compiledSrc.length() &&
                           isspace(compiledSrc[typeStart])) {
                        typeStart++;
                    }
                    size_t typeEnd = typeStart;
                    while (typeEnd < compiledSrc.length() &&
                           (isalnum(compiledSrc[typeEnd]) ||
                            compiledSrc[typeEnd] == '_')) {
                        typeEnd++;
                    }
                    std::string type =
                        compiledSrc.substr(typeStart, typeEnd - typeStart);

                    size_t nameStart = typeEnd;
                    while (nameStart < compiledSrc.length() &&
                           isspace(compiledSrc[nameStart])) {
                        nameStart++;
                    }
                    size_t nameEnd = nameStart;
                    while (nameEnd < compiledSrc.length() &&
                           (isalnum(compiledSrc[nameEnd]) ||
                            compiledSrc[nameEnd] == '_')) {
                        nameEnd++;
                    }
                    std::string name =
                        compiledSrc.substr(nameStart, nameEnd - nameStart);

                    size_t semi = compiledSrc.find(';', nameEnd);
                    if (semi != std::string::npos && !type.empty() &&
                        !name.empty()) {
                        size_t prevParen = compiledSrc.rfind('(', pos);
                        size_t nextParen = compiledSrc.find(')', pos);
                        if (prevParen == std::string::npos ||
                            nextParen == std::string::npos ||
                            prevParen > nextParen) {
                            std::string zeroVal;
                            if (type.rfind("vec", 0) == 0 ||
                                type.rfind("mat", 0) == 0) {
                                zeroVal = type + "(0.0)";
                            } else if (type.rfind("ivec", 0) == 0) {
                                zeroVal = type + "(0)";
                            } else if (type.rfind("uvec", 0) == 0) {
                                zeroVal = type + "(0u)";
                            } else if (type == "float") {
                                zeroVal = "0.0";
                            } else if (type == "int") {
                                zeroVal = "0";
                            } else if (type == "uint") {
                                zeroVal = "0u";
                            }
                            if (!zeroVal.empty()) {
                                // On the line of main's brace, keeping
                                // __LINE__ and log line numbers (GLSL ES 3.00
                                // §3.5).
                                inits += " " + name + " = " + zeroVal + ";";
                            }
                        }
                    }
                }
                pos += 4;
            }

            if (!inits.empty()) {
                size_t mainPos = compiledSrc.find("main");
                if (mainPos != std::string::npos) {
                    size_t openBrace = compiledSrc.find('{', mainPos);
                    if (openBrace != std::string::npos) {
                        compiledSrc.insert(openBrace + 1, inits);
                    }
                }
            }
        }
    }

    adjustShaderSourceForDriver(driverSource);
    const char* sourceArray[1] = { driverSource.c_str() };
    m_gl->shaderSource(shader->glObject(), 1, sourceArray, nullptr);
    m_gl->compileShader(shader->glObject());
}

// A copy rectangle reaching past INT32_MAX lies outside every framebuffer;
// moving it back to end there reads the same nothing and keeps origin + size
// from wrapping in the driver's clipping.
static GLint copyOriginForGL(GLint origin, GLsizei size)
{
    return static_cast<GLint>(
        std::min<int64_t>(origin, int64_t(INT32_MAX) - size));
}

static inline bool isPowerOfTwo(size_t value)
{
    return value && !(value & (value - 1));
}

// The formats WEBGL_depth_texture adds to WebGL 1.
static bool isDepthTextureFormat(GLenum format)
{
    return format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL;
}

void WebGLRenderingContext::copyTexImage2D(GLenum target, GLint level,
                                           GLenum internalformat, GLint x,
                                           GLint y, GLsizei width,
                                           GLsizei height, GLint border)
{
    ENTER_CONTEXT_SCOPE();
    completePendingJobs();

    if (target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                                    target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) {
        setGLError(GL_INVALID_ENUM, "Invalid texture target");
        return;
    }

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0");
        return;
    }

    if (level < 0 || width < 0 || height < 0) {
        setGLError(GL_INVALID_VALUE, "negative level or dimension");
        return;
    }

    if (!isWebGL2RenderingContext() && level > 0 &&
        (!isPowerOfTwo(static_cast<size_t>(width)) ||
         !isPowerOfTwo(static_cast<size_t>(height)))) {
        setGLError(GL_INVALID_VALUE,
                   "level > 0 requires power of two dimensions in WebGL 1.0");
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (!texture || (m_boundTextures.find(target) == m_boundTextures.end() &&
                     !isBoundCubeMapTexture(target))) {
        setGLError(GL_INVALID_OPERATION, "No texture bound to target");
        return;
    }

    if (texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }

    if (internalformat == 0x8054 /* GL_RGB16 */ ||
        internalformat == GL_R8_SNORM || internalformat == GL_RG8_SNORM ||
        internalformat == GL_RGB8_SNORM || internalformat == GL_RGBA8_SNORM) {
        setGLError(GL_INVALID_ENUM, "Invalid internal format");
        return;
    }
    // WEBGL_depth_texture: no copies into a depth texture.
    if (webGLVersion() == 1 && isDepthTextureFormat(internalformat)) {
        setGLError(isExtensionEnabled("WEBGL_depth_texture")
                       ? GL_INVALID_OPERATION
                       : GL_INVALID_ENUM,
                   "Invalid internal format");
        return;
    }

    if (!validateFramebufferComplete(GL_READ_FRAMEBUFFER)) {
        return;
    }

    if (copiesIntoReadBuffer(texture, target, level)) {
        return;
    }

    // Check alpha compatibility with read framebuffer
    if (internalformat == GL_RGBA || internalformat == GL_RGBA4 ||
        internalformat == GL_RGB5_A1 || internalformat == GL_RGBA8) {
        GLenum srcFormat = 0;
        Optional<WebGLFramebuffer*> maybeFb =
            getBoundFramebuffer(GL_READ_FRAMEBUFFER);
        if (maybeFb.hasValue() && maybeFb.value()) {
            WebGLFramebuffer* fb = maybeFb.value();
            size_t readAttachmentIndex = 0;
            GLenum rb = fb->readBuffer();
            if (rb >= GL_COLOR_ATTACHMENT0 && rb < GL_COLOR_ATTACHMENT0 + 16) {
                readAttachmentIndex = rb - GL_COLOR_ATTACHMENT0;
            }
            srcFormat = fb->getColorAttachmentFormat(readAttachmentIndex);
        } else {
            if (!m_attributes.alpha()) {
                srcFormat = GL_RGB;
            }
        }
        if (srcFormat == GL_RGB565 || srcFormat == GL_RGB ||
            srcFormat == GL_RGB8) {
            setGLError(GL_INVALID_OPERATION,
                       "Cannot copy non-alpha source to alpha destination");
            return;
        }
    }

    // The GL validates and defines the level first.
    m_gl->copyTexImage2D(target, level, internalformat,
                         copyOriginForGL(x, width), copyOriginForGL(y, height),
                         width, height, border);
    if (!hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   1);
        // WebGL 1.0 "Reading Pixels Outside the Framebuffer": texels for
        // pixels outside the read framebuffer are 0.
        GLsizei fbWidth = 0;
        GLsizei fbHeight = 0;
        GLenum format = 0;
        GLenum type = 0;
        if (readFramebufferSize(fbWidth, fbHeight) &&
            Pixel::formatAndTypeOf(internalformat, format, type)) {
            const int64_t x0 = std::max<int64_t>(x, 0);
            const int64_t y0 = std::max<int64_t>(y, 0);
            const int64_t x1 = std::min<int64_t>(int64_t(x) + width, fbWidth);
            const int64_t y1 = std::min<int64_t>(int64_t(y) + height, fbHeight);
            if (x0 != x || y0 != y || x1 != int64_t(x) + width ||
                y1 != int64_t(y) + height) {
                zeroTextureLevel(target, level, width, height, 1, format, type);
                if (x1 > x0 && y1 > y0) {
                    m_gl->copyTexSubImage2D(target, level, x0 - x, y0 - y, x0,
                                            y0, x1 - x0, y1 - y0);
                }
            }
        }
    }
}

bool WebGLRenderingContext::copiesIntoReadBuffer(const WebGLTexture* texture,
                                                 GLenum target, GLint level)
{
    // WebGL 1.0 "Feedback Loops Between Textures and the Framebuffer",
    // OpenGL ES 3.0.6 §4.4.3.2.
    WebGLFramebuffer* readFb =
        getBoundFramebuffer(GL_READ_FRAMEBUFFER).valueOrNull();
    if (readFb && readFb->readBufferIs(texture, target, level, -1)) {
        setGLError(GL_INVALID_OPERATION,
                   "the destination is the image the read buffer reads");
        return true;
    }
    return false;
}

bool WebGLRenderingContext::readFramebufferSize(GLsizei& width, GLsizei& height)
{
    Optional<WebGLFramebuffer*> maybeFb =
        getBoundFramebuffer(GL_READ_FRAMEBUFFER);
    if (!maybeFb.hasValue() || !maybeFb.value()) {
        width = drawingBufferWidth();
        height = drawingBufferHeight();
        return true;
    }
    WebGLFramebuffer* fb = maybeFb.value();
    size_t readAttachmentIndex = 0;
    GLenum rb = fb->readBuffer();
    if (rb >= GL_COLOR_ATTACHMENT0 && rb < GL_COLOR_ATTACHMENT0 + 16) {
        readAttachmentIndex = rb - GL_COLOR_ATTACHMENT0;
    }
    return fb->getColorAttachmentSize(readAttachmentIndex, width, height);
}

void WebGLRenderingContext::copyTexSubImage2D(GLenum target, GLint level,
                                              GLint xoffset, GLint yoffset,
                                              GLint x, GLint y, GLsizei width,
                                              GLsizei height)
{
    ENTER_CONTEXT_SCOPE();
    completePendingJobs();

    if (target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                                    target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) {
        setGLError(GL_INVALID_ENUM, "Invalid texture target");
        return;
    }

    if (level < 0 || xoffset < 0 || yoffset < 0 || width < 0 || height < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset or dimension");
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (!texture || (m_boundTextures.find(target) == m_boundTextures.end() &&
                     !isBoundCubeMapTexture(target))) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    // WEBGL_depth_texture: no copies into a depth texture.
    const TextureImageLevelInfo* destination =
        texture->getImageLevelInfo(target, level);
    if (webGLVersion() == 1 && destination &&
        isDepthTextureFormat(destination->internalFormat)) {
        setGLError(GL_INVALID_OPERATION, "the texture is a depth texture");
        return;
    }

    if (!validateFramebufferComplete(GL_READ_FRAMEBUFFER)) {
        return;
    }

    if (copiesIntoReadBuffer(texture, target, level)) {
        return;
    }

    // Check alpha compatibility with read framebuffer
    GLenum srcFormat = 0;
    Optional<WebGLFramebuffer*> maybeFb =
        getBoundFramebuffer(GL_READ_FRAMEBUFFER);
    if (maybeFb.hasValue() && maybeFb.value()) {
        WebGLFramebuffer* fb = maybeFb.value();
        size_t readAttachmentIndex = 0;
        GLenum rb = fb->readBuffer();
        if (rb >= GL_COLOR_ATTACHMENT0 && rb < GL_COLOR_ATTACHMENT0 + 16) {
            readAttachmentIndex = rb - GL_COLOR_ATTACHMENT0;
        }
        srcFormat = fb->getColorAttachmentFormat(readAttachmentIndex);
    } else {
        if (!m_attributes.alpha()) {
            srcFormat = GL_RGB;
        }
    }
    if (srcFormat == GL_RGB565 || srcFormat == GL_RGB || srcFormat == GL_RGB8) {
        GLenum dstFormat = 0;
        const auto* info = texture->getImageLevelInfo(target, level);
        if (info) {
            dstFormat = info->internalFormat;
        }
        // A WebGL 1 RGBA level allocated without data is kept as BGRA
        // (texImage2D, PORT_PIXEL_ORDER_BGRA).
        if (dstFormat == GL_RGBA || dstFormat == GL_RGBA4 ||
            dstFormat == GL_RGB5_A1 || dstFormat == GL_RGBA8 ||
            dstFormat == GL_BGRA_EXT || dstFormat == 0) {
            setGLError(GL_INVALID_OPERATION,
                       "Cannot copy non-alpha source to alpha destination");
            return;
        }
    }

    m_gl->copyTexSubImage2D(target, level, xoffset, yoffset,
                            copyOriginForGL(x, width),
                            copyOriginForGL(y, height), width, height);
}

WebGLBuffer* WebGLRenderingContext::createBuffer()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint buffer = 0;
    m_gl->genBuffers(1, &buffer);
    return new WebGLBuffer(scriptBindingInstance(), this, buffer);
}

WebGLFramebuffer* WebGLRenderingContext::createFramebuffer()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint fbo = 0;
    m_gl->genFramebuffers(1, &fbo);
    return new WebGLFramebuffer(scriptBindingInstance(), this, fbo);
}

WebGLProgram* WebGLRenderingContext::createProgram()
{
    ENTER_CONTEXT_SCOPE(nullptr);
    return new WebGLProgram(scriptBindingInstance(), this,
                            m_gl->createProgram());
}

WebGLRenderbuffer* WebGLRenderingContext::createRenderbuffer()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint rbo = 0;
    m_gl->genRenderbuffers(1, &rbo);
    return new WebGLRenderbuffer(scriptBindingInstance(), this, rbo);
}

WebGLShader* WebGLRenderingContext::createShader(unsigned long type)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (type != GL_VERTEX_SHADER && type != GL_FRAGMENT_SHADER) {
        setGLError(GL_INVALID_ENUM);
        return nullptr;
    }
    return new WebGLShader(scriptBindingInstance(), this,
                           m_gl->createShader(type));
}

WebGLTexture* WebGLRenderingContext::createTexture()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint textureId = 0;
    m_gl->genTextures(1, &textureId);
    return new WebGLTexture(scriptBindingInstance(), this, textureId);
}

void WebGLRenderingContext::cullFace(GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->cullFace(mode);
}

void WebGLRenderingContext::deleteBuffer(Optional<WebGLBuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE();
    if (maybe.hasValue()) {
        WebGLBuffer* value = maybe.value();
        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        if (value->isDeleted()) {
            return;
        }
        GLuint buffer = value->glObject();
        m_gl->deleteBuffers(1, &buffer);
        value->markDeleted();
        m_state->unbindBuffer(value);
    }
}

void WebGLRenderingContext::deleteFramebuffer(Optional<WebGLFramebuffer*> maybe)
{
    // Deleting a bound framebuffer reverts that binding to the default
    // framebuffer (WebGL 1.0 §5.14.6), which here is an FBO of its own rather
    // than GL's name 0. bindFramebuffer() also resets WebGL 2's read/draw
    // state; it opens its own context scope, so it runs before ours.
    if (maybe && isFromCurrentContext(maybe.value()) &&
        !maybe.value()->isDeleted()) {
        for (GLenum target : { GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER }) {
            Optional<WebGLFramebuffer*> bound = getBoundFramebuffer(target);
            if (bound && bound.value() == maybe.value()) {
                bindFramebuffer(webGLVersion() == 1 ? GL_FRAMEBUFFER : target,
                                nullptr);
            }
        }
    }

    ENTER_CONTEXT_SCOPE();
    if (maybe.hasValue()) {
        WebGLFramebuffer* value = maybe.value();
        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        if (value->isDeleted()) {
            return;
        }
        GLuint framebuffer = value->glObject();
        m_gl->deleteFramebuffers(1, &framebuffer);
        value->markDeleted();
    }
}

void WebGLRenderingContext::deleteRenderbuffer(
    Optional<WebGLRenderbuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE();
    if (maybe.hasValue()) {
        WebGLRenderbuffer* value = maybe.value();
        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        if (value->isDeleted()) {
            return;
        }
        // The driver drops it from the DEPTH and STENCIL points too, where a
        // WebGL 1 DEPTH_STENCIL_ATTACHMENT image hides the WebGL 1 DEPTH or
        // STENCIL one.
        WebGLFramebuffer* webGL1Fb =
            isWebGL2RenderingContext()
                ? nullptr
                : getBoundFramebuffer(GL_FRAMEBUFFER).valueOrNull();
        const bool restore =
            webGL1Fb && webGL1Fb->attachedRenderbuffer(
                            GL_DEPTH_STENCIL_ATTACHMENT) == value;
        GLuint buffer = value->glObject();
        m_gl->deleteRenderbuffers(1, &buffer);
        value->markDeleted();

        // Deleting detaches it from the bound draw and read framebuffers
        // (OpenGL ES 3.0 §4.4.2.3).
        for (GLenum target : { GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER }) {
            Optional<WebGLFramebuffer*> boundFb = getBoundFramebuffer(target);
            if (boundFb) {
                boundFb.value()->detachRenderbuffer(value);
            }
        }
        if (restore) {
            attachWebGL1DepthStencil(webGL1Fb, true, true);
        }
    }
}

void WebGLRenderingContext::deleteTexture(Optional<WebGLTexture*> maybe)
{
    ENTER_CONTEXT_SCOPE();
    if (maybe.hasValue()) {
        WebGLTexture* value = maybe.value();
        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        if (!value->isDeleted()) {
            // As in deleteRenderbuffer().
            WebGLFramebuffer* webGL1Fb =
                isWebGL2RenderingContext()
                    ? nullptr
                    : getBoundFramebuffer(GL_FRAMEBUFFER).valueOrNull();
            const bool restore =
                webGL1Fb &&
                webGL1Fb->attachedTexture(GL_DEPTH_STENCIL_ATTACHMENT) == value;
            GLuint buffer = value->glObject();
            m_gl->deleteTextures(1, &buffer);
            value->markDeleted();

            for (GLenum target : { GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER }) {
                Optional<WebGLFramebuffer*> boundFb =
                    getBoundFramebuffer(target);
                if (boundFb) {
                    boundFb.value()->detachTexture(value);
                }
            }
            if (restore) {
                attachWebGL1DepthStencil(webGL1Fb, true, true);
            }

            for (auto it = m_boundTextureObjects.begin();
                 it != m_boundTextureObjects.end();) {
                if (it->second == value) {
                    it = m_boundTextureObjects.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = m_boundTextures.begin();
                 it != m_boundTextures.end();) {
                if (it->second == value->glObject()) {
                    it = m_boundTextures.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

void WebGLRenderingContext::deleteShader(Optional<WebGLShader*> maybe)
{
    ENTER_CONTEXT_SCOPE();
    if (!maybe) {
        return;
    }
    WebGLShader* shader = maybe.value();
    if (!isFromCurrentContext(shader)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (shader->isDeleted()) {
        return;
    }
    // GL keeps an attached shader alive until it is detached; attachCount()
    // mirrors that (OpenGL ES 2.0 §2.10.1).
    m_gl->deleteShader(shader->glObject());
    shader->markDeleted();
}

void WebGLRenderingContext::deleteProgram(Optional<WebGLProgram*> maybe)
{
    ENTER_CONTEXT_SCOPE();
    if (!maybe) {
        return;
    }
    WebGLProgram* program = maybe.value();
    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (program->isDeleted()) {
        return;
    }
    program->markDeleted();
    // The current program stays in use until replaced (OpenGL ES 2.0
    // §2.10.3). It reaches glDeleteProgram only then, so an internal
    // useProgram (video uploads) cannot free it behind the page's back.
    if (m_state->webGLProgram() != program) {
        releaseProgram(program);
    }
}

void WebGLRenderingContext::releaseProgram(WebGLProgram* program)
{
    // Freeing a program detaches its shaders (OpenGL ES 2.0 §2.10.3).
    for (WebGLShader* shader : program->getWebGLShaders()) {
        program->removeDetachedShader(shader);
        shader->decrementAttachCount();
    }
    m_gl->deleteProgram(program->glObject());
}

void WebGLRenderingContext::depthFunc(GLenum func)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->depthFunc(func);
}

void WebGLRenderingContext::depthMask(GLboolean flag)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->depthMask(flag);
}

void WebGLRenderingContext::depthRange(GLclampf zNear, GLclampf zFar)
{
    ENTER_CONTEXT_SCOPE();

    // WebGL 1.0 "Viewport Depth Range": zNear must not exceed zFar.
    if (zNear > zFar) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->depthRangef(zNear, zFar);
}

void WebGLRenderingContext::detachShader(WebGLProgram* program,
                                         WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(program) || !validateProgramOrShader(shader)) {
        return;
    }

    hasNewGLError(); // Keep an earlier error from failing this call.
    m_gl->detachShader(program->glObject(), shader->glObject());
    if (hasNewGLError()) {
        return;
    }
    program->removeDetachedShader(shader);
    shader->decrementAttachCount();
}

static bool isValidEnableCap(GLenum cap, bool isWebGL2)
{
    switch (cap) {
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
        return true;
    case GL_RASTERIZER_DISCARD:
        return isWebGL2;
    default:
        return false;
    }
}

void WebGLRenderingContext::disable(GLenum cap)
{
    ENTER_CONTEXT_SCOPE();

    if (!isValidEnableCap(cap, isWebGL2RenderingContext())) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    m_gl->disable(cap);
}

void WebGLRenderingContext::disableVertexAttribArray(GLuint index)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->disableVertexAttribArray(index);
    if (hasNewGLError()) {
        return;
    }
    getState()->disableVertexAttribArray(index);
}

bool WebGLRenderingContext::isValidDrawMode(GLenum mode)
{
    return mode == GL_POINTS || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP ||
           mode == GL_LINES || mode == GL_TRIANGLE_STRIP ||
           mode == GL_TRIANGLE_FAN || mode == GL_TRIANGLES;
}

void WebGLRenderingContext::vertexAttribDivisor(GLuint index, GLuint divisor)
{
    ENTER_CONTEXT_SCOPE();
    m_gl->vertexAttribDivisor(index, divisor);
    if (hasNewGLError()) {
        return;
    }
    m_state->setVertexAttribDivisor(index, divisor);
}

bool WebGLRenderingContext::hasOnlyNonZeroAttribDivisors()
{
    // Only arrays the program actually reads matter: an array can be left
    // enabled at a location this program does not use, and counting it would
    // reject a legal draw.
    GLint program = getCurrentProgram();
    if (!program) {
        return false;
    }

    GLint activeAttribCount = 0;
    m_gl->getProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttribCount);
    if (activeAttribCount <= 0) {
        return false;
    }

    GLint maxNameLength = 0;
    m_gl->getProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);
    if (maxNameLength <= 0) {
        return false;
    }
    std::vector<char> name(static_cast<size_t>(maxNameLength) + 1);

    bool sawEnabledArray = false;
    for (GLint i = 0; i < activeAttribCount; ++i) {
        GLsizei nameLength = 0;
        GLint size = 0;
        GLenum type = 0;
        m_gl->getActiveAttrib(program, static_cast<GLuint>(i), maxNameLength,
                              &nameLength, &size, &type, name.data());
        name[static_cast<size_t>(nameLength)] = '\0';

        int location = m_gl->getAttribLocation(program, name.data());
        if (location < 0) {
            // Built-ins such as gl_VertexID have no array behind them.
            continue;
        }

        GLint enabled = 0;
        m_gl->getVertexAttribiv(static_cast<GLuint>(location),
                                GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        if (!enabled) {
            continue;
        }

        GLint divisor = 0;
        m_gl->getVertexAttribiv(static_cast<GLuint>(location),
                                GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
        if (divisor == 0) {
            return false;
        }
        sawEnabledArray = true;
    }
    return sawEnabledArray;
}

bool WebGLRenderingContext::hasEnoughElementArrayData(GLsizei count,
                                                      GLenum type,
                                                      WebGLintptr offset)
{
    if (count == 0) {
        // Nothing is fetched, so the offset need not be inside the buffer.
        return true;
    }

    int64_t indexSize = 0;
    switch (type) {
    case GL_UNSIGNED_BYTE:
        indexSize = 1;
        break;
    case GL_UNSIGNED_SHORT:
        indexSize = 2;
        break;
    case GL_UNSIGNED_INT:
        indexSize = 4;
        break;
    default:
        // An unknown type is GL's to reject.
        return true;
    }

    Optional<WebGLBuffer*> elementBuffer =
        getState()->getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);
    if (!elementBuffer.hasValue() || !elementBuffer.value()) {
        // The missing binding is reported separately.
        return true;
    }

    // count is a GLsizei, so the byte count needs the wider type. The caller
    // rejected a negative offset.
    return isRangeWithin(offset, static_cast<uint64_t>(count) * indexSize,
                         elementBuffer.value()->byteLength());
}

bool WebGLRenderingContext::hasEnoughVertexArrayData(int64_t vertexCount,
                                                     GLsizei instanceCount)
{
    // Only arrays the program reads are fetched, so only they can run past
    // their buffer.
    GLint program = getCurrentProgram();
    if (!program) {
        return true;
    }

    GLint activeAttribCount = 0;
    m_gl->getProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttribCount);
    GLint maxNameLength = 0;
    m_gl->getProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);
    if (activeAttribCount <= 0 || maxNameLength <= 0) {
        return true;
    }
    std::vector<char> name(static_cast<size_t>(maxNameLength) + 1);

    for (GLint i = 0; i < activeAttribCount; ++i) {
        GLsizei nameLength = 0;
        GLint size = 0;
        GLenum type = 0;
        m_gl->getActiveAttrib(program, static_cast<GLuint>(i), maxNameLength,
                              &nameLength, &size, &type, name.data());
        name[static_cast<size_t>(nameLength)] = '\0';

        int location = m_gl->getAttribLocation(program, name.data());
        if (location < 0) {
            continue;
        }

        // A matrix attribute takes one location per column.
        GLint columns = 1;
        switch (type) {
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT2x4:
            columns = 2;
            break;
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT3x4:
            columns = 3;
            break;
        case GL_FLOAT_MAT4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT4x3:
            columns = 4;
            break;
        default:
            break;
        }

        for (GLint column = 0; column < columns; ++column) {
            GLuint index = static_cast<GLuint>(location + column);
            if (!m_state->isVertexAttribArrayEnabled(index)) {
                continue;
            }
            WebGLRenderingContextState::AttribPointer pointer =
                m_state->getVertexAttribPointer(index);
            if (!pointer.buffer) {
                // The missing buffer is reported separately.
                continue;
            }

            int64_t elementBytes = 0;
            switch (pointer.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
                elementBytes = pointer.size;
                break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_HALF_FLOAT:
                elementBytes = static_cast<int64_t>(pointer.size) * 2;
                break;
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
                elementBytes = static_cast<int64_t>(pointer.size) * 4;
                break;
            case GL_INT_2_10_10_10_REV:
            case GL_UNSIGNED_INT_2_10_10_10_REV:
                elementBytes = 4;
                break;
            default:
                // An unknown type is GL's to reject.
                continue;
            }

            // How many elements of this array the draw fetches. An instanced
            // array advances once per divisor instances, from instance 0.
            int64_t elements = 0;
            if (pointer.divisor == 0) {
                if (vertexCount < 0) {
                    continue;
                }
                elements = vertexCount;
            } else {
                elements = (static_cast<int64_t>(instanceCount) +
                            pointer.divisor - 1) /
                           pointer.divisor;
            }
            if (elements <= 0) {
                continue;
            }

            // vertexAttribPointer rejected a negative offset.
            int64_t stride = pointer.stride ? pointer.stride : elementBytes;
            if (!isRangeWithin(pointer.offset,
                               (elements - 1) * stride + elementBytes,
                               pointer.buffer.value()->byteLength())) {
                return false;
            }
        }
    }
    return true;
}

void WebGLRenderingContext::drawArraysInstanced(GLenum mode, GLint first,
                                                GLsizei count,
                                                GLsizei instanceCount)
{
    ENTER_CONTEXT_SCOPE();

    if (!isValidDrawMode(mode)) {
        setGLError(GL_INVALID_ENUM, "invalid draw mode");
        return;
    }

    if (first < 0 || count < 0 || instanceCount < 0) {
        setGLError(GL_INVALID_VALUE, "negative count or offset");
        return;
    }

    const uint32_t enabled = getState()->arraysEnabled();
    for (GLuint array = 0;
         array < WebGLRenderingContextState::kMaxVertexAttribs; ++array) {
        if ((enabled & (1u << array)) &&
            !m_state->getBufferBoundToVertexAttributes(array)) {
            setGLError(GL_INVALID_OPERATION,
                       "drawArraysInstanced: vertex attribute enabled without "
                       "bound buffer");
            return;
        }
    }

    // ANGLE_instanced_arrays keeps the ES 2.0 rule that something must still
    // advance per vertex; ES 3.0 dropped it, so WebGL 2 allows this draw.
    if (!isWebGL2RenderingContext() && hasOnlyNonZeroAttribDivisors()) {
        setGLError(GL_INVALID_OPERATION,
                   "all enabled vertex attribute arrays have a non-zero "
                   "divisor");
        return;
    }

    // WebGL 1.0 §6.37, also for a draw of nothing.
    if (!m_state->webGLProgram()) {
        setGLError(GL_INVALID_OPERATION, "no current program");
        return;
    }
    if (!validateFramebufferComplete(GL_DRAW_FRAMEBUFFER)) {
        return;
    }

    if (count > 0 && instanceCount > 0 &&
        !hasEnoughVertexArrayData(static_cast<int64_t>(first) + count,
                                  instanceCount)) {
        setGLError(GL_INVALID_OPERATION,
                   "drawArraysInstanced: vertices or instances reach past "
                   "the bound ARRAY_BUFFER");
        return;
    }

    completePendingJobs();
    if (!validateDrawCallUBO() || !validateDrawCallVertexAttribs() ||
        !validateStencilSettings() || !validateFeedbackLoops())
        return;
    const bool standIns = bindIncompleteTextureStandIns();
    m_gl->drawArraysInstanced(mode, first, count, instanceCount);
    if (standIns) {
        restoreSampledTextureBindings();
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::drawElementsInstanced(GLenum mode, GLsizei count,
                                                  GLenum type,
                                                  WebGLintptr offset,
                                                  GLsizei instanceCount)
{
    ENTER_CONTEXT_SCOPE();

    if (!isValidDrawMode(mode)) {
        setGLError(GL_INVALID_ENUM, "invalid draw mode");
        return;
    }

    bool isUintAllowed =
        (webGLVersion() >= 2) || isExtensionEnabled("OES_element_index_uint");
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT &&
        (!isUintAllowed || type != GL_UNSIGNED_INT)) {
        setGLError(GL_INVALID_ENUM, "invalid type");
        return;
    }

    if (count < 0 || offset < 0 || instanceCount < 0) {
        setGLError(GL_INVALID_VALUE, "negative count or offset");
        return;
    }

    if ((type == GL_UNSIGNED_SHORT && (offset % 2) != 0) ||
        (type == GL_UNSIGNED_INT && (offset % 4) != 0)) {
        setGLError(GL_INVALID_OPERATION, "offset not properly aligned");
        return;
    }

    const uint32_t enabled = getState()->arraysEnabled();
    for (GLuint array = 0;
         array < WebGLRenderingContextState::kMaxVertexAttribs; ++array) {
        if ((enabled & (1u << array)) &&
            !m_state->getBufferBoundToVertexAttributes(array)) {
            setGLError(GL_INVALID_OPERATION,
                       "drawElementsInstanced: vertex attribute enabled "
                       "without bound buffer");
            return;
        }
    }

    // ANGLE_instanced_arrays keeps the ES 2.0 rule that something must still
    // advance per vertex; ES 3.0 dropped it, so WebGL 2 allows this draw.
    if (!isWebGL2RenderingContext() && hasOnlyNonZeroAttribDivisors()) {
        setGLError(GL_INVALID_OPERATION,
                   "all enabled vertex attribute arrays have a non-zero "
                   "divisor");
        return;
    }

    Optional<WebGLBuffer*> elementBuffer =
        getState()->getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);
    if (!elementBuffer.hasValue() || !elementBuffer.value()) {
        setGLError(GL_INVALID_OPERATION,
                   "drawElementsInstanced: no buffer bound to "
                   "ELEMENT_ARRAY_BUFFER");
        return;
    }

    // OpenGL ES 3.0.6 §2.15.2.
    if (isTransformFeedbackActiveUnpaused()) {
        setGLError(GL_INVALID_OPERATION,
                   "transform feedback is active and not paused");
        return;
    }
    // WebGL 1.0 §6.37, also for a draw of nothing.
    if (!m_state->webGLProgram()) {
        setGLError(GL_INVALID_OPERATION, "no current program");
        return;
    }
    if (!validateFramebufferComplete(GL_DRAW_FRAMEBUFFER)) {
        return;
    }

    // A draw of zero instances fetches nothing.
    if (instanceCount > 0 && !hasEnoughElementArrayData(count, type, offset)) {
        setGLError(GL_INVALID_OPERATION,
                   "drawElementsInstanced: indices reach past "
                   "ELEMENT_ARRAY_BUFFER");
        return;
    }

    if (count > 0 && instanceCount > 0 &&
        !hasEnoughVertexArrayData(-1, instanceCount)) {
        setGLError(GL_INVALID_OPERATION,
                   "drawElementsInstanced: instances reach past the bound "
                   "ARRAY_BUFFER");
        return;
    }

    completePendingJobs();
    if (!validateDrawCallUBO() || !validateDrawCallVertexAttribs() ||
        !validateStencilSettings() || !validateFeedbackLoops())
        return;
    const bool standIns = bindIncompleteTextureStandIns();
    m_gl->drawElementsInstanced(
        mode, count, type,
        reinterpret_cast<const void*>(clampToGLintptr(offset)), instanceCount);
    if (standIns) {
        restoreSampledTextureBindings();
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    if (first < 0) {
        // If first is negative, an INVALID_VALUE error will be generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!getCurrentProgram()) {
        // If the CURRENT_PROGRAM is null, an INVALID_OPERATION error will be
        // generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (!validateDrawCallUBO() || !validateDrawCallVertexAttribs() ||
        !validateStencilSettings()) {
        return;
    }

    const uint32_t enabled = getState()->arraysEnabled();
    for (GLuint array = 0;
         array < WebGLRenderingContextState::kMaxVertexAttribs; ++array) {
        if (!(enabled & (1u << array))) {
            continue;
        }
        // If a vertex attribute is enabled as an array via
        // enableVertexAttribArray but no buffer is bound to that attribute
        // (generally via bindBuffer and vertexAttribPointer), then draw
        // commands (drawArrays or drawElements) will generate an
        // INVALID_OPERATION error.
        if (!m_state->getBufferBoundToVertexAttributes(array)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
    }

    // A bad mode is INVALID_ENUM even when the range is wrong too.
    if (!isValidDrawMode(mode)) {
        setGLError(GL_INVALID_ENUM, "invalid draw mode");
        return;
    }

    if (!validateFramebufferComplete(GL_DRAW_FRAMEBUFFER)) {
        return;
    }

    if (count > 0 &&
        !hasEnoughVertexArrayData(static_cast<int64_t>(first) + count, 1)) {
        setGLError(GL_INVALID_OPERATION,
                   "drawArrays: vertices reach past the bound ARRAY_BUFFER");
        return;
    }

    if (!validateFeedbackLoops()) {
        return;
    }

    bool restoreDrawBuffers = false;
    GLenum originalBuffers[16];
    size_t originalCount = 0;
    if (!isDefaultFramebufferBound()) {
        Optional<WebGLProgram*> maybeProg = m_state->webGLProgram();
        if (maybeProg.hasValue() && maybeProg.value() &&
            maybeProg.value()->usesFragColor()) {
            Optional<WebGLFramebuffer*> maybeFb = m_state->webGLFramebuffer();
            if (maybeFb.hasValue() && maybeFb.value()) {
                WebGLFramebuffer* fb = maybeFb.value();
                if (fb->drawBufferCount() > 1) {
                    restoreDrawBuffers = true;
                    originalCount = fb->drawBufferCount();
                    GLenum singleBuf[16];
                    for (size_t i = 0; i < originalCount; ++i) {
                        originalBuffers[i] = fb->drawBuffers()[i];
                        singleBuf[i] =
                            (i == 0) ? fb->drawBuffers()[0] : GL_NONE;
                    }
                    m_gl->drawBuffers(originalCount, singleBuf);
                }
            }
        }
    }

    const bool standIns = bindIncompleteTextureStandIns();
    m_gl->drawArrays(mode, first, count);
    if (standIns) {
        restoreSampledTextureBindings();
    }

    if (restoreDrawBuffers) {
        m_gl->drawBuffers(originalCount, originalBuffers);
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::drawElements(GLenum mode, GLsizei count,
                                         GLenum type, WebGLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (!isValidDrawMode(mode)) {
        setGLError(GL_INVALID_ENUM, "invalid draw mode");
        return;
    }

    bool isUintAllowed =
        (webGLVersion() >= 2) || isExtensionEnabled("OES_element_index_uint");
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT &&
        (!isUintAllowed || type != GL_UNSIGNED_INT)) {
        setGLError(GL_INVALID_ENUM, "invalid type");
        return;
    }

    if (count < 0 || offset < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if ((type == GL_UNSIGNED_SHORT && (offset % 2) != 0) ||
        (type == GL_UNSIGNED_INT && (offset % 4) != 0)) {
        setGLError(GL_INVALID_OPERATION, "offset not properly aligned");
        return;
    }

    // A missing index buffer is an error even when nothing is drawn.
    Optional<WebGLBuffer*> elementBuffer =
        getState()->getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);
    if (!elementBuffer.hasValue() || !elementBuffer.value()) {
        setGLError(GL_INVALID_OPERATION,
                   "drawElements: no buffer bound to ELEMENT_ARRAY_BUFFER");
        return;
    }

    // OpenGL ES 3.0.6 §2.15.2.
    if (isTransformFeedbackActiveUnpaused()) {
        setGLError(GL_INVALID_OPERATION,
                   "transform feedback is active and not paused");
        return;
    }
    // WebGL 1.0 §6.37, also for a draw of nothing.
    if (!m_state->webGLProgram()) {
        setGLError(GL_INVALID_OPERATION, "no current program");
        return;
    }
    if (!validateFramebufferComplete(GL_DRAW_FRAMEBUFFER)) {
        return;
    }

    // A draw of nothing still reports a feedback loop, as drawArrays does.
    if (count == 0) {
        validateFeedbackLoops();
        return;
    }

    completePendingJobs();

    const uint32_t enabled = getState()->arraysEnabled();
    for (GLuint array = 0;
         array < WebGLRenderingContextState::kMaxVertexAttribs; ++array) {
        if (!(enabled & (1u << array))) {
            continue;
        }
        if (!m_state->getBufferBoundToVertexAttributes(array)) {
            setGLError(
                GL_INVALID_OPERATION,
                "drawElements: vertex attribute enabled without bound buffer");
            return;
        }
    }

    if (!hasEnoughElementArrayData(count, type, offset)) {
        setGLError(GL_INVALID_OPERATION,
                   "drawElements: indices reach past ELEMENT_ARRAY_BUFFER");
        return;
    }

    if (!validateDrawCallUBO() || !validateDrawCallVertexAttribs() ||
        !validateStencilSettings() || !validateFeedbackLoops()) {
        return;
    }

    bool restoreDrawBuffers = false;
    GLenum originalBuffers[16];
    size_t originalCount = 0;
    if (!isDefaultFramebufferBound()) {
        Optional<WebGLProgram*> maybeProg = m_state->webGLProgram();
        if (maybeProg.hasValue() && maybeProg.value() &&
            maybeProg.value()->usesFragColor()) {
            Optional<WebGLFramebuffer*> maybeFb = m_state->webGLFramebuffer();
            if (maybeFb.hasValue() && maybeFb.value()) {
                WebGLFramebuffer* fb = maybeFb.value();
                if (fb->drawBufferCount() > 1) {
                    restoreDrawBuffers = true;
                    originalCount = fb->drawBufferCount();
                    GLenum singleBuf[16];
                    for (size_t i = 0; i < originalCount; ++i) {
                        originalBuffers[i] = fb->drawBuffers()[i];
                        singleBuf[i] =
                            (i == 0) ? fb->drawBuffers()[0] : GL_NONE;
                    }
                    m_gl->drawBuffers(originalCount, singleBuf);
                }
            }
        }
    }

    const bool standIns = bindIncompleteTextureStandIns();
    m_gl->drawElements(mode, count, type,
                       reinterpret_cast<void*>(clampToGLintptr(offset)));
    if (standIns) {
        restoreSampledTextureBindings();
    }

    if (restoreDrawBuffers) {
        m_gl->drawBuffers(originalCount, originalBuffers);
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::enable(GLenum cap)
{
    ENTER_CONTEXT_SCOPE();

    if (!isValidEnableCap(cap, isWebGL2RenderingContext())) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    m_gl->enable(cap);
}

void WebGLRenderingContext::enableVertexAttribArray(GLuint index)
{
    ENTER_CONTEXT_SCOPE();
    /*
        NOTE: No idea to handle the following for now. It may already be handled
        in GLES3: WebGL imposes additional rules beyond OpenGL ES 2.0 regarding
        enabled vertex attributes; see Enabled Vertex Attributes and Range
        Checking.
    */
    m_gl->enableVertexAttribArray(index);
    if (hasNewGLError()) {
        return;
    }
    getState()->enableVertexAttribArray(index);
}

void WebGLRenderingContext::finish()
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    m_gl->finish();
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::flush()
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    m_gl->flush();
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::framebufferRenderbuffer(
    GLenum target, GLenum attachment, GLenum renderbuffertarget,
    Optional<WebGLRenderbuffer*> maybeRenderbuffer)
{
    ENTER_CONTEXT_SCOPE();

    bool isWebGL2 = isWebGL2RenderingContext();
    if (isWebGL2) {
        if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
            target != GL_DRAW_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else {
        if (target != GL_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    }

    if (renderbuffertarget != GL_RENDERBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    // WebGL 1 has no WEBGL_draw_buffers: COLOR_ATTACHMENT0 is its only one.
    GLint maxColorAttachments = 1;
    if (isWebGL2) {
        m_gl->getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    }
    if (maxColorAttachments < 1) {
        maxColorAttachments = 1;
    }
    if (attachment >= GL_COLOR_ATTACHMENT0 &&
        attachment < static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + 16)) {
        if (attachment >=
            static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + maxColorAttachments)) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (attachment != GL_DEPTH_ATTACHMENT &&
               attachment != GL_STENCIL_ATTACHMENT &&
               attachment != GL_DEPTH_STENCIL_ATTACHMENT) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    Optional<WebGLFramebuffer*> maybeFb = getBoundFramebuffer(target);
    if (!maybeFb.hasValue() || !maybeFb.value()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    WebGLFramebuffer* fb = maybeFb.value();

    if (maybeRenderbuffer.hasValue()) {
        WebGLRenderbuffer* renderBuffer = maybeRenderbuffer.value();

        if (!isFromCurrentContext(renderBuffer)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (renderBuffer->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        GLuint rbId = renderBuffer->glObject();
        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            m_gl->framebufferRenderbuffer(target, GL_DEPTH_ATTACHMENT,
                                          renderbuffertarget, rbId);
            m_gl->framebufferRenderbuffer(target, GL_STENCIL_ATTACHMENT,
                                          renderbuffertarget, rbId);
        } else {
            m_gl->framebufferRenderbuffer(target, attachment,
                                          renderbuffertarget, rbId);
        }

        fb->setAttachmentRenderbuffer(attachment, renderBuffer);
    } else {
        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            m_gl->framebufferRenderbuffer(target, GL_DEPTH_ATTACHMENT,
                                          renderbuffertarget, 0);
            m_gl->framebufferRenderbuffer(target, GL_STENCIL_ATTACHMENT,
                                          renderbuffertarget, 0);
        } else {
            m_gl->framebufferRenderbuffer(target, attachment,
                                          renderbuffertarget, 0);
        }

        fb->setAttachmentRenderbuffer(attachment, nullptr);
    }
    if (!isWebGL2 && (attachment == GL_DEPTH_ATTACHMENT ||
                      attachment == GL_STENCIL_ATTACHMENT ||
                      attachment == GL_DEPTH_STENCIL_ATTACHMENT)) {
        attachWebGL1DepthStencil(fb, attachment != GL_STENCIL_ATTACHMENT,
                                 attachment != GL_DEPTH_ATTACHMENT);
    }
}

void WebGLRenderingContext::framebufferTexture2D(
    GLenum target, GLenum attachment, GLenum textarget,
    Optional<WebGLTexture*> maybeTexture, GLint level)
{
    ENTER_CONTEXT_SCOPE();

    bool isWebGL2 = isWebGL2RenderingContext();
    if (isWebGL2) {
        if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
            target != GL_DRAW_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else {
        if (target != GL_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    }

    // WebGL 1 has no WEBGL_draw_buffers: COLOR_ATTACHMENT0 is its only one.
    GLint maxColorAttachments = 1;
    if (isWebGL2) {
        m_gl->getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    }
    if (maxColorAttachments < 1) {
        maxColorAttachments = 1;
    }
    if (attachment >= GL_COLOR_ATTACHMENT0 &&
        attachment < static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + 16)) {
        if (attachment >=
            static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + maxColorAttachments)) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (attachment != GL_DEPTH_ATTACHMENT &&
               attachment != GL_STENCIL_ATTACHMENT &&
               attachment != GL_DEPTH_STENCIL_ATTACHMENT) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    Optional<WebGLFramebuffer*> maybeFb = getBoundFramebuffer(target);
    if (!maybeFb.hasValue() || !maybeFb.value()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    WebGLFramebuffer* fb = maybeFb.value();

    if (maybeTexture.hasValue()) {
        WebGLTexture* texture = maybeTexture.value();

        if (!isFromCurrentContext(texture)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (texture->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        GLint maxTexSize = 0;
        m_gl->getIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
        GLint maxLevels = 0;
        while ((maxTexSize >> maxLevels) > 0) {
            ++maxLevels;
        }
        // OpenGL ES 2.0 §4.4.3: a WebGL 1 attachment is level 0.
        if (level < 0 || level >= (isWebGL2 ? maxLevels : 1)) {
            setGLError(GL_INVALID_VALUE);
            return;
        }

        GLuint textureId = texture->glObject();
        hasNewGLError(); // Keep an earlier error from failing this call.
        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            m_gl->framebufferTexture2D(target, GL_DEPTH_ATTACHMENT, textarget,
                                       textureId, level);
            m_gl->framebufferTexture2D(target, GL_STENCIL_ATTACHMENT, textarget,
                                       textureId, level);
        } else {
            m_gl->framebufferTexture2D(target, attachment, textarget, textureId,
                                       level);
        }
        // A rejected attach leaves the old image attached.
        if (hasNewGLError()) {
            return;
        }

        fb->setAttachmentTexture2D(attachment, texture, textarget, level);
    } else {
        if (level != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        hasNewGLError(); // Keep an earlier error from failing this call.
        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            m_gl->framebufferTexture2D(target, GL_DEPTH_ATTACHMENT, textarget,
                                       0, 0);
            m_gl->framebufferTexture2D(target, GL_STENCIL_ATTACHMENT, textarget,
                                       0, 0);
        } else {
            m_gl->framebufferTexture2D(target, attachment, textarget, 0, 0);
        }
        if (hasNewGLError()) {
            return;
        }

        fb->setAttachmentTexture2D(attachment, nullptr, textarget, level);
    }
    if (!isWebGL2 && (attachment == GL_DEPTH_ATTACHMENT ||
                      attachment == GL_STENCIL_ATTACHMENT ||
                      attachment == GL_DEPTH_STENCIL_ATTACHMENT)) {
        attachWebGL1DepthStencil(fb, attachment != GL_STENCIL_ATTACHMENT,
                                 attachment != GL_DEPTH_ATTACHMENT);
    }
}

// WebGL 1.0 §6.8: DEPTH_STENCIL_ATTACHMENT is a point of its own, which the
// OpenGL ES 3 driver lacks. Its image takes the driver's DEPTH and STENCIL
// points in place of the DEPTH and STENCIL images; with more than one of the
// three, checkWebGLStatus() is FRAMEBUFFER_UNSUPPORTED and nothing draws.
void WebGLRenderingContext::attachWebGL1DepthStencil(
    WebGLFramebuffer* framebuffer, bool depth, bool stencil)
{
    for (bool isStencil : { false, true }) {
        if (isStencil ? !stencil : !depth) {
            continue;
        }
        WebGLTexture* texture;
        WebGLRenderbuffer* renderbuffer;
        GLenum textarget;
        GLint level;
        framebuffer->webGL1DepthStencilImage(isStencil, texture, renderbuffer,
                                             textarget, level);
        const GLenum point =
            isStencil ? GL_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
        // shortcut: an image deleted while another hid it is gone from the
        // driver, and its name may name another object by now, so its point
        // is left empty; keep the hidden image alive to restore it.
        if (texture && !texture->isDeleted()) {
            m_gl->framebufferTexture2D(GL_FRAMEBUFFER, point, textarget,
                                       texture->glObject(), level);
        } else {
            m_gl->framebufferRenderbuffer(
                GL_FRAMEBUFFER, point, GL_RENDERBUFFER,
                renderbuffer && !renderbuffer->isDeleted()
                    ? renderbuffer->glObject()
                    : 0);
        }
    }
}

void WebGLRenderingContext::frontFace(GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->frontFace(mode);
}

void WebGLRenderingContext::generateMipmap(GLenum target)
{
    ENTER_CONTEXT_SCOPE();

    // OpenGL ES 2.0 §3.7.11: a WebGL 1 level 0 must be a power of two.
    if (!isWebGL2RenderingContext() &&
        (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP)) {
        WebGLTexture* texture = getBoundTextureObject(target);
        const TextureImageLevelInfo* info =
            texture ? texture->getImageLevelInfo(
                          target == GL_TEXTURE_CUBE_MAP
                              ? GL_TEXTURE_CUBE_MAP_POSITIVE_X
                              : target,
                          0)
                    : nullptr;
        // WEBGL_depth_texture: no mipmaps of a depth texture.
        if (info && isDepthTextureFormat(info->internalFormat)) {
            setGLError(GL_INVALID_OPERATION, "the texture is a depth texture");
            return;
        }
        if (info &&
            !(isPowerOfTwo(info->width) && isPowerOfTwo(info->height))) {
            setGLError(GL_INVALID_OPERATION, "level 0 is not a power of two");
            return;
        }
    }

    if (isWebGL2RenderingContext()) {
        WebGLTexture* texture = getBoundTextureObject(target);
        if (texture) {
            GLenum imageTarget = target == GL_TEXTURE_CUBE_MAP
                                     ? GL_TEXTURE_CUBE_MAP_POSITIVE_X
                                     : target;
            const TextureImageLevelInfo* info =
                texture->getImageLevelInfo(imageTarget, texture->baseLevel());
            if (info && info->isDefined) {
                GLenum fmt = info->internalFormat;
                bool isFloat32 = (fmt == GL_RGBA32F || fmt == GL_RGB32F ||
                                  fmt == GL_RG32F || fmt == GL_R32F);
                if (isFloat32 &&
                    !isExtensionEnabled("OES_texture_float_linear")) {
                    setGLError(GL_INVALID_OPERATION,
                               "generateMipmap: float texture not filterable "
                               "without OES_texture_float_linear");
                    return;
                }
            }
        }
    }

    hasNewGLError(); // Keep an earlier error from failing this call.
    m_gl->generateMipmap(target);
    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && !hasNewGLError()) {
        texture->setMipmapLevels(target);
    }
}

void WebGLRenderingContext::shaderSource(WebGLShader* shader, String* source)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(shader)) {
        return;
    }

    std::string str = source->toUTF8NonGCString();
    shader->setSource(str);
}

void WebGLRenderingContext::stencilFunc(GLenum func, GLint ref, GLuint mask)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->stencilFunc(func, ref, mask);
}

void WebGLRenderingContext::stencilFuncSeparate(GLenum face, GLenum func,
                                                GLint ref, GLuint mask)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->stencilFuncSeparate(face, func, ref, mask);
}

void WebGLRenderingContext::stencilMask(GLuint mask)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->stencilMask(mask);
}

void WebGLRenderingContext::stencilMaskSeparate(GLenum face, GLuint mask)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->stencilMaskSeparate(face, mask);
}

void WebGLRenderingContext::stencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->stencilOp(fail, zfail, zpass);
}

void WebGLRenderingContext::stencilOpSeparate(GLenum face, GLenum fail,
                                              GLenum zfail, GLenum zpass)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->stencilOpSeparate(face, fail, zfail, zpass);
}

ScriptValue WebGLRenderingContext::getBufferParameter(GLenum target,
                                                      GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    switch (pname) {
    // GLint
    case GL_BUFFER_SIZE: {
        GLint value;
        gl()->getBufferParameteriv(target, pname, &value);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(value);
    }
    // GLenum
    case GL_BUFFER_USAGE: {
        GLint value;
        gl()->getBufferParameteriv(target, pname, &value);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLenum>(value));
    }
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

ScriptValue WebGLRenderingContext::getParameter(GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    // GLint
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_GREEN_BITS:
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
    case GL_MAX_RENDERBUFFER_SIZE:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_VARYING_VECTORS:
    case GL_MAX_VERTEX_ATTRIBS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
    case GL_PACK_ALIGNMENT:
    case GL_RED_BITS:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
    case GL_STENCIL_BACK_REF:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_REF:
    case GL_SUBPIXEL_BITS:
    case GL_MAX_SAMPLES:
    case GL_UNPACK_ALIGNMENT: {
        std::vector<int> values(1);
        m_gl->getIntegerv(pname, &values[0]);
        return ValueRef::create(values[0]);
    }
    case GL_DEPTH_BITS: {
        if (!isDefaultFramebufferBound()) {
            Optional<WebGLFramebuffer*> maybeFb =
                getBoundFramebuffer(GL_FRAMEBUFFER);
            if (maybeFb.hasValue() && maybeFb.value()) {
                // WebGL 1 has a DEPTH_STENCIL_ATTACHMENT of its own.
                WebGLFramebuffer* fb = maybeFb.value();
                WebGLRenderbuffer* rb =
                    fb->attachedRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT);
                if (!rb) {
                    rb = fb->attachedRenderbuffer(GL_DEPTH_ATTACHMENT);
                }
                if (rb) {
                    GLenum fmt = rb->internalFormat();
                    if (fmt == GL_DEPTH_COMPONENT16) {
                        return ValueRef::create(16);
                    }
                    if (fmt == GL_DEPTH_COMPONENT24 ||
                        fmt == GL_DEPTH24_STENCIL8 || fmt == GL_DEPTH_STENCIL) {
                        return ValueRef::create(24);
                    }
                    if (fmt == GL_DEPTH_COMPONENT32F ||
                        fmt == GL_DEPTH32F_STENCIL8) {
                        return ValueRef::create(32);
                    }
                    return ValueRef::create(16);
                }
                if (fb->attachedTexture(GL_DEPTH_STENCIL_ATTACHMENT) ||
                    fb->attachedTexture(GL_DEPTH_ATTACHMENT)) {
                    return ValueRef::create(24);
                }
                return ValueRef::create(0);
            }
        }
        if (!m_attributes.depth()) {
            return ValueRef::create(0);
        }
        std::vector<int> values(1);
        m_gl->getIntegerv(pname, &values[0]);
        return ValueRef::create(values[0]);
    }
    case GL_STENCIL_BITS: {
        if (!isDefaultFramebufferBound()) {
            Optional<WebGLFramebuffer*> maybeFb =
                getBoundFramebuffer(GL_FRAMEBUFFER);
            if (maybeFb.hasValue() && maybeFb.value()) {
                // WebGL 1 has a DEPTH_STENCIL_ATTACHMENT of its own.
                WebGLFramebuffer* fb = maybeFb.value();
                for (GLenum attachment :
                     { GL_DEPTH_STENCIL_ATTACHMENT, GL_STENCIL_ATTACHMENT }) {
                    if (fb->attachedRenderbuffer(attachment) ||
                        fb->attachedTexture(attachment)) {
                        return ValueRef::create(8);
                    }
                }
                return ValueRef::create(0);
            }
        }
        if (!m_attributes.stencil()) {
            return ValueRef::create(0);
        }
        std::vector<int> values(1);
        m_gl->getIntegerv(pname, &values[0]);
        return ValueRef::create(values[0]);
    }
    // GLuint
    case GL_STENCIL_BACK_VALUE_MASK:
    case GL_STENCIL_BACK_WRITEMASK:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK: {
        GLint value = 0;
        m_gl->getIntegerv(pname, &value);
        return createScriptValue(static_cast<uint32_t>(value));
    }
    // GLfloat
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE: {
        GLfloat value = 0;
        m_gl->getFloatv(pname, &value);
        return createScriptValue(value);
    }
    case GL_FRAGMENT_SHADER_DERIVATIVE_HINT: {
        // WebGL 2 answers this before; WebGL 1 has it with the extension.
        if (!isExtensionEnabled("OES_standard_derivatives")) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        GLint value = 0;
        m_gl->getIntegerv(pname, &value);
        return createScriptValue(static_cast<uint32_t>(value));
    }
    case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: {
        if (!isExtensionEnabled("EXT_texture_filter_anisotropic")) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        std::vector<GLfloat> values(1);
        gl()->getFloatv(pname, &values[0]);
        return createScriptValue(values[0]);
    }
    // WebGLProgram
    case GL_CURRENT_PROGRAM: {
        GLint value = -1;
        m_gl->getIntegerv(pname, &value);

        // A program deleted while current is still the current program.
        Optional<WebGLProgram*> maybe = m_state->webGLProgram();
        if (!maybe.hasValue()) {
            return scriptNull();
        }
        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
    // WebGLFramebuffer
    case GL_FRAMEBUFFER_BINDING: {
        GLint value = -1;
        m_gl->getIntegerv(pname, &value);

        if (isDefaultFramebufferBound()) {
            return scriptNull();
        }

        Optional<WebGLFramebuffer*> maybe = m_state->webGLFramebuffer();
        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }
        TRACE(WEBGL, KV(maybe.value()->glObject()), KV(value));
        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
    // WebGLVertexArrayObjectOES
    case GL_VERTEX_ARRAY_BINDING: {
        // GL_VERTEX_ARRAY_BINDING_OES
        if (!isExtensionEnabled("OES_vertex_array_object")) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        GLint value = -1;
        m_gl->getIntegerv(pname, &value);
        if (value == 0) {
            return scriptNull();
        }

        Optional<WebGLVertexArrayObjectOES*> maybe =
            m_state->webGLVertexArrayObjectOES();

        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }

        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
    // WebGLBuffer
    case GL_ARRAY_BUFFER_BINDING:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING: {
        GLuint target = (pname == GL_ARRAY_BUFFER_BINDING)
                            ? GL_ARRAY_BUFFER
                            : GL_ELEMENT_ARRAY_BUFFER;
        WebGLBuffer* buffer = m_state->getBoundBuffer(target).valueOr(nullptr);
        GLint value = 0;
        m_gl->getIntegerv(pname, &value);
        if (buffer && !buffer->isDeleted()) {
            return buffer->scriptValue();
        }
        return scriptNull();
    }
    // WebGLTexture
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_CUBE_MAP: {
        GLint value = 0;
        m_gl->getIntegerv(pname, &value);
        if (value == 0) {
            return scriptNull();
        }
        WebGLTexture* texture = getBoundTextureObject(
            pname == GL_TEXTURE_BINDING_2D ? GL_TEXTURE_2D
                                           : GL_TEXTURE_CUBE_MAP);
        // The engine binds textures of its own on this context, so trust GL
        // over the tracked object when the two disagree.
        if (texture == nullptr || texture->isDeleted() ||
            static_cast<GLint>(texture->glObject()) != value) {
            return scriptNull();
        }
        return texture->scriptValue();
    }
    // WebGLRenderbuffer
    case GL_RENDERBUFFER_BINDING: {
        GLint value = 0;
        m_gl->getIntegerv(pname, &value);
        if (value == 0 || !m_boundRenderbuffer.hasValue() ||
            m_boundRenderbuffer.value()->isDeleted() ||
            static_cast<GLint>(m_boundRenderbuffer.value()->glObject()) !=
                value) {
            return scriptNull();
        }
        return m_boundRenderbuffer.value()->scriptValue();
    }
    // GLenum
    case GL_ACTIVE_TEXTURE:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_DST_RGB:
    // GL_BLEND_EQUATION_RGB and GL_BLEND_EQUATION are the same enum.
    case GL_BLEND_EQUATION_RGB:
    case GL_BLEND_EQUATION_ALPHA:
    case GL_BLEND_SRC_ALPHA:
    case GL_BLEND_SRC_RGB:
    case GL_CULL_FACE_MODE:
    case GL_DEPTH_FUNC:
    case GL_FRONT_FACE:
    case GL_GENERATE_MIPMAP_HINT:
    case GL_STENCIL_BACK_FAIL:
    case GL_STENCIL_BACK_FUNC:
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS: {
        GLint value = 0;
        m_gl->getIntegerv(pname, &value);
        return createScriptValue(static_cast<uint32_t>(value));
    }
    case kUNPACK_COLORSPACE_CONVERSION_WEBGL: {
        return createScriptValue(
            static_cast<uint32_t>(m_unpackColorspaceConversion));
    }
    case kIMPLEMENTATION_COLOR_READ_TYPE: {
        GLint readType = GL_UNSIGNED_BYTE;
        m_gl->getIntegerv(kIMPLEMENTATION_COLOR_READ_TYPE, &readType);
        return ValueRef::create(static_cast<uint32_t>(readType));
    }
    case kIMPLEMENTATION_COLOR_READ_FORMAT: {
        GLint readFormat = GL_RGBA;
        m_gl->getIntegerv(kIMPLEMENTATION_COLOR_READ_FORMAT, &readFormat);
        return ValueRef::create(static_cast<uint32_t>(readFormat));
    }
    // DOMString
    case GL_SHADING_LANGUAGE_VERSION:
        return createScriptASCIIString(kShadingLanguageVersion);
    case GL_VERSION:
        return createScriptASCIIString(kVersion);
    case GL_RENDERER:
    case GL_VENDOR: {
        const std::string output =
            reinterpret_cast<const char*>(glGetString(pname));
        return StringRef::createFromASCII(output.c_str(), output.length());
    }
    // GLboolean
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST: {
        std::vector<GLboolean> values(1);
        m_gl->getBooleanv(pname, &values[0]);
        return createScriptValue(static_cast<bool>(values[0]));
    }
    case kUNPACK_FLIP_Y_WEBGL: {
        return createScriptValue(m_unpackFlipY);
    }
    case kUNPACK_PREMULTIPLY_ALPHA_WEBGL: {
        return createScriptValue(m_unpackPremultiplyAlpha);
    }
    // sequence<GLboolean> (with 4 elements)
    case GL_COLOR_WRITEMASK: {
        std::vector<GLboolean> values(4);
        m_gl->getBooleanv(pname, &values[0]);
        std::vector<bool> mask;
        mask.reserve(values.size());
        for (GLboolean value : values) {
            mask.push_back(static_cast<bool>(value));
        }
        return createArray(scriptBindingInstance(), mask);
    }
    // Float32Array (with 2 elements)
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE: {
        std::vector<GLfloat> values(2);
        m_gl->getFloatv(pname, &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    // Float32Array (with 4 elements)
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE: {
        std::vector<GLfloat> values(4);
        m_gl->getFloatv(pname, &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    // Int32Array (with 2 elements)
    case GL_MAX_VIEWPORT_DIMS: {
        std::vector<int> values(2);
        m_gl->getIntegerv(pname, &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    // Int32Array (with 4 elements)
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT: {
        std::vector<int> values(4);
        m_gl->getIntegerv(pname, &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    // Uint32Array
    case GL_COMPRESSED_TEXTURE_FORMATS: {
        // The formats of the compressed texture extensions enabled so far:
        // the ETC2/EAC run, then the RGBA and SRGB8_ALPHA8 ASTC runs.
        std::vector<uint32_t> formats;
        for (GLenum first : { 0x9270u, 0x93B0u, 0x93D0u }) {
            for (GLenum format = first;
                 Pixel::compressedFormatInfo(format).hasValue(); ++format) {
                if (isCompressedFormatEnabled(format)) {
                    formats.push_back(format);
                }
            }
        }
        return createTypedArray<Uint32ArrayObjectRef>(scriptBindingInstance(),
                                                      formats);
    }
    default:
        STARFISH_UNSUPPORTED("pname: 0x%04X(%s)", pname, __PRETTY_FUNCTION__);
        // An unknown pname is an INVALID_ENUM, not a silent null: content that
        // reads the result (`getParameter(p).toString()`) must see the error
        // rather than a TypeError.
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }
    return scriptNull();
}

WebGLActiveInfo* WebGLRenderingContext::getActiveAttrib(WebGLProgram* program,
                                                        GLuint index)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(program)) {
        return nullptr;
    }

    GLint maxNameLength;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                       &maxNameLength);

    if (hasNewGLError()) {
        return nullptr;
    }

    GLint size;
    GLenum type;
    GLsizei length;

    std::vector<char> name;
    name.resize(maxNameLength, '\0');
    m_gl->getActiveAttrib(program->glObject(), index, maxNameLength, &length,
                          &size, &type, &name[0]);

    if (hasNewGLError()) {
        // a) If the passed index is out of range, generates an INVALID_VALUE
        // error and returns null. b) Returns null if any OpenGL errors are
        // generated during the execution of this function.
        return nullptr;
    }

    return new WebGLActiveInfo(scriptBindingInstance(), size, type,
                               String::createASCIIString(name.data(), length));
}

WebGLActiveInfo* WebGLRenderingContext::getActiveUniform(WebGLProgram* program,
                                                         GLuint index)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(program)) {
        return nullptr;
    }

    GLint maxNameLength;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH,
                       &maxNameLength);

    if (hasNewGLError()) {
        return nullptr;
    }

    GLint size;
    GLenum type;
    GLsizei length;

    std::vector<char> name;
    name.resize(maxNameLength, '\0');
    m_gl->getActiveUniform(program->glObject(), index, maxNameLength, &length,
                           &size, &type, &name[0]);

    if (hasNewGLError()) {
        // a) If the passed index is out of range, generates an INVALID_VALUE
        // error and returns null. b) Returns null if any OpenGL errors are
        // generated during the execution of this function.
        return nullptr;
    }

    return new WebGLActiveInfo(scriptBindingInstance(), size, type,
                               String::createASCIIString(name.data(), length));
}

Optional<GCVector<WebGLShader*>> WebGLRenderingContext::getAttachedShaders(
    WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(program)) {
        return nullptr;
    }

    return program->getAttachedShaders();
}

GLint WebGLRenderingContext::getAttribLocation(WebGLProgram* program,
                                               String* name)
{
    ENTER_CONTEXT_SCOPE(-1);

    if (!validateProgramOrShader(program)) {
        return -1;
    }

    if (!checkAttribOrUniformName(name)) {
        return -1;
    }

    // TODO: Returns -1 if the context's webgl context lost flag is set.

    if (program->invalidated()) {
        // If the invalidated flag of the passed program is set, generates an
        // INVALID_OPERATION error and returns -1.
        setGLError(GL_INVALID_OPERATION);
        return -1;
    }

    return m_gl->getAttribLocation(program->glObject(), CSTR(name));
}
Optional<WebGLFramebuffer*> WebGLRenderingContext::getBoundFramebuffer(
    GLenum target)
{
    return m_state->webGLFramebuffer();
}

ScriptValue WebGLRenderingContext::getFramebufferAttachmentParameter(
    GLenum target, GLenum attachment, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    bool isWebGL2 = isWebGL2RenderingContext();

    if (isWebGL2) {
        if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
            target != GL_DRAW_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
    } else {
        if (target != GL_FRAMEBUFFER) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
    }

    Optional<WebGLFramebuffer*> maybeFb = getBoundFramebuffer(target);

    if (!maybeFb.hasValue() || !maybeFb.value()) {
        if (!isWebGL2) {
            setGLError(GL_INVALID_OPERATION);
            return scriptNull();
        }

        if (attachment != GL_BACK && attachment != GL_DEPTH &&
            attachment != GL_STENCIL) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }

        if (pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE &&
            pname != GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }

        bool hasBuffer = true;
        if (attachment == GL_DEPTH) {
            hasBuffer = m_attributes.depth();
        } else if (attachment == GL_STENCIL) {
            hasBuffer = m_attributes.stencil();
        }

        if (!hasBuffer) {
            if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) {
                return Escargot::ValueRef::create(static_cast<GLenum>(GL_NONE));
            }
            setGLError(GL_INVALID_OPERATION);
            return scriptNull();
        }

        if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) {
            return Escargot::ValueRef::create(
                static_cast<GLenum>(GL_FRAMEBUFFER_DEFAULT));
        }

        if (pname == GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING &&
            (attachment == GL_DEPTH || attachment == GL_STENCIL)) {
            return Escargot::ValueRef::create(static_cast<GLenum>(GL_LINEAR));
        }

        GLenum driverAttachment = attachment;
        if (attachment == GL_BACK) {
            driverAttachment = GL_COLOR_ATTACHMENT0;
        } else if (attachment == GL_DEPTH) {
            driverAttachment = GL_DEPTH_ATTACHMENT;
        } else if (attachment == GL_STENCIL) {
            driverAttachment = GL_STENCIL_ATTACHMENT;
        }

        GLint params = 0;
        if (pname == GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE &&
            attachment == GL_STENCIL) {
            params = GL_UNSIGNED_NORMALIZED;
        } else {
            m_gl->getFramebufferAttachmentParameteriv(target, driverAttachment,
                                                      pname, &params);
        }
        if (hasNewGLError()) {
            return scriptNull();
        }
        if (pname == GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING ||
            pname == GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE) {
            return Escargot::ValueRef::create(static_cast<GLenum>(params));
        }
        return Escargot::ValueRef::create(params);
    }

    if (attachment == GL_BACK || attachment == GL_DEPTH ||
        attachment == GL_STENCIL) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    // WebGL 1 has no WEBGL_draw_buffers: COLOR_ATTACHMENT0 is its only one.
    GLint maxColorAttachments = 1;
    if (isWebGL2) {
        m_gl->getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    }
    if (maxColorAttachments < 1) {
        maxColorAttachments = 1;
    }

    bool isValidAttachment =
        (attachment >= GL_COLOR_ATTACHMENT0 &&
         attachment <
             static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + maxColorAttachments)) ||
        attachment == GL_DEPTH_ATTACHMENT ||
        attachment == GL_STENCIL_ATTACHMENT ||
        attachment == GL_DEPTH_STENCIL_ATTACHMENT;

    if (!isValidAttachment) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    WebGLFramebuffer* fb = maybeFb.value();
    WebGLTexture* tex = fb->attachedTexture(attachment);
    WebGLRenderbuffer* rb = fb->attachedRenderbuffer(attachment);

    // DEPTH_STENCIL_ATTACHMENT names one image only when depth and stencil
    // hold the same one (OpenGL ES 3.0 GetFramebufferAttachmentParameteriv);
    // WebGL 1's is a point of its own (WebGL 1.0 §6.8).
    if (isWebGL2 && attachment == GL_DEPTH_STENCIL_ATTACHMENT && !tex && !rb &&
        (fb->attachedTexture(GL_DEPTH_ATTACHMENT) ||
         fb->attachedRenderbuffer(GL_DEPTH_ATTACHMENT) ||
         fb->attachedTexture(GL_STENCIL_ATTACHMENT) ||
         fb->attachedRenderbuffer(GL_STENCIL_ATTACHMENT))) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    if (!tex && !rb) {
        if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) {
            return Escargot::ValueRef::create(static_cast<GLenum>(GL_NONE));
        }
        // With nothing attached, OpenGL ES 2.0 makes every other pname
        // INVALID_ENUM; OpenGL ES 3.0 answers OBJECT_NAME with 0 and makes
        // the rest INVALID_OPERATION (GetFramebufferAttachmentParameteriv).
        if (!isWebGL2) {
            setGLError(GL_INVALID_ENUM);
        } else if (pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME) {
            setGLError(GL_INVALID_OPERATION);
        }
        return scriptNull();
    }

    switch (pname) {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
        return Escargot::ValueRef::create(
            static_cast<GLenum>(tex ? GL_TEXTURE : GL_RENDERBUFFER));
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
        if (tex) {
            return tex->scriptValue();
        }
        if (rb) {
            return rb->scriptValue();
        }
        return scriptNull();
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
        if (!tex) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        if (!tex) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER:
        if (!isWebGL2 || !tex) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING:
        if (!isWebGL2) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        break;
    case GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE:
        if (!isWebGL2) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            setGLError(GL_INVALID_OPERATION);
            return scriptNull();
        }
        break;
    default:
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    if (pname == GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING &&
        (attachment == GL_DEPTH_ATTACHMENT ||
         attachment == GL_STENCIL_ATTACHMENT ||
         attachment == GL_DEPTH_STENCIL_ATTACHMENT)) {
        return Escargot::ValueRef::create(static_cast<GLenum>(GL_LINEAR));
    }

    GLenum driverAttachment = attachment;
    if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        if (pname == GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE) {
            driverAttachment = GL_STENCIL_ATTACHMENT;
        } else {
            driverAttachment = GL_DEPTH_ATTACHMENT;
        }
    }

    GLint params = 0;
    m_gl->getFramebufferAttachmentParameteriv(target, driverAttachment, pname,
                                              &params);
    if (hasNewGLError()) {
        return scriptNull();
    }

    if (pname == GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING ||
        pname == GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE) {
        return Escargot::ValueRef::create(static_cast<GLenum>(params));
    }

    return Escargot::ValueRef::create(params);
}

ScriptValue WebGLRenderingContext::getProgramParameter(WebGLProgram* program,
                                                       GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!validateProgramOrShader(program)) {
        return scriptNull();
    }

    switch (pname) {
    case GL_DELETE_STATUS:
    case GL_LINK_STATUS:
    case GL_VALIDATE_STATUS:
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
        break;
    default:
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    GLint params = 0;
    m_gl->getProgramiv(program->glObject(), pname, &params);

    if (hasNewGLError()) {
        /*
         - GL_INVALID_ENUM if pname is not an accepted value.
         - GL_INVALID_VALUE if program is not a value generated by OpenGL.
         - GL_INVALID_OPERATION if program does not refer to a program object.
        */
        return scriptNull();
    }

    switch (pname) {
    case GL_DELETE_STATUS:
        // GL learns of the deletion only once the program stops being current.
        return Escargot::ValueRef::create(program->isDeleted());
    case GL_LINK_STATUS:
        if (program->linkFailed()) {
            return Escargot::ValueRef::create(false);
        }
        return Escargot::ValueRef::create(static_cast<bool>(params));
    case GL_VALIDATE_STATUS:
        return Escargot::ValueRef::create(static_cast<bool>(params));
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
        return Escargot::ValueRef::create(static_cast<GLint>(params));
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

String* WebGLRenderingContext::getProgramInfoLog(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(program)) {
        return nullptr;
    }

    GLsizei length = 0, bufferSize = 0;
    m_gl->getProgramiv(program->glObject(), GL_INFO_LOG_LENGTH, &bufferSize);

    std::string buffer;
    buffer.resize(bufferSize);
    m_gl->getProgramInfoLog(program->glObject(), bufferSize, &length,
                            &buffer[0]);

    if (hasNewGLError()) {
        return nullptr;
    }
    buffer.resize(length);

    TRACE(WEBGL, buffer);

    return String::fromUTF8(buffer.data(), length);
}

ScriptValue WebGLRenderingContext::getRenderbufferParameter(GLenum target,
                                                            GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    GLint params = 0;
    m_gl->getRenderbufferParameteriv(target, pname, &params);

    if (hasNewGLError()) {
        return scriptNull();
    }

    switch (pname) {
    case GL_RENDERBUFFER_INTERNAL_FORMAT:
        // WebGL 1 keeps DEPTH_STENCIL as the format, which the driver holds
        // as DEPTH24_STENCIL8; WebGL 2 maps it so (WebGL 2.0
        // renderbufferStorage).
        if (!isWebGL2RenderingContext() && params == GL_DEPTH24_STENCIL8) {
            params = GL_DEPTH_STENCIL;
        }
        return createScriptValue(static_cast<GLenum>(params));
    case GL_RENDERBUFFER_WIDTH:
    case GL_RENDERBUFFER_HEIGHT:
    case GL_RENDERBUFFER_RED_SIZE:
    case GL_RENDERBUFFER_GREEN_SIZE:
    case GL_RENDERBUFFER_BLUE_SIZE:
    case GL_RENDERBUFFER_ALPHA_SIZE:
    case GL_RENDERBUFFER_DEPTH_SIZE:
    case GL_RENDERBUFFER_STENCIL_SIZE:
        return createScriptValue(params);
    case GL_RENDERBUFFER_SAMPLES:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        return createScriptValue(params);
    default:
        setGLError(GL_INVALID_ENUM);
        break;
    }
    return scriptNull();
}

ScriptValue WebGLRenderingContext::getShaderParameter(WebGLShader* shader,
                                                      GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!validateProgramOrShader(shader)) {
        return scriptNull();
    }

    GLint params = 0;
    m_gl->getShaderiv(shader->glObject(), pname, &params);

    if (hasNewGLError()) {
        /*
        - GL_INVALID_ENUM if pname is not an accepted value.
        - GL_INVALID_VALUE if shader is not a value generated by OpenGL.
        - GL_INVALID_OPERATION if shader does not refer to a shader object.
        */
        return scriptNull();
    }

    switch (pname) {
    case GL_SHADER_TYPE:
        return Escargot::ValueRef::create(static_cast<GLenum>(params));
    case GL_DELETE_STATUS:
        return Escargot::ValueRef::create(static_cast<bool>(params));
    case GL_COMPILE_STATUS:
        if (shader->compileFailed()) {
            return Escargot::ValueRef::create(false);
        }
        return Escargot::ValueRef::create(static_cast<bool>(params));
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

WebGLShaderPrecisionFormat* WebGLRenderingContext::getShaderPrecisionFormat(
    GLenum shadertype, GLenum precisiontype)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLint range[2];
    GLint precision;

    m_gl->getShaderPrecisionFormat(shadertype, precisiontype, range,
                                   &precision);
    if (hasNewGLError()) {
        return nullptr;
    }

    return new WebGLShaderPrecisionFormat(scriptBindingInstance(), range[0],
                                          range[1], precision);
}

String* WebGLRenderingContext::getShaderInfoLog(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(shader)) {
        return nullptr;
    }

    if (shader->compileFailed()) {
        return String::fromUTF8(shader->infoLog().c_str(),
                                shader->infoLog().length());
    }

    GLsizei length = 0, bufferSize = 0;
    m_gl->getShaderiv(shader->glObject(), GL_INFO_LOG_LENGTH, &bufferSize);

    std::string buffer;
    buffer.resize(bufferSize);
    m_gl->getShaderInfoLog(shader->glObject(), bufferSize, &length, &buffer[0]);

    if (hasNewGLError()) {
        return nullptr;
    }
    buffer.resize(length);

    TRACE(WEBGL, buffer);

    return String::fromUTF8(buffer.data(), length);
}

String* WebGLRenderingContext::getShaderSource(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(shader)) {
        return nullptr;
    }

    // The source as given: the driver has the text compileShader() made.
    const std::string& source = shader->source();
    return String::fromUTF8(source.data(), source.length());
}

WebGLTexture* WebGLRenderingContext::getBoundTextureObject(GLenum target) const
{
    GLenum bindTarget = target;
    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
        target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        bindTarget = GL_TEXTURE_CUBE_MAP;
    }
    auto it = m_boundTextureObjects.find(
        textureBindingKey(m_activeTextureUnit, bindTarget));
    if (it != m_boundTextureObjects.end()) {
        return it->second;
    }
    return nullptr;
}

bool WebGLRenderingContext::hasBoundTexture(GLenum target) const
{
    GLenum binding;
    switch (target) {
    case GL_TEXTURE_2D:
        binding = GL_TEXTURE_BINDING_2D;
        break;
    case GL_TEXTURE_3D:
        binding = GL_TEXTURE_BINDING_3D;
        break;
    case GL_TEXTURE_2D_ARRAY:
        binding = GL_TEXTURE_BINDING_2D_ARRAY;
        break;
    case GL_TEXTURE_CUBE_MAP:
    // A cube-map face is a 2D image target of the texture bound to
    // TEXTURE_CUBE_MAP (WebGL 2.0 texImage2D/texSubImage2D accept faces).
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        binding = GL_TEXTURE_BINDING_CUBE_MAP;
        break;
    default:
        return false;
    }
    GLint boundTexture = 0;
    m_gl->getIntegerv(binding, &boundTexture);
    return boundTexture != 0;
}

ScriptValue WebGLRenderingContext::getTexParameter(GLenum target, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    if (!hasBoundTexture(target)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT &&
        isExtensionEnabled("EXT_texture_filter_anisotropic")) {
        GLfloat params = 0;
        m_gl->getTexParameterfv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(params);
    }

    if (pname != GL_TEXTURE_MAG_FILTER && pname != GL_TEXTURE_MIN_FILTER &&
        pname != GL_TEXTURE_WRAP_S && pname != GL_TEXTURE_WRAP_T) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    GLint params = 0;
    m_gl->getTexParameteriv(target, pname, &params);

    if (hasNewGLError()) {
        return scriptNull();
    }

    return createScriptValue(static_cast<GLenum>(params));
}

GLenum WebGLRenderingContext::getUniformType(WebGLProgram* program,
                                             WebGLUniformLocation* location)
{
    GLint count = 0;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_UNIFORMS, &count);
    GLint maxNameLength = 0;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH,
                       &maxNameLength);
    for (GLint index = 0; index < count; index++) {
        GLint size;
        GLenum type;
        GLsizei length;
        std::vector<char> name;
        name.resize(maxNameLength, '\0');
        m_gl->getActiveUniform(program->glObject(), index, maxNameLength,
                               &length, &size, &type, &name[0]);
        if (size > 1) {
            std::string arrayName(&name[0]);
            if (arrayName.length() > 3 &&
                arrayName.substr(arrayName.length() - 3, arrayName.length()) ==
                    "[0]") {
                arrayName = arrayName.substr(0, arrayName.length() - 3);
            }
            for (GLint arrayIndex = 0; arrayIndex < size; arrayIndex++) {
                std::string elementName =
                    arrayName + "[" + std::to_string(arrayIndex) + "]";
                GLint uniformLocation = m_gl->getUniformLocation(
                    program->glObject(), elementName.data());
                if (uniformLocation == location->location()) {
                    return type;
                }
            }
        } else {
            GLint uniformLocation =
                m_gl->getUniformLocation(program->glObject(), &name[0]);
            if (uniformLocation == location->location()) {
                return type;
            }
        }
    }
    return GL_NONE;
}

Optional<ScriptValue> WebGLRenderingContext::getUniformImpl(
    WebGLProgram* program, WebGLUniformLocation* location, GLenum type)
{
    ENTER_CONTEXT_SCOPE(Optional<ScriptValue>());

    switch (type) {
    case GL_BOOL: {
        GLint value;
        m_gl->getUniformiv(program->glObject(), location->location(), &value);
        return createScriptValue(static_cast<bool>(value));
    }
    case GL_INT: {
        GLint value;
        m_gl->getUniformiv(program->glObject(), location->location(), &value);
        return createScriptValue(value);
    }
    case GL_FLOAT: {
        GLfloat value;
        m_gl->getUniformfv(program->glObject(), location->location(), &value);
        return createScriptValue(value);
    }
    case GL_FLOAT_VEC2: {
        std::vector<GLfloat> values(2);
        m_gl->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_INT_VEC2: {
        std::vector<GLint> values(2);
        m_gl->getUniformiv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    case GL_BOOL_VEC2: {
        std::vector<GLint> values(2);
        m_gl->getUniformiv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createArray(scriptBindingInstance(),
                        std::vector<bool>(values.begin(), values.end())));
    }
    case GL_FLOAT_VEC3: {
        std::vector<GLfloat> values(3);
        m_gl->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_INT_VEC3: {
        std::vector<GLint> values(3);
        m_gl->getUniformiv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    case GL_BOOL_VEC3: {
        std::vector<GLint> values(3);
        m_gl->getUniformiv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createArray(scriptBindingInstance(),
                        std::vector<bool>(values.begin(), values.end())));
    }
    case GL_FLOAT_VEC4: {
        std::vector<GLfloat> values(4);
        m_gl->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_INT_VEC4: {
        std::vector<GLint> values(4);
        m_gl->getUniformiv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    case GL_BOOL_VEC4: {
        std::vector<GLint> values(4);
        m_gl->getUniformiv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createArray(scriptBindingInstance(),
                        std::vector<bool>(values.begin(), values.end())));
    }
    case GL_FLOAT_MAT2: {
        std::vector<GLfloat> values(4);
        m_gl->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_FLOAT_MAT3: {
        std::vector<GLfloat> values(9);
        m_gl->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_FLOAT_MAT4: {
        std::vector<GLfloat> values(16);
        m_gl->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE: {
        GLint value;
        m_gl->getUniformiv(program->glObject(), location->location(), &value);
        return createScriptValue(value);
    }
    default:
        break;
    }
    return Optional<ScriptValue>();
}

ScriptValue WebGLRenderingContext::getUniform(WebGLProgram* program,
                                              WebGLUniformLocation* location)
{
    if (!validateProgramOrShader(program)) {
        return scriptNull();
    }

    if (location->program()->context() != this ||
        location->program() != program ||
        location->linkCount() != program->linkCount()) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    // getUniformType() queries the driver without a scope of its own (the
    // uniformMatrix* calls run it inside theirs), and getUniformImpl() opens
    // one, so the query gets a scope that closes before that.
    GLenum type = GL_NONE;
    {
        ENTER_CONTEXT_SCOPE(scriptNull());
        type = getUniformType(program, location);
    }
    Optional<ScriptValue> uniform = getUniformImpl(program, location, type);
    return uniform.valueOr(scriptNull());
}

// An array subscript is a GLSL int, so one beyond INT32_MAX names no uniform
// even where the driver would wrap it to a valid index.
static bool hasOutOfRangeSubscript(const std::string& name)
{
    for (size_t open = name.find('['); open != std::string::npos;
         open = name.find('[', open + 1)) {
        size_t first = name.find_first_not_of('0', open + 1);
        size_t close = name.find(']', open);
        if (first == std::string::npos || close == std::string::npos ||
            first > close) {
            continue;
        }
        size_t digits = close - first;
        if (digits > 10 ||
            (digits == 10 && name.compare(first, 10, "2147483647") > 0)) {
            return true;
        }
    }
    return false;
}

WebGLUniformLocation* WebGLRenderingContext::getUniformLocation(
    WebGLProgram* program, String* name)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!validateProgramOrShader(program)) {
        return nullptr;
    }

    if (!checkAttribOrUniformName(name)) {
        return nullptr;
    }

    if (hasOutOfRangeSubscript(name->toUTF8NonGCString())) {
        return nullptr;
    }

    GLint location = m_gl->getUniformLocation(program->glObject(), CSTR(name));
    if (location == -1) {
        /*
          - WebGL: The return value is null if name does not correspond to an
            active uniform variable in the passed program.
          - GLES: glGetUniformLocation returns -1 if name does not correspond to
            an active uniform variable in program or if name is associated with
            a named uniform block.
        */

        return nullptr;
    }

    if (hasNewGLError()) {
        // Returns null if any OpenGL errors are generated during the execution
        // of this function.
        return nullptr;
    }

    return new WebGLUniformLocation(scriptBindingInstance(), program, location,
                                    program->linkCount());
}

ScriptValue WebGLRenderingContext::getVertexAttrib(GLuint index, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    case GL_CURRENT_VERTEX_ATTRIB: {
        std::vector<float> values(4);
        m_gl->getVertexAttribfv(index, pname, &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING: {
        GLint value = 0;
        m_gl->getVertexAttribiv(index, pname, &value);

        TRACE(WEBGL, KV(index), KV(value));

        Optional<WebGLBuffer*> maybeBuffer =
            m_state->getBufferBoundToVertexAttributes(index);

        if (!maybeBuffer.hasValue()) {
            return scriptNull(); // No mention found for this in the spec.
        }

        TRACE(WEBGL, KV(index), KV(maybeBuffer.value()->glObject()));

        STARFISH_ASSERT(static_cast<GLuint>(value) ==
                        maybeBuffer.value()->glObject());

        return maybeBuffer.value()->scriptValue();
    }
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED: {
        GLint value = 0;
        m_gl->getVertexAttribiv(index, pname, &value);
        return ValueRef::create(value == 1 ? true : false);
    }
    case GL_VERTEX_ATTRIB_ARRAY_SIZE: {
        GLint value = 4;
        m_gl->getVertexAttribiv(index, pname, &value);
        return ValueRef::create(value);
    }
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE: {
        GLint value = 0;
        m_gl->getVertexAttribiv(index, pname, &value);
        return ValueRef::create(value);
    }
    case GL_VERTEX_ATTRIB_ARRAY_TYPE: {
        GLint value = GL_FLOAT;
        m_gl->getVertexAttribiv(index, pname, &value);
        return ValueRef::create(value);
    }
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED: {
        GLint value = 0;
        m_gl->getVertexAttribiv(index, pname, &value);
        return ValueRef::create(value == 1 ? true : false);
    }
    case GL_VERTEX_ATTRIB_ARRAY_DIVISOR: {
        // GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE: core in WebGL 2, only
        // readable through the extension in WebGL 1.
        if (!isWebGL2RenderingContext() &&
            !isExtensionEnabled("ANGLE_instanced_arrays")) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        GLint value = 0;
        m_gl->getVertexAttribiv(index, pname, &value);
        return ValueRef::create(value);
    }
    default:
        setGLError(GL_INVALID_ENUM);
        break;
    }
    return scriptNull();
}

WebGLintptr WebGLRenderingContext::getVertexAttribOffset(GLuint index,
                                                         GLenum pname)
{
    ENTER_CONTEXT_SCOPE(0);

    // The GL reports a bad index or pname. The offset itself is answered
    // from the recorded state: the GL pointer is pointer-sized, so it cannot
    // carry back every offset vertexAttribPointer accepted.
    hasNewGLError(); // Keep an earlier error from failing this call.
    GLvoid* pointer = nullptr;
    m_gl->getVertexAttribPointerv(index, pname, &pointer);
    if (hasNewGLError()) {
        return 0;
    }
    return m_state->getVertexAttribPointer(index).offset;
}

void WebGLRenderingContext::hint(GLenum target, GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    // OES_standard_derivatives adds this target to WebGL 1; WebGL 2 has it.
    if (target == GL_FRAGMENT_SHADER_DERIVATIVE_HINT && webGLVersion() == 1 &&
        !isExtensionEnabled("OES_standard_derivatives")) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    m_gl->hint(target, mode);
}

bool WebGLRenderingContext::isBuffer(Optional<WebGLBuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated() || maybe.value()->isDeleted()) {
        return false;
    }

    return m_gl->isBuffer(maybe.value()->glObject());
}

bool WebGLRenderingContext::isEnabled(GLenum cap)
{
    ENTER_CONTEXT_SCOPE(false);

    if (m_isContextLost) {
        return false;
    }

    if (!isValidEnableCap(cap, isWebGL2RenderingContext())) {
        setGLError(GL_INVALID_ENUM);
        return false;
    }

    return m_gl->isEnabled(cap);
}

bool WebGLRenderingContext::isFramebuffer(Optional<WebGLFramebuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated() || maybe.value()->isDeleted()) {
        return false;
    }

    return m_gl->isFramebuffer(maybe.value()->glObject());
}

bool WebGLRenderingContext::isProgram(Optional<WebGLProgram*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return hasGLObject(maybe.value());
}

bool WebGLRenderingContext::isRenderbuffer(Optional<WebGLRenderbuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated() || maybe.value()->isDeleted()) {
        return false;
    }

    return m_gl->isRenderbuffer(maybe.value()->glObject());
}

bool WebGLRenderingContext::isShader(Optional<WebGLShader*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return hasGLObject(maybe.value());
}

bool WebGLRenderingContext::isTexture(Optional<WebGLTexture*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated() || maybe.value()->isDeleted()) {
        return false;
    }

    return m_gl->isTexture(maybe.value()->glObject());
}

void WebGLRenderingContext::lineWidth(GLfloat width)
{
    ENTER_CONTEXT_SCOPE();

    // NaN slips through the driver's width <= 0 check (WebGL 1.0 "NaN Line
    // Width").
    if (std::isnan(width)) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    m_gl->lineWidth(width);
}

static bool validateUniformBlocksMatch(const GCVector<WebGLShader*>& shaders)
{
    std::unordered_map<std::string, std::vector<std::string>> blockDefinitions;

    for (WebGLShader* shader : shaders) {
        if (!shader)
            continue;
        const std::string& src = shader->source();
        if (src.empty())
            continue;

        size_t pos = 0;
        while ((pos = src.find("uniform", pos)) != std::string::npos) {
            if (pos > 0 && (isalnum(src[pos - 1]) || src[pos - 1] == '_')) {
                pos += 7;
                continue;
            }

            size_t afterUniform = pos + 7;
            if (afterUniform >= src.size() ||
                !(src[afterUniform] == ' ' || src[afterUniform] == '\t' ||
                  src[afterUniform] == '\r' || src[afterUniform] == '\n')) {
                pos += 7;
                continue;
            }

            size_t nameStart = afterUniform;
            while (nameStart < src.size() &&
                   (src[nameStart] == ' ' || src[nameStart] == '\t' ||
                    src[nameStart] == '\r' || src[nameStart] == '\n')) {
                nameStart++;
            }

            size_t nameEnd = nameStart;
            while (nameEnd < src.size() &&
                   (isalnum(src[nameEnd]) || src[nameEnd] == '_')) {
                nameEnd++;
            }

            if (nameEnd == nameStart) {
                pos += 7;
                continue;
            }

            size_t braceOpen = nameEnd;
            while (braceOpen < src.size() &&
                   (src[braceOpen] == ' ' || src[braceOpen] == '\t' ||
                    src[braceOpen] == '\r' || src[braceOpen] == '\n')) {
                braceOpen++;
            }

            if (braceOpen >= src.size() || src[braceOpen] != '{') {
                pos = nameEnd;
                continue;
            }

            size_t braceClose = src.find('}', braceOpen + 1);
            if (braceClose == std::string::npos) {
                pos = braceOpen + 1;
                continue;
            }

            std::string blockName = src.substr(nameStart, nameEnd - nameStart);
            std::string body =
                src.substr(braceOpen + 1, braceClose - (braceOpen + 1));

            // Extract tokens (normalize whitespace and semicolons)
            std::vector<std::string> tokens;
            std::string cur;
            for (char c : body) {
                if (isspace(c) || c == ';') {
                    if (!cur.empty()) {
                        tokens.push_back(cur);
                        cur.clear();
                    }
                    if (c == ';') {
                        tokens.push_back(";");
                    }
                } else {
                    cur += c;
                }
            }
            if (!cur.empty()) {
                tokens.push_back(cur);
            }

            auto existing = blockDefinitions.find(blockName);
            if (existing == blockDefinitions.end()) {
                blockDefinitions[blockName] = tokens;
            } else {
                if (existing->second != tokens) {
                    // Uniform block declaration mismatch across shaders!
                    return false;
                }
            }

            pos = braceClose + 1;
        }
    }
    return true;
}

void WebGLRenderingContext::linkProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(program)) {
        return;
    }

    /*
        NOTE: No idea to handle the following for now. It may already be handled
        in GLES3: 6.26 Packing Restrictions for Uniforms and Varyings: The WebGL
        API further requires that if the packing algorithm fails either for the
        uniform variables of a shader or for the varying variables of a program,
        compilation or linking must fail.
    */

    hasNewGLError(); // Keep an earlier error from failing this call.
    m_gl->linkProgram(program->glObject());

    if (hasNewGLError()) {
        /*
            NOTE: No idea to handle the following for now. It may already be
            handled in GLES3: If the given program is also the the current
            program object in use as defined by useProgram, then: If the program
            is not linked successfully, the executable code referenced by the
            current rendering state is immediately invalidated. Further draw
            calls that utilize the current program generate an INVALID_OPERATION
            error. See Current program invalidated upon unsuccessful
            link(https://registry.khronos.org/webgl/specs/latest/1.0/#6.43).
        */
        // A rejected call, as while transform feedback uses the program
        // (OpenGL ES 3.0.6 §2.15.2), links nothing: the previous executable,
        // its uniforms and its locations stay (§2.5).
        return;
    }

    // The sampler table follows the driver's executable: a successful link
    // resets every uniform even where WebGL then fails the program, and after
    // a failed one the previous executable stays in use (OpenGL ES 2.0
    // §2.10.3, §2.10.4).
    GLint linkStatus = 0;
    m_gl->getProgramiv(program->glObject(), GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_TRUE) {
        program->updateSamplers(m_gl);
    }

    program->incrementLinkCount();
    program->setLinkFailed(false);

    for (WebGLShader* shader : program->getWebGLShaders()) {
        if (shader && shader->compileFailed()) {
            program->setLinkFailed(true);
            return;
        }
    }

    if (webGLVersion() >= 2) {
        if (!validateUniformBlocksMatch(program->getWebGLShaders())) {
            program->setLinkFailed(true);
            return;
        }
    }

    if (linkStatus != GL_TRUE) {
        program->setLinkFailed(true);
        return;
    }

    if (webGLVersion() >= 2) {
        program->updateUniformBlocks(m_gl);
    }

    GLint maxVertexAttribs = 0;
    m_gl->getIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);

    GLint numActiveAttribs = 0;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_ATTRIBUTES,
                       &numActiveAttribs);

    GLint maxNameLength = 0;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                       &maxNameLength);

    std::vector<std::tuple<std::string, GLuint, GLint>> activeAttribs;
    for (GLint i = 0; i < numActiveAttribs; ++i) {
        GLint size = 0;
        GLenum type = 0;
        GLsizei length = 0;
        std::vector<char> nameBuf(maxNameLength, '\0');
        m_gl->getActiveAttrib(program->glObject(), i, maxNameLength, &length,
                              &size, &type, &nameBuf[0]);
        std::string name(nameBuf.data(), length);

        GLint numLocations = 1;
        switch (type) {
        case GL_FLOAT_MAT2:
            numLocations = 2;
            break;
        case GL_FLOAT_MAT3:
            numLocations = 3;
            break;
        case GL_FLOAT_MAT4:
            numLocations = 4;
            break;
        default:
            break;
        }

        GLint location =
            m_gl->getAttribLocation(program->glObject(), name.c_str());
        if (location >= 0) {
            activeAttribs.push_back(std::make_tuple(
                name, static_cast<GLuint>(location), numLocations));
        }
    }

    for (const auto& attrib : activeAttribs) {
        const std::string& name = std::get<0>(attrib);
        GLuint location = std::get<1>(attrib);
        GLint numLocations = std::get<2>(attrib);

        if (location + numLocations > static_cast<GLuint>(maxVertexAttribs)) {
            program->setLinkFailed(true);
            return;
        }

        for (const auto& other : activeAttribs) {
            const std::string& otherName = std::get<0>(other);
            if (otherName == name) {
                continue;
            }

            GLuint otherLocation = std::get<1>(other);
            GLint otherNumLocations = std::get<2>(other);

            if (otherLocation >= location &&
                otherLocation < location + static_cast<GLuint>(numLocations)) {
                program->setLinkFailed(true);
                return;
            }
        }
    }
}

void WebGLRenderingContext::pixelStorei(GLenum pname, GLint param)
{
    ENTER_CONTEXT_SCOPE();

    switch (pname) {
    case kUNPACK_FLIP_Y_WEBGL:
        TRACE(WEBGL, "UNPACK_FLIP_Y_WEBGL", param);
        m_unpackFlipY = static_cast<bool>(param);
        break;
    case kUNPACK_PREMULTIPLY_ALPHA_WEBGL:
        TRACE(WEBGL, "UNPACK_PREMULTIPLY_ALPHA_WEBGL", param);
        m_unpackPremultiplyAlpha = static_cast<bool>(param);
        break;
    case kUNPACK_COLORSPACE_CONVERSION_WEBGL:
        TRACE(WEBGL, "UNPACK_COLORSPACE_CONVERSION_WEBGL", param);
        if (param == kBROWSER_DEFAULT_WEBGL || param == GL_NONE) {
            // NOTE: we don't do nothing for this as default.
            m_unpackColorspaceConversion = param;
        }
        break;
    case GL_PACK_ALIGNMENT:
        if (param != 1 && param != 2 && param != 4 && param != 8) {
            setGLError(GL_INVALID_VALUE, "Invalid pack alignment");
            return;
        }
        m_packAlignment = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_UNPACK_ALIGNMENT:
        if (param != 1 && param != 2 && param != 4 && param != 8) {
            setGLError(GL_INVALID_VALUE, "Invalid unpack alignment");
            return;
        }
        m_unpackAlignment = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_PACK_ROW_LENGTH:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative pack row length");
            return;
        }
        m_packRowLength = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_PACK_SKIP_PIXELS:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative pack skip pixels");
            return;
        }
        m_packSkipPixels = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_PACK_SKIP_ROWS:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative pack skip rows");
            return;
        }
        m_packSkipRows = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_UNPACK_ROW_LENGTH:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative unpack row length");
            return;
        }
        m_unpackRowLength = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_UNPACK_IMAGE_HEIGHT:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative unpack image height");
            return;
        }
        m_unpackImageHeight = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_UNPACK_SKIP_PIXELS:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative unpack skip pixels");
            return;
        }
        m_unpackSkipPixels = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_UNPACK_SKIP_ROWS:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative unpack skip rows");
            return;
        }
        m_unpackSkipRows = param;
        m_gl->pixelStorei(pname, param);
        break;
    case GL_UNPACK_SKIP_IMAGES:
        if (!isWebGL2RenderingContext()) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            setGLError(GL_INVALID_VALUE, "negative unpack skip images");
            return;
        }
        m_unpackSkipImages = param;
        m_gl->pixelStorei(pname, param);
        break;
    default:
        TRACE(WEBGL, KV(glValueString(pname)), KV(glValueString(param)));
        m_gl->pixelStorei(pname, param);
        break;
    }
}

void WebGLRenderingContext::polygonOffset(GLfloat factor, GLfloat units)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->polygonOffset(factor, units);
}

void WebGLRenderingContext::renderbufferStorage(GLenum target,
                                                GLenum internalformat,
                                                GLsizei width, GLsizei height)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_RENDERBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }
    // OpenGL ES 2.0 §4.4.3 and WebGL 1.0 "Framebuffer Object Attachments":
    // the WebGL 1 formats. No WebGL 1 extension exposed here adds one
    // (EXT_sRGB's SRGB8_ALPHA8 included).
    if (webGLVersion() == 1 && internalformat != GL_RGBA4 &&
        internalformat != GL_RGB565 && internalformat != GL_RGB5_A1 &&
        internalformat != GL_DEPTH_COMPONENT16 &&
        internalformat != GL_STENCIL_INDEX8 &&
        internalformat != GL_DEPTH_STENCIL) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (width < 0 || height < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (internalformat == 0x881B /* GL_RGB16F */ ||
        internalformat == 0x8815 /* GL_RGB32F */) {
        setGLError(GL_INVALID_ENUM, "RGB float formats are not renderable");
        return;
    }
    if (WebGLFramebuffer::isFloatColorFormat(internalformat)) {
        if (!isExtensionEnabled("EXT_color_buffer_float")) {
            setGLError(GL_INVALID_ENUM, "EXT_color_buffer_float not enabled");
            return;
        }
    }

    GLint maxSize = 0;
    m_gl->getIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxSize);
    if (maxSize > 0 && (width > maxSize || height > maxSize)) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!m_boundRenderbuffer.hasValue() ||
        m_boundRenderbuffer.value() == nullptr ||
        m_boundRenderbuffer.value()->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLenum glInternalFormat = internalformat;
    if (glInternalFormat == GL_DEPTH_STENCIL) {
        glInternalFormat = GL_DEPTH24_STENCIL8;
    }

    m_gl->renderbufferStorage(target, glInternalFormat, width, height);
    if (!hasNewGLError()) {
        m_boundRenderbuffer.value()->setStorage(internalformat, width, height);
        initializeRenderbuffer(m_boundRenderbuffer.value());
    }
}

void WebGLRenderingContext::sampleCoverage(GLclampf value, GLboolean invert)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->sampleCoverage(value, invert);
}

void WebGLRenderingContext::scissor(GLint x, GLint y, GLsizei width,
                                    GLsizei height)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->scissor(x, y, width, height);
}

void WebGLRenderingContext::texParameterf(GLenum target, GLenum pname,
                                          GLfloat param)
{
    // OpenGL ES 2.0 §3.7.4: every WebGL 1 texture parameter but the
    // anisotropy takes an enum, which texParameteri validates. A value that
    // is no int is no enum either (and is not cast: NaN and out-of-range
    // floats make a float-to-int cast undefined).
    if (pname != GL_TEXTURE_MAX_ANISOTROPY_EXT) {
        const double value = param;
        texParameteri(target, pname,
                      value > -2147483649.0 && value < 2147483648.0
                          ? static_cast<GLint>(value)
                          : 0);
        return;
    }

    ENTER_CONTEXT_SCOPE();

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT &&
        !isExtensionEnabled("EXT_texture_filter_anisotropic")) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (!hasBoundTexture(target)) {
        // If an attempt is made to call this function with no WebGLTexture
        // bound, an INVALID_OPERATION error is generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->texParameterf(target, pname, param);
}

void WebGLRenderingContext::texParameteri(GLenum target, GLenum pname,
                                          GLint param)
{
    ENTER_CONTEXT_SCOPE();

    if (pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T) {
        if (param != GL_REPEAT && param != GL_CLAMP_TO_EDGE &&
            param != GL_MIRRORED_REPEAT) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        if (param != GL_NEAREST && param != GL_LINEAR) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_MIN_FILTER) {
        if (param != GL_NEAREST && param != GL_LINEAR &&
            param != GL_NEAREST_MIPMAP_NEAREST &&
            param != GL_LINEAR_MIPMAP_NEAREST &&
            param != GL_NEAREST_MIPMAP_LINEAR &&
            param != GL_LINEAR_MIPMAP_LINEAR) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) {
        if (!isExtensionEnabled("EXT_texture_filter_anisotropic")) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (!hasBoundTexture(target)) {
        // If an attempt is made to call this function with no WebGLTexture
        // bound, an INVALID_OPERATION error is generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    hasNewGLError(); // Keep an earlier error from failing this call.
    m_gl->texParameteri(target, pname, param);
    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && !hasNewGLError()) {
        texture->setSamplingParameter(pname, param);
    }
}

void WebGLRenderingContext::uniform1f(
    Optional<WebGLUniformLocation*> maybeUniform, GLfloat x)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    // Each of the uniform* functions sets the specified uniform or uniforms to
    // the values provided.
    if (!isFromCurrentProgram(uniform)) {
        // If the passed location is not null and was not obtained from the
        // currently used program via an earlier call to getUniformLocation, an
        // INVALID_OPERATION error will be generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    // If the passed location is null, the data passed in will be silently
    // ignored and no uniform variables will be changed.
    m_gl->uniform1f(uniform->location(), x);
}

void WebGLRenderingContext::uniform2f(
    Optional<WebGLUniformLocation*> maybeUniform, GLfloat x, GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->uniform2f(uniform->location(), x, y);
}

void WebGLRenderingContext::uniform3f(
    Optional<WebGLUniformLocation*> maybeUniform, GLfloat x, GLfloat y,
    GLfloat z)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->uniform3f(uniform->location(), x, y, z);
}

void WebGLRenderingContext::uniform4f(
    Optional<WebGLUniformLocation*> maybeUniform, GLfloat x, GLfloat y,
    GLfloat z, GLfloat w)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->uniform4f(uniform->location(), x, y, z, w);
}

void WebGLRenderingContext::uniform1i(
    Optional<WebGLUniformLocation*> maybeUniform, GLint x)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    WebGLProgram* program = uniform->program();
    Optional<size_t> sampler = program->samplerIndex(uniform->location());
    if (!sampler) {
        m_gl->uniform1i(uniform->location(), x);
        return;
    }
    if (!areTextureUnits(&x, 1)) {
        setGLError(GL_INVALID_VALUE, "sampler value is no texture unit");
        return;
    }
    hasNewGLError(); // Keep an earlier error from failing this call.
    m_gl->uniform1i(uniform->location(), x);
    if (!hasNewGLError()) {
        program->setSamplerUnits(sampler.value(), &x, 1);
    }
}

void WebGLRenderingContext::uniform2i(
    Optional<WebGLUniformLocation*> maybeUniform, GLint x, GLint y)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->uniform2i(uniform->location(), x, y);
}

void WebGLRenderingContext::uniform3i(
    Optional<WebGLUniformLocation*> maybeUniform, GLint x, GLint y, GLint z)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->uniform3i(uniform->location(), x, y, z);
}

void WebGLRenderingContext::uniform4i(
    Optional<WebGLUniformLocation*> maybeUniform, GLint x, GLint y, GLint z,
    GLint w)
{
    ENTER_CONTEXT_SCOPE();

    if (!maybeUniform.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = maybeUniform.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->uniform4i(uniform->location(), x, y, z, w);
}

void WebGLRenderingContext::useProgram(Optional<WebGLProgram*> maybeProgram)
{
    ENTER_CONTEXT_SCOPE();

    Optional<WebGLProgram*> previous = m_state->webGLProgram();
    if (maybeProgram.hasValue()) {
        WebGLProgram* program = maybeProgram.value();
        if (!validateProgramOrShader(program)) {
            return;
        }
        // A program marked for deletion cannot be made current again.
        if (program->isDeleted()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        hasNewGLError(); // Keep an earlier error from failing this call.
        m_gl->useProgram(program->glObject());
        if (hasNewGLError()) {
            return;
        }
        m_state->setWebGLProgram(program);
    } else {
        m_gl->useProgram(0);
        m_state->setWebGLProgram(nullptr);
    }

    if (previous && previous.value()->isDeleted() &&
        m_state->webGLProgram() != previous) {
        releaseProgram(previous.value());
    }
}

void WebGLRenderingContext::validateProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    if (!validateProgramOrShader(program)) {
        return;
    }

    m_gl->validateProgram(program->glObject());
}

void WebGLRenderingContext::vertexAttrib1f(GLuint index, GLfloat x)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib1f(index, x);
    m_state->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGLRenderingContext::vertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib2f(index, x, y);
    m_state->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGLRenderingContext::vertexAttrib3f(GLuint index, GLfloat x, GLfloat y,
                                           GLfloat z)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib3f(index, x, y, z);
    m_state->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGLRenderingContext::vertexAttrib4f(GLuint index, GLfloat x, GLfloat y,
                                           GLfloat z, GLfloat w)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib4f(index, x, y, z, w);
    m_state->setGenericVertexAttribType(index, GL_FLOAT);
}

#define IMPLEMENT_VERTEX_ATTRIB_NFV(N)                                        \
    void WebGLRenderingContext::vertexAttrib##N##fv(GLuint index,             \
                                                    Float32List variant)      \
    {                                                                         \
        throwIfNotConverted(variant);                                         \
        ENTER_CONTEXT_SCOPE();                                                \
        m_state->setGenericVertexAttribType(index, GL_FLOAT);                 \
        if (variant.isFloat32ArrayValue()) {                                  \
            Float32ArrayObjectRef* values = variant.getFloat32ArrayValue();   \
            const size_t arrayLength = values->arrayLength();                 \
            uint8_t* rawBuffer = const_cast<uint8_t*>(values->rawBuffer());   \
            if (arrayLength < N) {                                            \
                setGLError(GL_INVALID_VALUE);                                 \
                return;                                                       \
            }                                                                 \
            m_gl->vertexAttrib##N##fv(index,                                  \
                                      reinterpret_cast<GLfloat*>(rawBuffer)); \
        } else {                                                              \
            STARFISH_ASSERT(variant.isSequenceOfGLfloatValue());              \
            /* Due to the memory size of double and float types, the raw      \
             * buffer returned by GCAtomicVector<double>::data() cannot be    \
             * used directly. Reconstructs a float buffer including values    \
             * converted from double. */                                      \
            const GCAtomicVector<double> v =                                  \
                variant.getSequenceOfGLfloatValue();                          \
            if (v.size() < N) {                                               \
                setGLError(GL_INVALID_VALUE);                                 \
                return;                                                       \
            }                                                                 \
            std::vector<GLfloat> vector;                                      \
            vector.reserve(v.size());                                         \
            for (const double& value : v) {                                   \
                vector.push_back(static_cast<GLfloat>(value));                \
            }                                                                 \
            m_gl->vertexAttrib##N##fv(index, vector.data());                  \
        }                                                                     \
    }

IMPLEMENT_VERTEX_ATTRIB_NFV(1)
IMPLEMENT_VERTEX_ATTRIB_NFV(2)
IMPLEMENT_VERTEX_ATTRIB_NFV(3)
IMPLEMENT_VERTEX_ATTRIB_NFV(4)

#undef IMPLEMENT_VERTEX_ATTRIB_NFV

void WebGLRenderingContext::vertexAttribPointer(GLuint index, GLint size,
                                                GLenum type,
                                                GLboolean normalized,
                                                GLsizei stride,
                                                WebGLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    // See Buffer Offset and Stride Requirements.
    GLsizei typeSize = 0;
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        typeSize = 1;
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        typeSize = 2;
        break;
    case GL_FLOAT:
        typeSize = 4;
        break;
    // OpenGL ES 3.0.6 §2.8 table 2.5; FIXED stays out (WebGL 1.0 "Fixed point
    // support").
    case GL_HALF_FLOAT:
        typeSize = webGLVersion() >= 2 ? 2 : 0;
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
        typeSize = webGLVersion() >= 2 ? 4 : 0;
        break;
    default:
        break;
    }
    if (!typeSize) {
        setGLError(GL_INVALID_ENUM);
        return;
    }
    // Rejected here, before the driver: Mesa applies the pointer of a call it
    // rejects for a negative stride, which the recorded one then misses.
    if (stride < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }
    if (offset % typeSize != 0 || stride % typeSize != 0) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (offset < 0) {
        // If offset is negative, an INVALID_VALUE error will be generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!getState()->getBoundBuffer(GL_ARRAY_BUFFER).hasValue() &&
        offset != 0) {
        // If no WebGLBuffer is bound to the ARRAY_BUFFER target and offset is
        // non-zero, an INVALID_OPERATION error will be generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (stride > kMaximumSupportedStride) {
        // In WebGL, the maximum supported stride is 255
        setGLError(GL_INVALID_VALUE);
        return;
    }

    /*
        The following errors are handled in GLES3.
        (https://docs.gl/es3/glVertexAttribPointer)
        We don't do any additional validation for them:
        - GL_INVALID_VALUE if size is not 1, 2, 3 or 4.
        - GL_INVALID_ENUM if type is not an accepted value.
        - GL_INVALID_VALUE if stride is negative.
        - GL_INVALID_VALUE if index is greater than or equal to
            GL_MAX_VERTEX_ATTRIBS.
        - GL_INVALID_OPERATION if type is GL_INT_2_10_10_10_REV or
            GL_UNSIGNED_INT_2_10_10_10_REV and size is not 4.
        - GL_INVALID_OPERATION a non-zero vertex array object is bound, zero is
            bound to the GL_ARRAY_BUFFER buffer object binding point and the
            pointer argument is not NULL.
    */
    m_gl->vertexAttribPointer(index, size, type, normalized, stride,
                              reinterpret_cast<void*>(clampToGLintptr(offset)));
    if (hasNewGLError()) {
        return;
    }
    m_state->setVertexAttribType(index, GL_FLOAT);
    m_state->setVertexAttribPointer(index,
                                    m_state->getBoundBuffer(GL_ARRAY_BUFFER),
                                    size, type, stride, offset);
}

void WebGLRenderingContext::viewport(uint32_t x, uint32_t y, uint32_t width,
                                     uint32_t height)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->viewport(x, y, width, height);
}

// WebGLRenderingContextOverloads

// OpenGL ES 2.0 §2.9 has these three usages; READ and COPY are ES 3.0's.
static bool isWebGL1BufferUsage(GLenum usage)
{
    return usage == GL_STREAM_DRAW || usage == GL_STATIC_DRAW ||
           usage == GL_DYNAMIC_DRAW;
}

void WebGLRenderingContext::bufferData(GLenum target, WebGLsizeiptr size,
                                       GLenum usage)
{
    ENTER_CONTEXT_SCOPE();

    if (size < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }
    if (!isWebGL2RenderingContext() && !isWebGL1BufferUsage(usage)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    // The driver reports a bad target or usage, a missing buffer and an
    // allocation failure (OUT_OF_MEMORY) before anything is filled.
    hasNewGLError(); // Keep an earlier error from failing this call.
    const GLsizeiptr glSize = clampToGLsizeiptr(size);
    m_gl->bufferData(target, glSize, nullptr, usage);
    if (!bufferDataSucceeded(target)) {
        return;
    }

    // The buffer is initialized to 0 (WebGL 1.0 §5.14.5). Fill it in bounded
    // chunks so a large buffer costs no client-side copy of its size.
    const GLsizeiptr chunk = std::min<GLsizeiptr>(glSize, 64 * 1024);
    const std::vector<unsigned char> zeros(chunk);
    for (GLsizeiptr offset = 0; offset < glSize; offset += chunk) {
        m_gl->bufferSubData(target, offset, std::min(chunk, glSize - offset),
                            zeros.data());
    }

    Optional<WebGLBuffer*> maybeBound = m_state->getBoundBuffer(target);
    if (maybeBound.hasValue() && maybeBound.value()) {
        maybeBound.value()->setByteLength(glSize);
    }
}

void WebGLRenderingContext::bufferData(GLenum target,
                                       Optional<AllowSharedBufferSource> data,
                                       GLenum usage)
{
    ENTER_CONTEXT_SCOPE();

    if (!data.hasValue()) {
        setGLError(GL_INVALID_VALUE);
        return;
    }
    if (!isWebGL2RenderingContext() && !isWebGL1BufferUsage(usage)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    // As in bufferData(size): the recorded size follows the GL's buffer only
    // when this call succeeded, and an earlier error must not fail it.
    hasNewGLError();
    GLsizeiptr byteLength = 0;
    if (data.value().isArrayBufferValue()) {
        ScriptArrayBuffer buffer = data.value().getArrayBufferValue();
        byteLength = buffer->byteLength();
        m_gl->bufferData(target, byteLength, buffer->rawBuffer(), usage);
    } else if (data.value().isArrayBufferViewValue()) {
        ScriptArrayBufferView view = data.value().getArrayBufferViewValue();
        byteLength = view->byteLength();
        m_gl->bufferData(target, byteLength, view->rawBuffer(), usage);
    } else if (data.value().isSharedArrayBufferValue()) {
        ScriptSharedArrayBuffer buffer =
            data.value().getSharedArrayBufferValue();
        byteLength = buffer->byteLength();
        m_gl->bufferData(target, byteLength, buffer->rawBuffer(), usage);
    } else {
        setGLError(GL_INVALID_VALUE);
        return;
    }
    if (!bufferDataSucceeded(target)) {
        return;
    }

    Optional<WebGLBuffer*> maybeBound = m_state->getBoundBuffer(target);
    if (maybeBound.hasValue() && maybeBound.value()) {
        maybeBound.value()->setByteLength(byteLength);
    }
}

void WebGLRenderingContext::bufferSubData(GLenum target, WebGLintptr offset,
                                          AllowSharedBufferSource data)
{
    throwIfNotConverted(data);
    ENTER_CONTEXT_SCOPE();

    size_t byteLength = 0;
    const void* source = nullptr;
    if (data.isArrayBufferValue()) {
        ScriptArrayBuffer buffer = data.getArrayBufferValue();
        byteLength = buffer->byteLength();
        source = buffer->rawBuffer();
    } else if (data.isArrayBufferViewValue()) {
        ScriptArrayBufferView view = data.getArrayBufferViewValue();
        byteLength = view->byteLength();
        source = view->rawBuffer();
    } else if (data.isSharedArrayBufferValue()) {
        ScriptSharedArrayBuffer buffer = data.getSharedArrayBufferValue();
        byteLength = buffer->byteLength();
        source = buffer->rawBuffer();
    } else {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!isValidBufferTarget(target)) {
        setGLError(GL_INVALID_ENUM, "invalid target");
        return;
    }
    // Every case is decided here, so no unchecked offset reaches the GL
    // (OpenGL ES 2.0.25 §2.9, WebGL 1.0 §5.14.5).
    Optional<WebGLBuffer*> buffer = getState()->getBoundBuffer(target);
    if (!buffer) {
        setGLError(GL_INVALID_OPERATION, "no buffer is bound to target");
        return;
    }
    if (offset < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset");
        return;
    }
    // The size every bufferData recorded is compared, not a GL query, so no
    // unrelated pending GL error can drop the upload.
    if (!isRangeWithin(offset, byteLength, buffer.value()->byteLength())) {
        setGLError(GL_INVALID_VALUE, "buffer too small");
        return;
    }

    m_gl->bufferSubData(target, clampToGLintptr(offset), byteLength, source);
}

size_t WebGLRenderingContext::viewElementSize(ScriptArrayBufferView view)
{
    if (view->isInt16ArrayObject() || view->isUint16ArrayObject() ||
        view->isFloat16ArrayObject()) {
        return 2;
    }
    if (view->isInt32ArrayObject() || view->isUint32ArrayObject() ||
        view->isFloat32ArrayObject()) {
        return 4;
    }
    if (view->isFloat64ArrayObject() || view->isBigInt64ArrayObject() ||
        view->isBigUint64ArrayObject()) {
        return 8;
    }
    return 1;
}

bool WebGLRenderingContext::isValidBufferTarget(GLenum target)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
    case GL_ELEMENT_ARRAY_BUFFER:
        return true;
    case GL_COPY_READ_BUFFER:
    case GL_COPY_WRITE_BUFFER:
    case GL_PIXEL_PACK_BUFFER:
    case GL_PIXEL_UNPACK_BUFFER:
    case GL_TRANSFORM_FEEDBACK_BUFFER:
    case GL_UNIFORM_BUFFER:
        return isWebGL2RenderingContext();
    default:
        return false;
    }
}

bool WebGLRenderingContext::bufferDataSucceeded(GLenum target)
{
    GLenum error = m_gl->getError();
    if (error == GL_NO_ERROR) {
        return true;
    }
    setGLError(error);
    // A rejected call leaves the buffer as it was, but after OUT_OF_MEMORY its
    // data store is undefined (OpenGL ES 2.0.25 §2.5), so no later check may
    // trust the old size.
    if (error == GL_OUT_OF_MEMORY) {
        if (Optional<WebGLBuffer*> bound = m_state->getBoundBuffer(target)) {
            bound.value()->setByteLength(0);
        }
    }
    return false;
}

// A compressed format is only accepted once the WebGL extension defining it
// has been enabled on this context; the core specifications define none.
// https://registry.khronos.org/webgl/specs/latest/1.0/#COMPRESSED_TEXTURE_SUPPORT
bool WebGLRenderingContext::isCompressedFormatEnabled(GLenum format)
{
    Optional<CompressedFormatInfo> info = Pixel::compressedFormatInfo(format);
    return info && isExtensionEnabled(info.value().extension);
}

// Checks shared by compressed uploads before any GL call. imageSize must match
// the format's block layout exactly, which also keeps the driver from reading
// past the end of the client data.
bool WebGLRenderingContext::checkCompressedTexImage(
    GLenum target, bool is3D, GLenum format, GLsizei width, GLsizei height,
    GLsizei depth, size_t imageSize)
{
    const bool isValidTarget =
        is3D ? (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY)
             : (target == GL_TEXTURE_2D ||
                (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
                 target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));
    if (!isValidTarget) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return false;
    }
    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return false;
    }
    if (!isCompressedFormatEnabled(format)) {
        setGLError(GL_INVALID_ENUM,
                   StringUtils::formatString(
                       "compressed format (0x%04X) is not enabled.", format)
                       .c_str());
        return false;
    }
    if (width < 0 || height < 0 || depth < 0) {
        setGLError(GL_INVALID_VALUE, "dimensions are negative.");
        return false;
    }

    const CompressedFormatInfo info =
        Pixel::compressedFormatInfo(format).value();
    const uint64_t blocksWide =
        (static_cast<uint64_t>(width) + info.blockWidth - 1) / info.blockWidth;
    const uint64_t blocksHigh =
        (static_cast<uint64_t>(height) + info.blockHeight - 1) /
        info.blockHeight;
    const uint64_t layerSize = blocksWide * blocksHigh * info.bytesPerBlock;
    // GLsizei bounds imageSize, so an image larger than that cannot match.
    const uint64_t maxImageSize = std::numeric_limits<GLsizei>::max();
    if (depth != 0 && layerSize > maxImageSize / depth) {
        setGLError(GL_INVALID_VALUE, "image is too large.");
        return false;
    }
    if (imageSize != layerSize * depth) {
        setGLError(GL_INVALID_VALUE,
                   "data size does not match the format and dimensions.");
        return false;
    }
    return true;
}

void WebGLRenderingContext::compressedTexImage(
    GLenum target, bool is3D, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLsizei depth, GLint border, size_t imageSize,
    const void* data)
{
    if (!checkCompressedTexImage(target, is3D, internalformat, width, height,
                                 depth, imageSize)) {
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }

    if (is3D) {
        m_gl->compressedTexImage3D(target, level, internalformat, width, height,
                                   depth, border, imageSize, data);
    } else {
        m_gl->compressedTexImage2D(target, level, internalformat, width, height,
                                   border, imageSize, data);
    }

    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   depth);
    }
}

// Block alignment of the region and the format matching the level are left to
// the GL, which reports both as INVALID_OPERATION as WebGL requires.
void WebGLRenderingContext::compressedTexSubImage(
    GLenum target, bool is3D, GLint level, GLint xoffset, GLint yoffset,
    GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format,
    size_t imageSize, const void* data)
{
    if (!checkCompressedTexImage(target, is3D, format, width, height, depth,
                                 imageSize)) {
        return;
    }

    if (is3D) {
        m_gl->compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset,
                                      width, height, depth, format, imageSize,
                                      data);
    } else {
        m_gl->compressedTexSubImage2D(target, level, xoffset, yoffset, width,
                                      height, format, imageSize, data);
    }
}

void WebGLRenderingContext::compressedTexImage2D(GLenum target, GLint level,
                                                 GLenum internalformat,
                                                 GLsizei width, GLsizei height,
                                                 GLint border,
                                                 ScriptArrayBufferView data)
{
    ENTER_CONTEXT_SCOPE();

    compressedTexImage(target, false, level, internalformat, width, height, 1,
                       border, data->byteLength(), data->rawBuffer());
}

void WebGLRenderingContext::compressedTexSubImage2D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
    GLsizei height, GLenum format, ScriptArrayBufferView data)
{
    ENTER_CONTEXT_SCOPE();

    compressedTexSubImage(target, false, level, xoffset, yoffset, 0, width,
                          height, 1, format, data->byteLength(),
                          data->rawBuffer());
}

size_t WebGLRenderingContext::getBytesPerPixel(GLenum format, GLenum type)
{
    return Pixel::getBytesPerPixel(format, type, webGLVersion());
}

// size_t arithmetic that refuses to wrap. (The unsigned multiply of the
// vendored CheckedArithmetic.h misses overflows and takes x * 0 for one.)
static bool multiplyChecked(size_t& value, size_t factor)
{
    if (factor && value > std::numeric_limits<size_t>::max() / factor) {
        return false;
    }
    value *= factor;
    return true;
}

static bool addChecked(size_t& value, size_t addend)
{
    if (value > std::numeric_limits<size_t>::max() - addend) {
        return false;
    }
    value += addend;
    return true;
}

// The bytes of client memory a width x height x depth pixel transfer spans,
// from the start of the buffer to the end of its last pixel, under the given
// pixel store layout (OpenGL ES 3.0.6 §3.7.4 for unpacking, §4.3.2 for
// packing); imageHeight and skipImages only count for 3D transfers. Returns
// false when the span does not fit in size_t. rowStride is the byte distance
// between rows.
static bool pixelStoreSpan(size_t bytesPerPixel, size_t width, size_t height,
                           size_t depth, size_t rowLength, size_t imageHeight,
                           size_t alignment, size_t skipPixels, size_t skipRows,
                           size_t skipImages, size_t& span, size_t& rowStride)
{
    size_t stride = rowLength;
    if (!multiplyChecked(stride, bytesPerPixel) ||
        !addChecked(stride, alignment - 1)) {
        return false;
    }
    rowStride = stride / alignment * alignment;

    // Images first: a 2D transfer has none, whatever its image height.
    size_t images = skipImages;
    size_t rows = skipRows;
    size_t pixels = skipPixels;
    if (!addChecked(images, depth - 1) ||
        !multiplyChecked(images, imageHeight) ||
        !multiplyChecked(images, rowStride) || !addChecked(rows, height - 1) ||
        !multiplyChecked(rows, rowStride) || !addChecked(pixels, width) ||
        !multiplyChecked(pixels, bytesPerPixel) || !addChecked(images, rows) ||
        !addChecked(images, pixels)) {
        return false;
    }
    span = images;
    return true;
}

bool WebGLRenderingContext::computeRequiredPackBytes(GLsizei width,
                                                     GLsizei height,
                                                     GLenum format, GLenum type,
                                                     size_t& requiredBytes)
{
    if (width < 0 || height < 0) {
        setGLError(GL_INVALID_VALUE, "negative width or height");
        return false;
    }

    if (width == 0 || height == 0) {
        requiredBytes = 0;
        return true;
    }

    if (isWebGL2RenderingContext()) {
        size_t actualRowLength = (m_packRowLength > 0)
                                     ? static_cast<size_t>(m_packRowLength)
                                     : static_cast<size_t>(width);
        if (m_packRowLength > 0 && m_packRowLength < width) {
            setGLError(GL_INVALID_OPERATION, "PACK_ROW_LENGTH < width");
            return false;
        }
        if (static_cast<size_t>(m_packSkipPixels) + static_cast<size_t>(width) >
            actualRowLength) {
            setGLError(GL_INVALID_OPERATION,
                       "PACK_SKIP_PIXELS + width > rowLength");
            return false;
        }
    }

    size_t bytesPerPixel = getBytesPerPixel(format, type);
    if (bytesPerPixel == 0) {
        setGLError(GL_INVALID_ENUM, "invalid format or type");
        return false;
    }

    // WebGL 1 has no PACK_ROW_LENGTH or PACK_SKIP_*; they stay 0.
    size_t rowStride = 0;
    if (!pixelStoreSpan(bytesPerPixel, width, height, 1,
                        m_packRowLength > 0 ? m_packRowLength : width, 1,
                        m_packAlignment, m_packSkipPixels, m_packSkipRows, 0,
                        requiredBytes, rowStride)) {
        // No buffer can be that large.
        setGLError(GL_INVALID_OPERATION, "pixel store layout overflows");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::computeRequiredUnpackBytes(
    GLenum target, GLsizei width, GLsizei height, GLsizei depth, GLenum format,
    GLenum type, size_t& requiredBytes, size_t& rowStride)
{
    if (width < 0 || height < 0 || depth < 0) {
        setGLError(GL_INVALID_VALUE, "negative dimension");
        return false;
    }

    if (width == 0 || height == 0 || depth == 0) {
        requiredBytes = 0;
        return true;
    }

    size_t bytesPerPixel = getBytesPerPixel(format, type);
    if (bytesPerPixel == 0) {
        setGLError(isKnownFormatAndType(format, type) ? GL_INVALID_OPERATION
                                                      : GL_INVALID_ENUM,
                   "invalid format and type combination");
        return false;
    }

    // UNPACK_IMAGE_HEIGHT and UNPACK_SKIP_IMAGES apply to every 3D upload,
    // whatever its depth (OpenGL ES 3.0.6 §3.8.3).
    const bool is3D = target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY;
    // WebGL 1 has no UNPACK_ROW_LENGTH, UNPACK_IMAGE_HEIGHT or UNPACK_SKIP_*;
    // they stay 0.
    const size_t rowLength = m_unpackRowLength > 0 ? m_unpackRowLength : width;
    const size_t imageHeight =
        m_unpackImageHeight > 0 ? m_unpackImageHeight : height;

    // WebGL 2.0, "Pixel store parameter constraints".
    if (static_cast<size_t>(m_unpackSkipPixels) + width > rowLength) {
        setGLError(GL_INVALID_OPERATION,
                   "UNPACK_SKIP_PIXELS + width > rowLength");
        return false;
    }
    if (is3D && static_cast<size_t>(m_unpackSkipRows) + height > imageHeight) {
        setGLError(GL_INVALID_OPERATION,
                   "UNPACK_SKIP_ROWS + height > imageHeight");
        return false;
    }

    if (!pixelStoreSpan(bytesPerPixel, width, height, depth, rowLength,
                        is3D ? imageHeight : 0, m_unpackAlignment,
                        m_unpackSkipPixels, m_unpackSkipRows,
                        is3D ? m_unpackSkipImages : 0, requiredBytes,
                        rowStride)) {
        // No buffer can be that large.
        setGLError(GL_INVALID_OPERATION, "pixel store layout overflows");
        return false;
    }
    return true;
}

void WebGLRenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                       GLsizei height, GLenum format,
                                       GLenum type,
                                       Optional<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (pixels.hasValue()) {
        ArrayBufferViewRef* pixelsView = pixels.getValue();

        // 1. If the types don't match, an INVALID_OPERATION error is generated.
        bool isTypeMatched = false;
        if (type == GL_UNSIGNED_BYTE) {
            isTypeMatched = (pixelsView->isUint8ArrayObject() ||
                             pixelsView->isUint8ClampedArrayObject());
        } else if (type == GL_UNSIGNED_SHORT_5_6_5 ||
                   type == GL_UNSIGNED_SHORT_4_4_4_4 ||
                   type == GL_UNSIGNED_SHORT_5_5_5_1) {
            isTypeMatched = pixelsView->isUint16ArrayObject();
        } else if (type == GL_FLOAT) {
            isTypeMatched = pixelsView->isFloat32ArrayObject();
        } else if (webGLVersion() == 2) {
            if (type == GL_BYTE) {
                isTypeMatched = pixelsView->isInt8ArrayObject();
            } else if (type == GL_SHORT) {
                isTypeMatched = pixelsView->isInt16ArrayObject();
            } else if (type == GL_UNSIGNED_SHORT || type == GL_HALF_FLOAT) {
                isTypeMatched = pixelsView->isUint16ArrayObject();
            } else if (type == GL_INT) {
                isTypeMatched = pixelsView->isInt32ArrayObject();
            } else if (type == GL_UNSIGNED_INT ||
                       type == GL_UNSIGNED_INT_2_10_10_10_REV ||
                       type == GL_UNSIGNED_INT_10F_11F_11F_REV ||
                       type == GL_UNSIGNED_INT_5_9_9_9_REV) {
                isTypeMatched = pixelsView->isUint32ArrayObject();
            }
        }

        if (!isTypeMatched) {
            setGLError(GL_INVALID_OPERATION, "Type not matched with view");
            return;
        }

        // 2. Only two combinations of format and type are accepted. The first
        //    is format RGBA and type UNSIGNED_BYTE. The second is an
        //    implementation-chosen format.

        // As for webgl/1.0.3/conformance/reading/read-pixels-test.html:162,
        // GL_INVALID_ENUM needs to be set for the luminance.
        if ((format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) &&
            type == GL_UNSIGNED_BYTE) {
            setGLError(GL_INVALID_ENUM);
            return;
        }

        bool isValidCombination = false;
        if (webGLVersion() == 1) {
            isValidCombination =
                (format == GL_RGBA && type == GL_UNSIGNED_BYTE);
        } else if (webGLVersion() == 2) {
            bool isIntegerFormat =
                (format == GL_RGBA_INTEGER || format == GL_RGB_INTEGER ||
                 format == GL_RG_INTEGER || format == GL_RED_INTEGER);
            bool isIntegerType =
                (type == GL_INT || type == GL_UNSIGNED_INT ||
                 type == GL_SHORT || type == GL_UNSIGNED_SHORT ||
                 type == GL_BYTE || type == GL_UNSIGNED_BYTE ||
                 type == GL_UNSIGNED_INT_2_10_10_10_REV);
            bool isNormalizedFloatFormat =
                (format == GL_RGBA || format == GL_RGB || format == GL_RG ||
                 format == GL_RED);
            bool isNormalizedFloatType =
                (type == GL_UNSIGNED_BYTE || type == GL_BYTE ||
                 type == GL_FLOAT || type == GL_HALF_FLOAT ||
                 type == GL_UNSIGNED_SHORT_5_6_5 ||
                 type == GL_UNSIGNED_SHORT_4_4_4_4 ||
                 type == GL_UNSIGNED_SHORT_5_5_5_1 ||
                 type == GL_UNSIGNED_INT_2_10_10_10_REV);
            isValidCombination =
                (isNormalizedFloatFormat && isNormalizedFloatType) ||
                (isIntegerFormat && isIntegerType);
        }

        if (!isValidCombination) {
            std::string msg = StringUtils::formatString(
                "Invalid combination of format=0x%x and type=0x%x", format,
                type);
            setGLError(GL_INVALID_OPERATION, msg.c_str());
            return;
        }

        size_t byteLengthOfPixels = 0;
        if (!computeRequiredPackBytes(width, height, format, type,
                                      byteLengthOfPixels)) {
            return;
        }
        size_t byteLengthOfView = pixels->byteLength();

        if (byteLengthOfView < byteLengthOfPixels) {
            // If pixels is non-null, but is not large enough to retrieve all of
            // the pixels in the specified rectangle taking into account pixel
            // store modes, an INVALID_OPERATION error is generated.
            setGLError(GL_INVALID_OPERATION, "View buffer too small");
            return;
        }

        /*
            TODO: 6.28 Reading From a Missing Attachment

            In the OpenGL ES 2.0 API, it is not specified what happens when a
            command tries to source data from a missing attachment, such as
            ReadPixels of color data from a complete framebuffer that does not
            have a color attachment.

            In the WebGL API, any [Read Operations] that require data from an
            attachment that is missing will generate an INVALID_OPERATION error.
        */

        /*
            TODO: 6.29 Drawing To a Missing Attachment

            If this function attempts to read from a complete framebuffer with a
            missing color attachment, an INVALID_OPERATION error is generated
            per Reading from a Missing Attachment.

            In the OpenGL ES 2.0 API, it is not specified what happens when a
            command tries to draw to a missing attachment, such as clearing a
            draw buffer from a complete framebuffer that does not have a color
            attachment.

            In the WebGL API, any [Draw Operations] that draw to an attachment
            that is missing will draw nothing to that attachment. No error is
            generated.
        */

        if (!validateFramebufferComplete(GL_READ_FRAMEBUFFER)) {
            return;
        }

        GLvoid* data = pixelsView->rawBuffer();

        completePendingJobs();
        m_gl->readPixels(x, y, width, height, format, type, data);
    } else {
        // If pixels is null, an INVALID_VALUE error is generated.
        setGLError(GL_INVALID_VALUE);
    }
}

TexImageHelper::TexImageHelper(size_t width, size_t height, size_t stride,
                               GLenum format, void* data)
{
    STARFISH_ASSERT(data != nullptr);

    m_sourceImage.width = width;
    m_sourceImage.height = height;
    m_sourceImage.stride = stride;
    m_sourceImage.format = format;
    m_sourceImage.data = static_cast<unsigned char*>(data);
    m_isNativeImageDataUsed = false;
    m_width = width;
    m_height = height;
}

TexImageHelper::TexImageHelper(NativeImageData* imageData, GLenum format)
{
    STARFISH_ASSERT(imageData != nullptr);

    m_sourceImage.width = imageData->width();
    m_sourceImage.height = imageData->height();
    m_sourceImage.stride = imageData->stride();
    m_sourceImage.format = format;
    m_sourceImage.data = static_cast<unsigned char*>(imageData->data());
    m_isNativeImageDataUsed = true;
    m_width = m_sourceImage.width;
    m_height = m_sourceImage.height;
}

TexImageHelper::TexImageHelper(::Starfish::ImageData* imageData, GLenum format)
{
    m_sourceImage.width = imageData->width();
    m_sourceImage.height = imageData->height();
    m_sourceImage.stride = imageData->width() * 4;
    m_sourceImage.format = format;
    m_sourceImage.data = imageData->data()->rawBuffer();
    m_isNativeImageDataUsed = false;
    m_isImageDataUsed = true;
    m_width = m_sourceImage.width;
    m_height = m_sourceImage.height;
}

bool TexImageHelper::allocate(size_t byteLength)
{
    m_isConverted = true;
    if (byteLength == 0) {
        return true;
    }
    // calloc, not a vector: an oversized request fails instead of aborting,
    // and pages the conversion never writes stay uncommitted.
    m_data.reset(static_cast<unsigned char*>(calloc(byteLength, 1)));
    return !!m_data;
}

GLenum TexImageHelper::draw(const bool needsFlipY,
                            const bool needsPremultiplyAlpha, const GLenum type,
                            const size_t bytesPerPixel)
{
    const size_t width = m_sourceImage.width;
    const size_t height = m_sourceImage.height;
    const size_t rowBytes = width * bytesPerPixel;
    if (!allocate(height * rowBytes)) {
        return GL_OUT_OF_MEMORY;
    }

    for (size_t row = 0; row < height && rowBytes; row++) {
        unsigned char* dest =
            m_data.get() + (needsFlipY ? height - 1 - row : row) * rowBytes;
        memcpy(dest, m_sourceImage.data + row * m_sourceImage.stride, rowBytes);
        if (needsPremultiplyAlpha) {
            for (size_t column = 0; column < width; column++) {
                premultiplyPixel(dest + column * bytesPerPixel, type);
            }
        }
    }
    return GL_NO_ERROR;
}

GLenum TexImageHelper::drawSubRectangle(const bool needsFlipY,
                                        const bool needsPremultiplyAlpha,
                                        const GLenum type, size_t skipPixels,
                                        size_t skipRows, size_t destWidth,
                                        size_t destHeight, size_t depth,
                                        size_t imageHeight)
{
    const size_t srcWidth = m_sourceImage.width;
    const size_t srcHeight = m_sourceImage.height;
    const unsigned char* image = m_sourceImage.data;
    const GLenum format = m_sourceImage.format;

    // The source channel (R, G, B, A = 0..3) of each destination component.
    // LUMINANCE is the red channel, as RED is (WebGL 2.0 §3.7.6).
    static const struct {
        GLenum format;
        uint8_t count;
        uint8_t channels[4];
    } layouts[] = {
        { GL_RGBA, 4, { 0, 1, 2, 3 } },
        { GL_RGBA_INTEGER, 4, { 0, 1, 2, 3 } },
        { GL_BGRA_EXT, 4, { 0, 1, 2, 3 } },
        { GL_RGB, 3, { 0, 1, 2 } },
        { GL_RGB_INTEGER, 3, { 0, 1, 2 } },
        { GL_RG, 2, { 0, 1 } },
        { GL_RG_INTEGER, 2, { 0, 1 } },
        { GL_RED, 1, { 0 } },
        { GL_RED_INTEGER, 1, { 0 } },
        { GL_LUMINANCE, 1, { 0 } },
        { GL_LUMINANCE_ALPHA, 2, { 0, 3 } },
        { GL_ALPHA, 1, { 3 } },
    };
    const uint8_t* channels = nullptr;
    size_t count = 0;
    for (const auto& layout : layouts) {
        if (layout.format == format) {
            channels = layout.channels;
            count = layout.count;
            break;
        }
    }
    const bool isInteger = format == GL_RGBA_INTEGER ||
                           format == GL_RGB_INTEGER ||
                           format == GL_RG_INTEGER || format == GL_RED_INTEGER;

    // Only 8-bit unsigned integers, half floats and floats are converted
    // to (WebGL 2.0 §3.7.6); anything else is not uploaded at all.
    size_t dstBytesPerPixel = 0;
    switch (type) {
    case GL_UNSIGNED_BYTE:
        dstBytesPerPixel = count;
        break;
    case GL_HALF_FLOAT:
    case GL_HALF_FLOAT_OES:
        dstBytesPerPixel = isInteger ? 0 : 2 * count;
        break;
    case GL_FLOAT:
        dstBytesPerPixel = isInteger ? 0 : 4 * count;
        break;
    case GL_UNSIGNED_SHORT_5_6_5:
        dstBytesPerPixel = format == GL_RGB ? 2 : 0;
        break;
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        dstBytesPerPixel = format == GL_RGBA ? 2 : 0;
        break;
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
        dstBytesPerPixel = format == GL_RGB ? 4 : 0;
        break;
    case GL_UNSIGNED_INT_2_10_10_10_REV:
        dstBytesPerPixel = format == GL_RGBA ? 4 : 0;
        break;
    default:
        break;
    }
    if (dstBytesPerPixel == 0) {
        return GL_INVALID_OPERATION;
    }

    bool needsColorConversion = false;
#if defined(PORT_PIXEL_ORDER_BGRA)
    if (m_isNativeImageDataUsed) {
        if (m_sourceImage.format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE &&
            WebGLExtensionRegistry::instance()
                .hasEXT_texture_format_BGRA8888()) {
            m_dataFormat = GL_BGRA_EXT;
            needsColorConversion = false;
        } else {
            needsColorConversion = true;
        }
    }
#endif

    // Every source is 4 bytes per pixel.
    const size_t srcStride = m_sourceImage.stride;
    m_width = destWidth;
    m_height = destHeight;
    if (!needsFlipY && !needsPremultiplyAlpha && !needsColorConversion &&
        (format == GL_RGBA || format == GL_BGRA_EXT) &&
        type == GL_UNSIGNED_BYTE && skipPixels == 0 && skipRows == 0 &&
        destWidth == srcWidth && destHeight == srcHeight && depth == 1 &&
        srcStride == srcWidth * 4) {
        // The source already is the upload.
        return GL_NO_ERROR;
    }

    size_t byteLength = dstBytesPerPixel;
    if (!multiplyChecked(byteLength, destWidth) ||
        !multiplyChecked(byteLength, destHeight) ||
        !multiplyChecked(byteLength, depth) || !allocate(byteLength)) {
        return GL_OUT_OF_MEMORY;
    }

    const size_t dstSliceStride = dstBytesPerPixel * destWidth * destHeight;
    const size_t dstRowStride = dstBytesPerPixel * destWidth;

    std::array<uint8_t, 4> order;
    if (needsColorConversion) {
        order = { 2, 1, 0, 3 };
    } else {
        order = { 0, 1, 2, 3 };
    }

    const size_t effectiveImageHeight =
        (imageHeight > 0) ? imageHeight : (depth > 1 ? destHeight : srcHeight);

    for (size_t d = 0; d < depth; d++) {
        size_t effectiveSliceIndex = needsFlipY ? (depth - 1 - d) : d;
        size_t dstSliceOffset = d * dstSliceStride;

        for (size_t r = 0; r < destHeight; r++) {
            size_t srcRow = 0;
            if (needsFlipY) {
                if (effectiveImageHeight >= 1 + skipRows + r) {
                    srcRow = effectiveSliceIndex * effectiveImageHeight +
                             (effectiveImageHeight - 1 - skipRows - r);
                }
            } else {
                srcRow =
                    effectiveSliceIndex * effectiveImageHeight + skipRows + r;
            }
            if (srcRow >= srcHeight) {
                continue;
            }
            size_t srcRowOffset = srcRow * srcStride;
            size_t dstRowOffset = dstSliceOffset + r * dstRowStride;

            for (size_t c = 0; c < destWidth; c++) {
                size_t srcCol = skipPixels + c;
                if (srcCol >= srcWidth) {
                    continue;
                }
                const unsigned char* src = image + srcRowOffset + srcCol * 4;
                const uint8_t rgba[4] = { src[order[0]], src[order[1]],
                                          src[order[2]], src[order[3]] };
                writePixel(m_data.get() + dstRowOffset + c * dstBytesPerPixel,
                           rgba, channels, count, type, needsPremultiplyAlpha);
            }
        }
    }
    return GL_NO_ERROR;
}

const void* TexImageHelper::data() const
{
    return m_isConverted ? m_data.get() : m_sourceImage.data;
}

Optional<GLenum> TexImageHelper::dataFormat() const
{
    return m_dataFormat;
}

unsigned char TexImageHelper::multiplyAlpha(unsigned char color, float alpha)
{
    return ((color / 255.f) * alpha) * 255;
}

void TexImageHelper::writePixel(unsigned char* dest, const uint8_t* rgba,
                                const uint8_t* channels, size_t count,
                                GLenum type, bool needsPremultiplyAlpha)
{
    const float alpha = needsPremultiplyAlpha ? rgba[3] / 255.f : 1.f;
    // Channel c normalized, or as an 8-bit value, with the color channels
    // premultiplied when asked.
    auto normalized = [&](uint8_t c) {
        return c == 3 ? rgba[3] / 255.f : rgba[c] / 255.f * alpha;
    };
    auto byte = [&](uint8_t c) -> uint8_t {
        return (needsPremultiplyAlpha && c != 3) ? multiplyAlpha(rgba[c], alpha)
                                                 : rgba[c];
    };

    switch (type) {
    case GL_UNSIGNED_BYTE:
        for (size_t i = 0; i < count; i++) {
            dest[i] = byte(channels[i]);
        }
        return;
    case GL_HALF_FLOAT:
    case GL_HALF_FLOAT_OES:
        for (size_t i = 0; i < count; i++) {
            GLushort value = Pixel::makeHalfFloat(normalized(channels[i]));
            memcpy(dest + i * sizeof(value), &value, sizeof(value));
        }
        return;
    case GL_FLOAT:
        for (size_t i = 0; i < count; i++) {
            float value = normalized(channels[i]);
            memcpy(dest + i * sizeof(value), &value, sizeof(value));
        }
        return;
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1: {
        GLushort packed =
            type == GL_UNSIGNED_SHORT_5_6_5
                ? Pixel::makePixel565(byte(0), byte(1), byte(2))
            : type == GL_UNSIGNED_SHORT_4_4_4_4
                ? Pixel::makePixel4444(byte(0), byte(1), byte(2), byte(3))
                : Pixel::makePixel5551(byte(0), byte(1), byte(2), byte(3));
        memcpy(dest, &packed, sizeof(packed));
        return;
    }
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV: {
        uint32_t packed =
            type == GL_UNSIGNED_INT_10F_11F_11F_REV
                ? Pixel::makePixel10F11F11F(normalized(0), normalized(1),
                                            normalized(2))
                : Pixel::makePixel2101010(normalized(0), normalized(1),
                                          normalized(2), normalized(3));
        memcpy(dest, &packed, sizeof(packed));
        return;
    }
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }
}

// The alpha of the pixel, if its format has one, is multiplied into its color
// channels in place.
void TexImageHelper::premultiplyPixel(unsigned char* pixel, GLenum type)
{
    const GLenum format = m_sourceImage.format;
    const bool hasAlpha = format == GL_RGBA || format == GL_LUMINANCE_ALPHA;
    const size_t colors = format == GL_RGBA ? 3 : 1;
    switch (type) {
    case GL_UNSIGNED_BYTE:
        if (hasAlpha) {
            float alpha = pixel[colors] / 255.f;
            for (size_t i = 0; i < colors; i++) {
                pixel[i] = multiplyAlpha(pixel[i], alpha);
            }
        }
        return;
    case GL_FLOAT:
        if (hasAlpha) {
            float values[4];
            memcpy(values, pixel, (colors + 1) * sizeof(float));
            for (size_t i = 0; i < colors; i++) {
                values[i] *= values[colors];
            }
            memcpy(pixel, values, (colors + 1) * sizeof(float));
        }
        return;
    case GL_UNSIGNED_SHORT_4_4_4_4: {
        GLushort value;
        memcpy(&value, pixel, sizeof(value));
        uint8_t channel[4];
        for (size_t i = 0; i < 4; i++) {
            channel[i] = ((value >> (12 - 4 * i)) & 0xF) * 17;
        }
        float alpha = channel[3] / 255.f;
        value = Pixel::makePixel4444(
            multiplyAlpha(channel[0], alpha), multiplyAlpha(channel[1], alpha),
            multiplyAlpha(channel[2], alpha), channel[3]);
        memcpy(pixel, &value, sizeof(value));
        return;
    }
    case GL_UNSIGNED_SHORT_5_5_5_1: {
        // A one-bit alpha keeps the color or clears it.
        GLushort value;
        memcpy(&value, pixel, sizeof(value));
        if (!(value & 1)) {
            value = 0;
        }
        memcpy(pixel, &value, sizeof(value));
        return;
    }
    default:
        // shortcut: half-float, packed 32-bit and integer data is copied
        // without premultiplying; add it when a test exercises the case.
        return;
    }
}

// The buffers the engine builds itself (the zeroes for a null source, a
// flipped, premultiplied or converted copy) are tightly packed client memory,
// so the GL must read them that way whatever unpack state the page set. Every
// user has already refused an upload while a PIXEL_UNPACK_BUFFER is bound.
// WebGL 1 has only the first parameter.
static const struct {
    GLenum name;
    GLint value;
} s_tight[] = {
    { GL_UNPACK_ALIGNMENT, 1 },    { GL_UNPACK_ROW_LENGTH, 0 },
    { GL_UNPACK_IMAGE_HEIGHT, 0 }, { GL_UNPACK_SKIP_PIXELS, 0 },
    { GL_UNPACK_SKIP_ROWS, 0 },    { GL_UNPACK_SKIP_IMAGES, 0 },
};

class ScopedTightUnpackState {
public:
    ScopedTightUnpackState(GL* gl, bool isWebGL2)
        : m_gl(gl)
        , m_count(isWebGL2 ? 6 : 1)
    {
        for (size_t i = 0; i < m_count; i++) {
            m_gl->getIntegerv(s_tight[i].name, &m_saved[i]);
            if (m_saved[i] != s_tight[i].value) {
                m_gl->pixelStorei(s_tight[i].name, s_tight[i].value);
            }
        }
    }

    ~ScopedTightUnpackState()
    {
        for (size_t i = 0; i < m_count; i++) {
            if (m_saved[i] != s_tight[i].value) {
                m_gl->pixelStorei(s_tight[i].name, m_saved[i]);
            }
        }
    }

private:
    GL* m_gl;
    size_t m_count;
    GLint m_saved[6];
};

bool WebGLRenderingContext::isSrcDataValid(ScriptArrayBufferView srcData,
                                           GLenum type)
{
    if (type == GL_UNSIGNED_BYTE && (!srcData->isUint8ArrayObject() &&
                                     !srcData->isUint8ClampedArrayObject())) {
        // If it is UNSIGNED_BYTE, a Uint8Array or Uint8ClampedArray
        // must be supplied.
        return false;
    }
    if ((type == GL_UNSIGNED_SHORT_5_6_5 || type == GL_UNSIGNED_SHORT_4_4_4_4 ||
         type == GL_UNSIGNED_SHORT_5_5_5_1) &&
        !srcData->isUint16ArrayObject()) {
        // If it is UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, or
        // UNSIGNED_SHORT_5_5_5_1, a Uint16Array must be supplied.
        return false;
    }
    // OES_texture_float / OES_texture_half_float: FLOAT data comes in a
    // Float32Array, HALF_FLOAT_OES data in a Uint16Array. Without the
    // extension the type is the INVALID_ENUM the caller reports.
    if (type == GL_FLOAT && !srcData->isFloat32ArrayObject() &&
        isExtensionEnabled("OES_texture_float")) {
        return false;
    }
    if (type == GL_HALF_FLOAT_OES && !srcData->isUint16ArrayObject() &&
        isExtensionEnabled("OES_texture_half_float")) {
        return false;
    }
    return true;
}

bool WebGLRenderingContext::checkUnpackBufferBinding(bool fromPixelUnpackBuffer)
{
    bool hasUnpackBuffer = !!getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER);
    if (hasUnpackBuffer != fromPixelUnpackBuffer) {
        setGLError(GL_INVALID_OPERATION,
                   hasUnpackBuffer ? "PIXEL_UNPACK_BUFFER is bound."
                                   : "PIXEL_UNPACK_BUFFER is not bound.");
        return false;
    }
    return true;
}

// Unknown tokens are an INVALID_ENUM, a pair of known ones that does not go
// together an INVALID_OPERATION (OpenGL ES 3.0.6 §3.8.3).
bool WebGLRenderingContext::isKnownFormatAndType(GLenum format, GLenum type)
{
    if (isWebGL2RenderingContext()) {
        return Pixel::isKnownFormatWebGL2(format) &&
               Pixel::isKnownTypeWebGL2(type);
    }
    // FLOAT and HALF_FLOAT_OES are accepted tokens only once their OES
    // extension is enabled; otherwise they are an INVALID_ENUM like any
    // unknown token (OES_texture_float / OES_texture_half_float).
    bool isKnownType =
        type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT_5_6_5 ||
        type == GL_UNSIGNED_SHORT_4_4_4_4 ||
        type == GL_UNSIGNED_SHORT_5_5_5_1 ||
        (type == GL_FLOAT && isExtensionEnabled("OES_texture_float")) ||
        (type == GL_HALF_FLOAT_OES &&
         isExtensionEnabled("OES_texture_half_float"));
    return (format == GL_ALPHA || format == GL_RGB || format == GL_RGBA ||
            format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) &&
           isKnownType;
}

// What every TexImageSource upload checks before it reads its source: known
// format and type tokens, and no bound PIXEL_UNPACK_BUFFER (WebGL 2.0
// §3.7.6).
bool WebGLRenderingContext::validateTexImageSource(GLenum format, GLenum type)
{
    if (!isKnownFormatAndType(format, type)) {
        setGLError(GL_INVALID_ENUM, "format or type is not an accepted token");
        return false;
    }
    return checkUnpackBufferBinding(false);
}

// A null source is a buffer of zeros (WebGL 1.0 §5.14.8), as are the texels
// copyTexImage2D reads from outside the framebuffer. They go up a bounded
// chunk of rows at a time, so no buffer of the level's size is made.
void WebGLRenderingContext::zeroTextureLevel(GLenum target, GLint level,
                                             GLsizei width, GLsizei height,
                                             GLsizei depth, GLenum format,
                                             GLenum type)
{
    const size_t rowBytes =
        static_cast<size_t>(width) * getBytesPerPixel(format, type);
    if (!rowBytes || height <= 0 || depth <= 0) {
        return;
    }
    // As bufferData fills a buffer.
    const size_t rowsPerChunk = std::max<size_t>(1, (64 * 1024) / rowBytes);
    const std::vector<uint8_t> zeros(std::min<size_t>(rowsPerChunk, height) *
                                     rowBytes);
    ScopedTightUnpackState unpackState(m_gl, isWebGL2RenderingContext());
    // copyTexImage2D may run with a PIXEL_UNPACK_BUFFER bound.
    GLint unpackBuffer = 0;
    if (isWebGL2RenderingContext()) {
        m_gl->getIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        if (unpackBuffer) {
            m_gl->bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
    }
    const bool is3D = target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY;
    for (GLsizei z = 0; z < depth; z++) {
        for (GLsizei y = 0; y < height;) {
            const GLsizei rows = static_cast<GLsizei>(
                std::min<size_t>(rowsPerChunk, height - y));
            if (is3D) {
                m_gl->texSubImage3D(target, level, 0, y, z, width, rows, 1,
                                    format, type, zeros.data());
            } else {
                m_gl->texSubImage2D(target, level, 0, y, width, rows, format,
                                    type, zeros.data());
            }
            y += rows;
        }
    }
    if (unpackBuffer) {
        m_gl->bindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
    }
}

void WebGLRenderingContext::handleTexImageWithArrayBufferView(
    GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth,
    GLenum format, GLenum type, Optional<ScriptArrayBufferView> pixels,
    unsigned long long srcOffset,
    std::function<void(const TexImageHelper*)> updateImage,
    std::function<void()> allocateImage)
{
    STARFISH_ASSERT(updateImage != nullptr);

    // A null source is no exception (WebGL 2.0 §3.7.6).
    if (!checkUnpackBufferBinding(false)) {
        return;
    }

    if (pixels.hasValue()) {
        ArrayBufferViewRef* pixelsView = pixels.getValue();

        // WebGL 2.0, "Pixel store parameter constraints": a 3D upload from
        // client memory is neither flipped nor premultiplied.
        if ((target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY) &&
            (m_unpackFlipY || m_unpackPremultiplyAlpha)) {
            setGLError(GL_INVALID_OPERATION,
                       "UNPACK_FLIP_Y_WEBGL or UNPACK_PREMULTIPLY_ALPHA_WEBGL "
                       "is set for a 3D upload.");
            return;
        }

        if (!isSrcDataValid(pixelsView, type)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (webGLVersion() == 1 && (type == GL_FLOAT) &&
            !isExtensionEnabled("OES_texture_float")) {
            setGLError(GL_INVALID_ENUM);
            return;
        }

        size_t elementSize = viewElementSize(pixelsView);

        if (srcOffset > pixelsView->byteLength() / elementSize) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        size_t byteOffset = srcOffset * elementSize;

        size_t bytesPerPixel = getBytesPerPixel(format, type);
        size_t byteLengthOfPixels = 0;
        size_t stride = 0;
        if (!computeRequiredUnpackBytes(target, width, height, depth, format,
                                        type, byteLengthOfPixels, stride)) {
            return;
        }
        size_t availableBytes = pixelsView->byteLength() - byteOffset;

        if (availableBytes < byteLengthOfPixels) {
            setGLError(GL_INVALID_OPERATION, "buffer too small");
            return;
        }

        uint8_t* data = pixelsView->rawBuffer() + byteOffset;

        if (!m_unpackFlipY && !m_unpackPremultiplyAlpha) {
            // The GL reads the view in place with the page's unpack state,
            // which computeRequiredUnpackBytes() checked it against.
            TexImageHelper image(width, height * depth, width * bytesPerPixel,
                                 format, data);
            updateImage(&image);
            return;
        }

        // A flipped or premultiplied copy reads the view with the page's
        // unpack layout (OpenGL ES 3.0.6 §3.8.2). Only 2D uploads get here,
        // so the rows it copies end exactly at byteLengthOfPixels.
        STARFISH_ASSERT(depth == 1);
        const size_t skipBytes =
            byteLengthOfPixels
                ? byteLengthOfPixels -
                      (static_cast<size_t>(height) - 1) * stride -
                      static_cast<size_t>(width) * bytesPerPixel
                : 0;
        TexImageHelper image(width, height, stride, format, data + skipBytes);
        GLenum error = image.draw(m_unpackFlipY, m_unpackPremultiplyAlpha, type,
                                  bytesPerPixel);
        if (error != GL_NO_ERROR) {
            setGLError(error);
            return;
        }

        ScopedTightUnpackState unpackState(m_gl, isWebGL2RenderingContext());
        updateImage(&image);
    } else {
        if (width < 0 || height < 0 || depth < 0) {
            setGLError(GL_INVALID_VALUE, "negative dimension");
            return;
        }
        // The caller zeroes the level once the GL has defined it.
        STARFISH_ASSERT(allocateImage != nullptr);
        allocateImage();
    }
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
static bool isVideoOriginClean(HTMLVideoElement* video);
static bool keepsVideoFrameInMemory(HTMLVideoElement* video)
{
    MediaPlayer* player = video->activeMediaPlayer();
    return player && player->keepsVideoFramesInMemory();
}
#endif

// https://registry.khronos.org/webgl/specs/latest/1.0/#ORIGIN_RESTRICTIONS
// Uploading a source that is not origin-clean would make cross-origin pixels
// readable through the texture, so it throws unless the embedder turned web
// security off.
static void throwIfNotOriginClean(ExecutionContext* context, bool originClean)
{
    if (originClean || context->webBase()->getWebSecurityMode() ==
                           LWE::WebSecurityMode::Disable) {
        return;
    }
    throw new DOMException(context, DOMException::Code::SECURITY_ERR,
                           "The source contains cross-origin data.");
}

void WebGLRenderingContext::throwConversionError()
{
    throw new DOMException(executionContext(),
                           DOMException::Code::SCRIPT_TYPE_ERR,
                           "The value matches no type of the argument");
}

void WebGLRenderingContext::handleTexImageWithImageSource(
    const GLenum target, const GLenum format, const GLenum type,
    const TexImageSource& source,
    std::function<void(const TexImageHelper*)> updateImage, size_t skipPixels,
    size_t skipRows, GLsizei destWidth, GLsizei destHeight, GLsizei depth,
    size_t imageHeight)
{
    STARFISH_ASSERT(updateImage != nullptr);

    if (!validateTexImageSource(format, type)) {
        return;
    }

    if (destWidth < 0 || destHeight < 0 || depth < 0) {
        setGLError(GL_INVALID_VALUE, "negative size");
        return;
    }
    // A sized upload with no texels converts nothing.
    const bool hasSubRectangle = destWidth > 0 || destHeight > 0;
    if (hasSubRectangle) {
        // The size sets the size of the conversion buffer, so the GL's own
        // limits (OpenGL ES 3.0.6 §3.8.3) are applied before it is built.
        GLint maxSize = 0;
        GLint maxDepth = 1;
        if (target == GL_TEXTURE_3D) {
            m_gl->getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxSize);
            maxDepth = maxSize;
        } else if (target == GL_TEXTURE_2D_ARRAY) {
            m_gl->getIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
            m_gl->getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxDepth);
        } else {
            m_gl->getIntegerv(target == GL_TEXTURE_2D
                                  ? GL_MAX_TEXTURE_SIZE
                                  : GL_MAX_CUBE_MAP_TEXTURE_SIZE,
                              &maxSize);
        }
        if (destWidth > maxSize || destHeight > maxSize || depth > maxDepth) {
            setGLError(GL_INVALID_VALUE, "size exceeds the texture limit");
            return;
        }
    }

    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei stride = 0;
    NativeImageData* imageData = nullptr;
    NativeImageData* rasterizedSVGImage = nullptr;
    ::Starfish::ImageData* sourceImageData = nullptr;
    std::vector<uint8_t> canvasPixels;
    bool flipY = m_unpackFlipY;
    bool premultiplyAlpha = m_unpackPremultiplyAlpha;

    if (source.isNoneValue()) {
        setGLError(GL_INVALID_VALUE);
        return;
    } else if (source.isImageBitmapValue()) {
        TRACE(WEBGL, "source.isImageBitmapValue");
        ImageBitmap* imageBitmap = source.getImageBitmapValue();
        // WebGL 1.0 §5.14.8: a closed (neutered) bitmap is an INVALID_VALUE.
        if (imageBitmap->isDetached()) {
            setGLError(GL_INVALID_VALUE, "The ImageBitmap is closed.");
            return;
        }
        throwIfNotOriginClean(executionContext(),
                              imageBitmap->originCleanFlag());
        imageData = imageBitmap->nativeImageData();
        width = imageData->width();
        height = imageData->height();
        // An ImageBitmap uploads as it was created; the unpack flags do not
        // apply to it (WebGL 1.0, "Pixel Storage Parameters").
        flipY = false;
        premultiplyAlpha = false;
    } else if (source.isImageDataValue()) {
        sourceImageData = source.getImageDataValue();
        ScriptUint8ClampedArray pixels = sourceImageData->data();
        if (pixels->buffer()->isArrayBufferObject() &&
            pixels->buffer()->asArrayBufferObject()->isDetachedBuffer()) {
            setGLError(GL_INVALID_VALUE, "ImageData buffer is detached");
            return;
        }
        width = sourceImageData->width();
        height = sourceImageData->height();
        if (height && size_t(width) > (pixels->byteLength() / 4) / height) {
            setGLError(GL_INVALID_VALUE, "ImageData buffer is too small");
            return;
        }
    } else if (source.isHTMLImageElementValue()) {
        TRACE(WEBGL, "source.isHTMLImageElementValue");
        HTMLImageElement* element = source.getHTMLImageElementValue();
        imageData = element->imageData();
        if (imageData == nullptr) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        throwIfNotOriginClean(executionContext(),
                              element->isImageCORSSameOrigin());
        if (imageData->isSVGNativeImageData()) {
            width = imageData->width();
            height = imageData->height();
            if (width == 0 || height == 0) {
                width = element->width();
                height = element->height();
            }
            if (width > 0 && height > 0) {
                rasterizedSVGImage =
                    imageData->asSVGNativeImageData()->rasterize();
                imageData = rasterizedSVGImage;
                width = imageData->width();
                height = imageData->height();
            }
        } else {
            width = imageData->width();
            height = imageData->height();
        }
    } else if (source.isHTMLCanvasElementValue()) {
        TRACE(WEBGL, "source.isHTMLCanvasElementValue");
        // TODO: Consider using CanvasImageSourceUtils::toNativeImageData
        HTMLCanvasElement* element = source.getHTMLCanvasElementValue();
        width = element->width();
        height = element->height();
        CanvasRenderingContext* context = element->canvasRenderingContext();
        throwIfNotOriginClean(executionContext(),
                              !context || context->originCleanFlag());
        if (!context) {
            // A canvas that has no rendering context yet still has a bitmap
            // of its size, initialized to transparent black (HTML, "The canvas
            // element"). A 0x0 canvas still gets one byte so the upload has a
            // valid buffer to point at.
            canvasPixels.resize(
                std::max<size_t>(1, size_t(width) * height * 4));
        } else if (context->isCanvasRenderingContext2D()) {
            CanvasRenderingContext2DMixIn* context2d =
                context->asCanvasRenderingContext2D();
            context2d->flushForReadback();
            imageData = NativeImageData::attach(context2d->canvas());
        } else if (context->isWebGLRenderingContext() ||
                   context->isWebGL2RenderingContext()) {
            // A WebGL canvas uploads its drawing buffer (WebGL 1.0 §5.14.8).
            WebGLRenderingContext* sourceContext =
                static_cast<WebGLRenderingContext*>(context);
            // A context whose document is gone has no drawing buffer left.
            if (!sourceContext->m_canvasSurface) {
                return;
            }
            // shortcut: the drawing buffer size stands for the canvas size;
            // they differ only for a clamped or 0x0 canvas.
            width = sourceContext->drawingBufferWidth();
            height = sourceContext->drawingBufferHeight();
            canvasPixels.resize(size_t(width) * height * 4);
            // This context's scope is open, so the source's context is made
            // current by hand rather than by a nested scope (WEBGL.md §5.1),
            // and this one again after.
            if (!sourceContext->readClearedDrawingBuffer(canvasPixels.data(),
                                                         size_t(width) * 4)) {
                if (sourceContext->m_context.setCurrent()) {
                    sourceContext->readDrawingBuffer(canvasPixels.data(),
                                                     size_t(width) * 4);
                }
                if (!m_context.setCurrent()) {
                    return;
                }
            }
            // A premultipliedAlpha drawing buffer holds premultiplied colors;
            // they go back to straight ones unless the upload premultiplies
            // (WebGL 1.0, "Pixel Storage Parameters").
            const bool isPremultiplied =
                sourceContext->m_attributes.alpha() &&
                sourceContext->m_attributes.premultipliedAlpha();
            if (isPremultiplied && !premultiplyAlpha) {
                convertImageBufferAsUnmultipliedAlphaIfNeeds(
                    canvasPixels.data(), width, size_t(width) * 4, height);
            }
            premultiplyAlpha = premultiplyAlpha && !isPremultiplied;
        } else {
            // A bitmaprenderer canvas cannot be read back yet; nothing is
            // uploaded.
            STARFISH_UNIMPLEMENTED("texImage from a bitmaprenderer canvas");
            return;
        }
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (source.isHTMLVideoElementValue()) {
        HTMLVideoElement* video = source.getHTMLVideoElementValue();
        MediaPlayer* player = video->activeMediaPlayer();
        // No media data yet (its origin is only known from HAVE_METADATA on),
        // or a srcObject stream (empty currentSrc): nothing to upload.
        if (!player || video->currentSrc()->isEmpty() ||
            video->readyState() < HTMLMediaElement::HAVE_METADATA) {
            return;
        }
        if (!isVideoOriginClean(video)) {
            throw new DOMException(
                video->document()->executionContext(),
                DOMException::Code::SECURITY_ERR,
                "The video element contains cross-origin data.");
        }
        // The upload is the size of the frame (WebGL 1.0 "Texture Upload
        // Width and Height"). A player without its frame in memory uploads
        // nothing here; the WebGL 1 overloads copy its frame on the GPU.
        size_t frameWidth = 0;
        size_t frameHeight = 0;
        if (!player->copyCurrentVideoFrame(canvasPixels, frameWidth,
                                           frameHeight)) {
            return;
        }
        width = static_cast<GLsizei>(frameWidth);
        height = static_cast<GLsizei>(frameHeight);
    }
#endif
    else {
        STARFISH_ASSERT_NOT_REACHED();
        return;
    }

    if (imageData == nullptr && sourceImageData == nullptr &&
        canvasPixels.empty()) {
        STARFISH_ASSERT_NOT_REACHED();
        return;
    }

    stride = width * 4;

    TexImageHelper image =
        sourceImageData ? TexImageHelper(sourceImageData, format)
        : imageData     ? TexImageHelper(imageData, format)
                        : TexImageHelper(width, height, stride, format,
                                         canvasPixels.data());
    GLenum error = image.drawSubRectangle(
        flipY, premultiplyAlpha, type, hasSubRectangle ? skipPixels : 0,
        hasSubRectangle ? skipRows : 0, hasSubRectangle ? destWidth : width,
        hasSubRectangle ? destHeight : height, hasSubRectangle ? depth : 1,
        imageHeight);

    TRACE(WEBGL_V, "source:", KV(width), KV(height), KV(stride), KV(imageData),
          KV(error));

    if (error == GL_NO_ERROR) {
        ScopedTightUnpackState unpackState(m_gl, isWebGL2RenderingContext());
        updateImage(&image);
    } else {
        setGLError(error);
    }
    if (rasterizedSVGImage != nullptr) {
        delete rasterizedSVGImage;
    }
}

bool WebGLRenderingContext::checkInternalFormat(GLint internalFormat,
                                                GLenum format, GLenum type)
{
    const bool depthTextures = isExtensionEnabled("WEBGL_depth_texture");
    if (internalFormat != GL_ALPHA && internalFormat != GL_RGB &&
        internalFormat != GL_RGBA && internalFormat != GL_LUMINANCE &&
        internalFormat != GL_LUMINANCE_ALPHA &&
        !(depthTextures && isDepthTextureFormat(internalFormat))) {
        setGLError(GL_INVALID_ENUM, "internalFormat is not an accepted token");
        return false;
    }
    if (format != GL_ALPHA && format != GL_RGB && format != GL_RGBA &&
        format != GL_LUMINANCE && format != GL_LUMINANCE_ALPHA &&
        !(depthTextures && isDepthTextureFormat(format))) {
        setGLError(GL_INVALID_ENUM, "format is not an accepted token");
        return false;
    }
    // WEBGL_depth_texture "Errors": its formats and types go only together.
    if (depthTextures &&
        (isDepthTextureFormat(format) || type == GL_UNSIGNED_SHORT ||
         type == GL_UNSIGNED_INT || type == GL_UNSIGNED_INT_24_8)) {
        const bool paired =
            static_cast<GLenum>(internalFormat) == format &&
            (format == GL_DEPTH_COMPONENT
                 ? type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT
                 : format == GL_DEPTH_STENCIL && type == GL_UNSIGNED_INT_24_8);
        if (!paired) {
            setGLError(GL_INVALID_OPERATION,
                       "WEBGL_depth_texture format and type do not match");
        }
        return paired;
    }
    // OpenGL ES 2.0 §3.7.1: an unknown type token is INVALID_ENUM, before the
    // format and type are matched.
    if (!isKnownFormatAndType(format, type)) {
        setGLError(GL_INVALID_ENUM, "type is not an accepted token");
        return false;
    }

    if (!Pixel::isInternalFormatValid(internalFormat, format, type, 1)) {
        // The format, in WebGL 1, must be the same as internalFormat. See:
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/texImage2D
        // WebGL 2 checks combination of internalFormat, format, and type.
        setGLError(GL_INVALID_OPERATION,
                   StringUtils::formatString(
                       "The given parameters, internal format (0x%04X) and "
                       "format (0x%04X), are not the same.",
                       internalFormat, format)
                       .c_str());
        return false;
    }
    return true;
}

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLsizei width,
                                       GLsizei height, GLint border,
                                       GLenum format, GLenum type,
                                       Optional<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                                    target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) {
        setGLError(GL_INVALID_ENUM, "Invalid texture target");
        return;
    }

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0");
        return;
    }

    if (!isWebGL2RenderingContext() && level > 0 &&
        (!isPowerOfTwo(static_cast<size_t>(width)) ||
         !isPowerOfTwo(static_cast<size_t>(height)))) {
        setGLError(GL_INVALID_VALUE,
                   "level > 0 requires power of two dimensions in WebGL 1.0");
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }
    if (texture) {
        texture->setSize(width, height);
    }

    if (m_boundTextures.find(target) == m_boundTextures.end() &&
        !isBoundCubeMapTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!checkInternalFormat(internalFormat, format, type)) {
        return;
    }

    // WEBGL_depth_texture: a depth texture is a 2D level 0 with no data.
    const bool isDepthTexture =
        webGLVersion() == 1 && isDepthTextureFormat(format);
    if (isDepthTexture &&
        (target != GL_TEXTURE_2D || level != 0 || pixels.hasValue())) {
        setGLError(GL_INVALID_OPERATION,
                   "a depth texture is level 0 of a TEXTURE_2D with no data");
        return;
    }

    bool allocated = false;
    handleTexImageWithArrayBufferView(
        target, level, width, height, 1, format, type, pixels, 0,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            m_gl->texImage2D(target, level, internalFormat, width, height, 0,
                             format, type, helper->data());
        },
        [&]() {
#if defined(PORT_PIXEL_ORDER_BGRA)
            if (webGLVersion() == 1 && format == GL_RGBA &&
                type == GL_UNSIGNED_BYTE) {
                if (WebGLExtensionRegistry::instance()
                        .hasEXT_texture_format_BGRA8888()) {
                    // According to OpenGL ES specification, the format must
                    // match the base internal format (no conversions from
                    // one format to another during texture image processing
                    // are supported.)
                    internalFormat = GL_BGRA_EXT;
                    format = GL_BGRA_EXT;
                }
            }
#endif
            m_gl->texImage2D(target, level, internalFormat, width, height, 0,
                             format, type, nullptr);
            allocated = true;
        });

    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalFormat, width, height,
                                   1, type);
        if (isDepthTexture) {
            initializeImage(format, [&](GLenum attachment) {
                m_gl->framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment,
                                           GL_TEXTURE_2D, texture->glObject(),
                                           0);
            });
        } else if (allocated) {
            zeroTextureLevel(target, level, width, height, 1, format, type);
        }
    }
}

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLenum format,
                                       GLenum type, TexImageSource source)
{
    throwIfNotConverted(source);
    ENTER_CONTEXT_SCOPE();

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }

    if (m_boundTextures.find(target) == m_boundTextures.end() &&
        !isBoundCubeMapTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!checkInternalFormat(internalFormat, format, type)) {
        return;
    }
    // WEBGL_depth_texture: a depth texture takes no data.
    if (webGLVersion() == 1 && isDepthTextureFormat(format)) {
        setGLError(GL_INVALID_OPERATION, "a depth texture takes no data");
        return;
    }

#ifdef STARFISH_ENABLE_MULTIMEDIA
    // A frame the player keeps in memory converts like any other source
    // below; any other player's frame is copied on the GPU.
    if (source.isHTMLVideoElementValue() &&
        !keepsVideoFrameInMemory(source.getHTMLVideoElementValue())) {
        if (validateTexImageSource(format, type)) {
            texImageVideo(target, level, internalFormat, 0, 0, format, type,
                          source.getHTMLVideoElementValue());
        }
        return;
    }
#endif

    handleTexImageWithImageSource(
        target, format, type, source, [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);

            TRACE(WEBGL_V, KV(glValueString(internalFormat)),
                  KV(glValueString(
                      helper->dataFormat().valueOr(internalFormat))));
            TRACE(WEBGL_V, KV(glValueString(format)),
                  KV(glValueString(helper->dataFormat().valueOr(format))));
            TRACE(WEBGL_V, KV(glValueString(type)));

            // OpenGL ES 2.0 §3.7.1, as in the other texImage2D(): the source
            // gives the size.
            if (webGLVersion() == 1 && level > 0 &&
                !(isPowerOfTwo(helper->width()) &&
                  isPowerOfTwo(helper->height()))) {
                setGLError(GL_INVALID_VALUE,
                           "level > 0 of a non-power-of-two size");
                return;
            }

            // Uploads the given image data to the currently bound texture.
            m_gl->texImage2D(
                target, level, helper->dataFormat().valueOr(internalFormat),
                helper->width(), helper->height(), 0,
                helper->dataFormat().valueOr(format), type, helper->data());
            if (texture && !hasNewGLError()) {
                texture->setImageLevelInfo(
                    target, level, helper->dataFormat().valueOr(internalFormat),
                    helper->width(), helper->height(), 1, type);
            }
        });
}

// WEBGL_depth_texture: a depth texture takes no data.
bool WebGLRenderingContext::validateTexSubImageFormat(GLenum format)
{
    if (webGLVersion() == 1 && isDepthTextureFormat(format) &&
        isExtensionEnabled("WEBGL_depth_texture")) {
        setGLError(GL_INVALID_OPERATION, "a depth texture takes no data");
        return false;
    }
    return true;
}

// WebGL 1.0 "Texture Type in TexSubImage2D Calls": the type must be the one
// that defined the level. Unknown tokens are left to the INVALID_ENUM
// checks, and a level whose type is not recorded (copyTexImage2D, and the
// levels generateMipmap makes from it) to the GL.
bool WebGLRenderingContext::isTexSubImageTypeAllowed(GLenum target, GLint level,
                                                     GLenum format, GLenum type)
{
    if (webGLVersion() >= 2 || !isKnownFormatAndType(format, type)) {
        return true;
    }
    WebGLTexture* texture = getBoundTextureObject(target);
    const TextureImageLevelInfo* info =
        texture ? texture->getImageLevelInfo(target, level) : nullptr;
    if (info && info->type && info->type != type) {
        setGLError(GL_INVALID_OPERATION, "type differs from the level's type");
        return false;
    }
    return true;
}

void WebGLRenderingContext::texSubImage2D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
    GLsizei height, GLenum format, GLenum type,
    Optional<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                                    target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) {
        setGLError(GL_INVALID_ENUM, "Invalid texture target");
        return;
    }

    if (format == 0x80E0 /* GL_BGR */ || format == 0x80E1 /* GL_BGRA */ ||
        type == 0x8032 /* GL_UNSIGNED_BYTE_3_3_2 */) {
        setGLError(GL_INVALID_ENUM, "Invalid format or type");
        return;
    }

    // WebGL 1.0 §5.14.8: unlike texImage2D, a null source is an error.
    if (!pixels.hasValue()) {
        setGLError(GL_INVALID_VALUE, "pixels is null.");
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (!texture || (m_boundTextures.find(target) == m_boundTextures.end() &&
                     !isBoundCubeMapTexture(target))) {
        setGLError(GL_INVALID_OPERATION, "No texture bound to target");
        return;
    }

    if (!validateTexSubImageFormat(format)) {
        return;
    }
    if (!isTexSubImageTypeAllowed(target, level, format, type)) {
        return;
    }

    handleTexImageWithArrayBufferView(
        target, level, width, height, 1, format, type, pixels, 0,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            m_gl->texSubImage2D(target, level, xoffset, yoffset, width, height,
                                format, type, helper->data());
        });
}

void WebGLRenderingContext::texSubImage2D(GLenum target, GLint level,
                                          GLint xoffset, GLint yoffset,
                                          GLenum format, GLenum type,
                                          TexImageSource source)
{
    throwIfNotConverted(source);
    ENTER_CONTEXT_SCOPE();

    if (!validateTexSubImageFormat(format)) {
        return;
    }
    if (!isTexSubImageTypeAllowed(target, level, format, type)) {
        return;
    }

#ifdef STARFISH_ENABLE_MULTIMEDIA
    // As in texImage2D(): a frame kept in memory converts on the CPU.
    if (source.isHTMLVideoElementValue() &&
        !keepsVideoFrameInMemory(source.getHTMLVideoElementValue())) {
        if (validateTexImageSource(format, type)) {
            texImageVideo(target, level, Optional<GLint>(), xoffset, yoffset,
                          format, type, source.getHTMLVideoElementValue());
        }
        return;
    }
#endif

    handleTexImageWithImageSource(
        target, format, type, source, [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);

            TRACE(WEBGL_V, KV(glValueString(format)),
                  KV(glValueString(helper->dataFormat().valueOr(format))));
            TRACE(WEBGL_V, KV(glValueString(type)));

            // Uploads the given image data to the currently bound texture.
            m_gl->texSubImage2D(target, level, xoffset, yoffset,
                                helper->width(), helper->height(),
                                helper->dataFormat().valueOr(format), type,
                                helper->data());
        });
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
// https://registry.khronos.org/webgl/specs/latest/1.0/#ORIGIN_RESTRICTIONS
// A video whose media data is not CORS-same-origin would leak its pixels
// through the texture, so it is rejected unless the embedder turned web
// security off.
static bool isVideoOriginClean(HTMLVideoElement* video)
{
    ExecutionContext* context = video->document()->executionContext();
    if (context->webBase()->getWebSecurityMode() ==
        LWE::WebSecurityMode::Disable) {
        return true;
    }
    return video->mediaDataIsCORSSameOrigin();
}

static GLuint createVideoCopyProgram(GL* gl, bool externalSource)
{
    const char* vertexSource =
        "attribute vec2 aPosition;\n"
        "uniform bool uFlipY;\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "  vTexCoord = vec2(aPosition.x,\n"
        "                   uFlipY ? 1.0 - aPosition.y : aPosition.y);\n"
        "  gl_Position = vec4(aPosition * 2.0 - 1.0, 0.0, 1.0);\n"
        "}\n";
    // highp where available: mediump texture coordinates are too coarse to
    // address every texel of a 4K frame.
    std::string fragmentSource =
        externalSource ? "#extension GL_OES_EGL_image_external : require\n"
                       : "";
    fragmentSource +=
        "#ifdef GL_ES\n"
        "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
        "precision highp float;\n"
        "#else\n"
        "precision mediump float;\n"
        "#endif\n"
        "#endif\n"
        "varying vec2 vTexCoord;\n";
    fragmentSource += externalSource ? "uniform samplerExternalOES uTexture;\n"
                                     : "uniform sampler2D uTexture;\n";
    fragmentSource +=
        "void main() {\n"
        "  gl_FragColor = texture2D(uTexture, vTexCoord);\n"
        "}\n";

    auto compile = [gl](GLenum shaderType, const char* source) {
        GLuint shader = gl->createShader(shaderType);
        gl->shaderSource(shader, 1, &source, nullptr);
        gl->compileShader(shader);
        return shader;
    };
    GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource.c_str());
    GLuint program = gl->createProgram();
    gl->attachShader(program, vertexShader);
    gl->attachShader(program, fragmentShader);
    gl->bindAttribLocation(program, 0, "aPosition");
    gl->linkProgram(program);
    gl->deleteShader(vertexShader);
    gl->deleteShader(fragmentShader);

    GLint linked = 0;
    gl->getProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        STARFISH_LOG_ERROR("WebGL video copy program failed to link\n");
        gl->deleteProgram(program);
        return 0;
    }
    return program;
}

void WebGLRenderingContext::texImageVideo(GLenum target, GLint level,
                                          Optional<GLint> internalFormat,
                                          GLint xoffset, GLint yoffset,
                                          GLenum format, GLenum type,
                                          HTMLVideoElement* video)
{
    MediaPlayer* player = video->activeMediaPlayer();
    // No media data yet (its origin is only known from HAVE_METADATA on),
    // or a srcObject stream (empty currentSrc), whose players do not expose
    // frames to WebGL: nothing to upload.
    if (!player || video->currentSrc()->isEmpty() ||
        video->readyState() < HTMLMediaElement::HAVE_METADATA) {
        return;
    }
    if (!isVideoOriginClean(video)) {
        throw new DOMException(video->document()->executionContext(),
                               DOMException::Code::SECURITY_ERR,
                               "The video element contains cross-origin data.");
    }
    // The GPU copy renders the frame into the level, so it takes only 8-bit
    // RGB(A); a player that keeps its frame in memory is converted on the
    // CPU instead (handleTexImageWithImageSource).
    if ((format != GL_RGBA && format != GL_RGB) || type != GL_UNSIGNED_BYTE) {
        setGLError(GL_INVALID_OPERATION,
                   "Unsupported format or type for a video source.");
        return;
    }
    if (target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                                    target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) {
        setGLError(GL_INVALID_ENUM, "Invalid texture target.");
        return;
    }
    WebGLTexture* texture = getBoundTextureObject(target);
    if (!texture) {
        setGLError(GL_INVALID_OPERATION, "No texture is bound.");
        return;
    }
    if (!internalFormat && !texture->getImageLevelInfo(target, level)) {
        setGLError(GL_INVALID_OPERATION, "The texture level is not defined.");
        return;
    }

    const bool isWebGL2 = webGLVersion() == 2;
    const GLenum bindingTarget =
        target == GL_TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    const GLenum bindingQuery = target == GL_TEXTURE_2D
                                    ? GL_TEXTURE_BINDING_2D
                                    : GL_TEXTURE_BINDING_CUBE_MAP;
    // Without vertex array objects the copy borrows attribute 0 of the
    // current vertex state and puts it back afterwards.
    const bool hasVertexArrays =
        isWebGL2 || WebGLExtensionRegistry::instance()
                        .getGenerator("OES_vertex_array_object", false)
                        .hasValue();
    const bool hasDivisor =
        isWebGL2 || WebGLExtensionRegistry::instance()
                        .getGenerator("ANGLE_instanced_arrays", false)
                        .hasValue();

    // Save every piece of context state the copy changes.
    GLint savedActiveTexture, savedUnit0Texture2D, savedUnit0Binding,
        savedProgram, savedArrayBuffer, savedFramebuffer,
        savedReadFramebuffer = 0, savedUnpackAlignment, savedUnpackBuffer = 0,
        savedUnpackRowLength = 0, savedUnpackSkipRows = 0,
        savedUnpackSkipPixels = 0, savedVertexArray = 0;
    GLint savedViewport[4];
    GLboolean savedColorMask[4];
    const GLenum caps[] = { GL_BLEND,        GL_CULL_FACE,
                            GL_DEPTH_TEST,   GL_SCISSOR_TEST,
                            GL_STENCIL_TEST, GL_RASTERIZER_DISCARD };
    const size_t capCount = isWebGL2 ? 6 : 5;
    GLboolean savedCaps[6];
    GLint savedAttribEnabled = 0, savedAttribSize = 0, savedAttribType = 0,
          savedAttribNormalized = 0, savedAttribStride = 0,
          savedAttribBuffer = 0, savedAttribDivisor = 0;
    void* savedAttribPointer = nullptr;

    m_gl->getIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);
    m_gl->activeTexture(GL_TEXTURE0);
    m_gl->getIntegerv(GL_TEXTURE_BINDING_2D, &savedUnit0Texture2D);
    m_gl->getIntegerv(bindingQuery, &savedUnit0Binding);
    m_gl->getIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
    m_gl->getIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
    m_gl->getIntegerv(GL_FRAMEBUFFER_BINDING, &savedFramebuffer);
    m_gl->getIntegerv(GL_VIEWPORT, savedViewport);
    m_gl->getBooleanv(GL_COLOR_WRITEMASK, savedColorMask);
    m_gl->getIntegerv(GL_UNPACK_ALIGNMENT, &savedUnpackAlignment);
    if (isWebGL2) {
        m_gl->getIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFramebuffer);
        m_gl->getIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &savedUnpackBuffer);
        m_gl->getIntegerv(GL_UNPACK_ROW_LENGTH, &savedUnpackRowLength);
        m_gl->getIntegerv(GL_UNPACK_SKIP_ROWS, &savedUnpackSkipRows);
        m_gl->getIntegerv(GL_UNPACK_SKIP_PIXELS, &savedUnpackSkipPixels);
    }
    if (hasVertexArrays) {
        m_gl->getIntegerv(GL_VERTEX_ARRAY_BINDING, &savedVertexArray);
    } else {
        m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                                &savedAttribEnabled);
        m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE,
                                &savedAttribSize);
        m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE,
                                &savedAttribType);
        m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
                                &savedAttribNormalized);
        m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE,
                                &savedAttribStride);
        m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
                                &savedAttribBuffer);
        m_gl->getVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER,
                                      &savedAttribPointer);
        if (hasDivisor) {
            m_gl->getVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_DIVISOR,
                                    &savedAttribDivisor);
        }
    }
    for (size_t i = 0; i < capCount; i++) {
        savedCaps[i] = m_gl->isEnabled(caps[i]);
    }

    // A frame a player uploads from memory is tightly packed.
    m_gl->pixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (isWebGL2) {
        m_gl->bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        m_gl->pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        m_gl->pixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        m_gl->pixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    }

    // A 2D frame texture is kept for the next upload so that same-sized
    // frames update its storage in place; an external one is released below
    // so it does not hold on to a decoder buffer.
    GLuint source = m_videoCopySource;
    if (!source) {
        m_gl->genTextures(1, &source);
    }
    size_t width = m_videoCopySourceWidth, height = m_videoCopySourceHeight;
    void* image = nullptr;
    // Record errors queued before the copy, so the checks below only see
    // the ones it raises.
    // Bounded in case a driver keeps reporting the same error.
    for (int i = 0; i < 8 && hasNewGLError(); i++) {
    }
    GLenum sourceTarget =
        player->bindCurrentVideoFrame(m_gl, source, width, height, image);
    if (m_gl->getError() != GL_NO_ERROR) {
        // e.g. the frame upload or the image binding failed; that is the
        // engine's failure, not an error of the page's call.
        sourceTarget = 0;
        width = height = 0;
    }
    if (sourceTarget == GL_TEXTURE_2D || source == m_videoCopySource) {
        m_videoCopySource = source;
        m_videoCopySourceWidth = width;
        m_videoCopySourceHeight = height;
    }

    GLuint framebuffer = 0;
    GLuint vertexArray = 0;
    bool allocated = false;
    if (sourceTarget) {
        // A plain 2D texture without mipmaps is incomplete under the default
        // minification filter; the frame is drawn 1:1, so LINEAR is exact.
        m_gl->texParameteri(sourceTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        m_gl->texParameteri(sourceTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_gl->texParameteri(sourceTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_gl->texParameteri(sourceTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (internalFormat) {
            m_gl->bindTexture(bindingTarget, texture->glObject());
            m_gl->texImage2D(target, level, internalFormat.value(), width,
                             height, 0, format, type, nullptr);
            if (hasNewGLError()) {
                sourceTarget = 0;
            } else {
                texture->setImageLevelInfo(target, level,
                                           internalFormat.value(), width,
                                           height, 1, type);
                allocated = true;
            }
        }
    }
    if (sourceTarget) {
        const TextureImageLevelInfo* levelInfo =
            texture->getImageLevelInfo(target, level);
        bool fits = levelInfo && xoffset >= 0 && yoffset >= 0 &&
                    xoffset + width <= size_t(levelInfo->width) &&
                    yoffset + height <= size_t(levelInfo->height);

        m_gl->genFramebuffers(1, &framebuffer);
        m_gl->bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        m_gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                                   texture->glObject(), level);
        size_t programIndex = sourceTarget == GL_TEXTURE_2D ? 0 : 1;
        if (!m_videoCopyPrograms[programIndex]) {
            m_videoCopyPrograms[programIndex] =
                createVideoCopyProgram(m_gl, programIndex == 1);
        }
        GLuint program = m_videoCopyPrograms[programIndex];

        if (!fits) {
            setGLError(GL_INVALID_VALUE,
                       "The video frame does not fit in the texture level.");
        } else if (!program || m_gl->checkFramebufferStatus(GL_FRAMEBUFFER) !=
                                   GL_FRAMEBUFFER_COMPLETE) {
            setGLError(GL_INVALID_OPERATION,
                       "The texture level cannot receive a video frame.");
            if (allocated) {
                // Do not leave the new level holding uninitialized memory.
                std::vector<GLubyte> zeros(width * height * 4);
                m_gl->texImage2D(target, level, internalFormat.value(), width,
                                 height, 0, format, type, zeros.data());
            }
        } else {
            if (!m_videoCopyQuad) {
                const GLfloat quad[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
                m_gl->genBuffers(1, &m_videoCopyQuad);
                m_gl->bindBuffer(GL_ARRAY_BUFFER, m_videoCopyQuad);
                m_gl->bufferData(GL_ARRAY_BUFFER, sizeof(quad), quad,
                                 GL_STATIC_DRAW);
            }
            if (hasVertexArrays) {
                m_gl->genVertexArrays(1, &vertexArray);
                m_gl->bindVertexArray(vertexArray);
            } else if (hasDivisor) {
                m_gl->vertexAttribDivisor(0, 0);
            }
            m_gl->bindBuffer(GL_ARRAY_BUFFER, m_videoCopyQuad);
            m_gl->enableVertexAttribArray(0);
            m_gl->vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

            m_gl->bindTexture(sourceTarget, source);
            m_gl->useProgram(program);
            m_gl->uniform1i(m_gl->getUniformLocation(program, "uFlipY"),
                            m_unpackFlipY);
            for (size_t i = 0; i < capCount; i++) {
                m_gl->disable(caps[i]);
            }
            m_gl->colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            m_gl->viewport(xoffset, yoffset, width, height);
            m_gl->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

    // The page already got the errors that apply to its call; whatever the
    // driver raised for the copy itself (e.g. an ES2 FBO rejecting level > 0,
    // reported above as INVALID_OPERATION) is not the page's.
    for (int i = 0; i < 8 && m_gl->getError() != GL_NO_ERROR; i++) {
    }

    // Restore the saved state.
    if (vertexArray) {
        m_gl->deleteVertexArrays(1, &vertexArray);
    }
    if (hasVertexArrays) {
        m_gl->bindVertexArray(savedVertexArray);
    } else if (sourceTarget) {
        m_gl->bindBuffer(GL_ARRAY_BUFFER, savedAttribBuffer);
        m_gl->vertexAttribPointer(0, savedAttribSize, savedAttribType,
                                  savedAttribNormalized, savedAttribStride,
                                  savedAttribPointer);
        if (!savedAttribEnabled) {
            m_gl->disableVertexAttribArray(0);
        }
        if (hasDivisor) {
            m_gl->vertexAttribDivisor(0, savedAttribDivisor);
        }
    }
    if (framebuffer) {
        m_gl->deleteFramebuffers(1, &framebuffer);
    }
    if (isWebGL2) {
        m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, savedFramebuffer);
        m_gl->bindFramebuffer(GL_READ_FRAMEBUFFER, savedReadFramebuffer);
        m_gl->bindBuffer(GL_PIXEL_UNPACK_BUFFER, savedUnpackBuffer);
        m_gl->pixelStorei(GL_UNPACK_ROW_LENGTH, savedUnpackRowLength);
        m_gl->pixelStorei(GL_UNPACK_SKIP_ROWS, savedUnpackSkipRows);
        m_gl->pixelStorei(GL_UNPACK_SKIP_PIXELS, savedUnpackSkipPixels);
    } else {
        m_gl->bindFramebuffer(GL_FRAMEBUFFER, savedFramebuffer);
    }
    m_gl->pixelStorei(GL_UNPACK_ALIGNMENT, savedUnpackAlignment);
    for (size_t i = 0; i < capCount; i++) {
        if (savedCaps[i]) {
            m_gl->enable(caps[i]);
        }
    }
    m_gl->colorMask(savedColorMask[0], savedColorMask[1], savedColorMask[2],
                    savedColorMask[3]);
    m_gl->viewport(savedViewport[0], savedViewport[1], savedViewport[2],
                   savedViewport[3]);
    m_gl->useProgram(savedProgram);
    m_gl->bindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
    if (sourceTarget && sourceTarget != GL_TEXTURE_2D) {
        m_gl->bindTexture(sourceTarget, 0);
    }
    if (source != m_videoCopySource) {
        m_gl->deleteTextures(1, &source);
    }
    if (image) {
        m_gl->xglDestroyImage(image);
    }
    m_gl->bindTexture(GL_TEXTURE_2D, savedUnit0Texture2D);
    if (bindingTarget != GL_TEXTURE_2D) {
        m_gl->bindTexture(bindingTarget, savedUnit0Binding);
    }
    m_gl->activeTexture(savedActiveTexture);
}
#endif

void WebGLRenderingContext::releaseInternalGLObjects()
{
    // Every context shares one GL namespace, so destroying this one frees
    // no name. A 0 name is ignored.
    m_gl->deleteTextures(2, m_incompleteTextureStandIns);
    m_incompleteTextureStandIns[0] = m_incompleteTextureStandIns[1] = 0;
#ifdef STARFISH_ENABLE_MULTIMEDIA
    for (GLuint& program : m_videoCopyPrograms) {
        if (program) {
            m_gl->deleteProgram(program);
            program = 0;
        }
    }
    if (m_videoCopyQuad) {
        m_gl->deleteBuffers(1, &m_videoCopyQuad);
        m_videoCopyQuad = 0;
    }
    if (m_videoCopySource) {
        m_gl->deleteTextures(1, &m_videoCopySource);
        m_videoCopySource = 0;
    }
#endif
}

void WebGLRenderingContext::implementUniformNfv(
    size_t n, void (GL::*uniformNfv)(GLint, GLsizei, const GLfloat*),
    Optional<WebGLUniformLocation*> location, Float32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    throwIfNotConverted(data);
    ENTER_CONTEXT_SCOPE();
    /* location is nullable. */
    if (!location.hasValue()) {
        return;
    }
    if (!isFromCurrentProgram(location.value())) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (data.isFloat32ArrayValue()) {
        const ScriptFloat32Array values = data.getFloat32ArrayValue();
        const size_t dataLength = values->arrayLength();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < n ||
            srcLength % n != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        /* count specifies the number of sets. */
        (gl()->*uniformNfv)(
            location.value()->location(), srcLength / n,
            reinterpret_cast<const GLfloat*>(values->rawBuffer()) + srcOffset);
    } else if (data.isSequenceOfGLfloatValue()) {
        const GCAtomicVector<double> values = data.getSequenceOfGLfloatValue();
        const size_t dataLength = values.size();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < n ||
            srcLength % n != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        const std::vector<GLfloat> floats(values.begin(), values.end());
        /* count specifies the number of sets. */
        (gl()->*uniformNfv)(location.value()->location(), srcLength / n,
                            floats.data() + srcOffset);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void WebGLRenderingContext::uniform1fv(Optional<WebGLUniformLocation*> location,
                                       Float32List v)
{
    implementUniformNfv(1, &GL::uniform1fv, location, v, 0, 0);
}

void WebGLRenderingContext::uniform2fv(Optional<WebGLUniformLocation*> location,
                                       Float32List v)
{
    implementUniformNfv(2, &GL::uniform2fv, location, v, 0, 0);
}

void WebGLRenderingContext::uniform3fv(Optional<WebGLUniformLocation*> location,
                                       Float32List v)
{
    implementUniformNfv(3, &GL::uniform3fv, location, v, 0, 0);
}

void WebGLRenderingContext::uniform4fv(Optional<WebGLUniformLocation*> location,
                                       Float32List v)
{
    implementUniformNfv(4, &GL::uniform4fv, location, v, 0, 0);
}

void WebGLRenderingContext::implementUniformNiv(
    size_t n, void (GL::*uniformNiv)(GLint, GLsizei, const GLint*),
    Optional<WebGLUniformLocation*> location, Int32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    throwIfNotConverted(data);
    ENTER_CONTEXT_SCOPE();
    /* location is nullable. */
    if (!location) {
        return;
    }
    if (!isFromCurrentProgram(location.value())) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    WebGLProgram* program = location.value()->program();
    const GLint uniform = location.value()->location();
    // count specifies the number of sets. The values a sampler array takes
    // from `data` must be texture units (OpenGL ES 3.0.6 §2.12.6).
    auto upload = [&](const GLint* values, GLsizei count) {
        const Optional<size_t> sampler =
            n == 1 ? program->samplerIndex(uniform) : Optional<size_t>();
        if (!sampler) {
            (gl()->*uniformNiv)(uniform, count, values);
            return;
        }
        // The count goes first, as in Mesa and ANGLE.
        if (count > 1 && !program->samplers()[sampler.value()].isArray) {
            setGLError(GL_INVALID_OPERATION,
                       "more than one value for a sampler that is no array");
            return;
        }
        if (!areTextureUnits(values,
                             program->samplerSpan(sampler.value(), count))) {
            setGLError(GL_INVALID_VALUE, "sampler value is no texture unit");
            return;
        }
        hasNewGLError(); // Keep an earlier error from failing this call.
        (gl()->*uniformNiv)(uniform, count, values);
        if (!hasNewGLError()) {
            program->setSamplerUnits(sampler.value(), values, count);
        }
    };
    if (data.isInt32ArrayValue()) {
        const ScriptInt32Array values = data.getInt32ArrayValue();
        const size_t dataLength = values->arrayLength();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < n ||
            srcLength % n != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        upload(reinterpret_cast<const GLint*>(values->rawBuffer()) + srcOffset,
               srcLength / n);
    } else if (data.isSequenceOfGLintValue()) {
        const GCAtomicVector<int32_t> values = data.getSequenceOfGLintValue();
        const size_t dataLength = values.size();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < n ||
            srcLength % n != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        upload(values.data() + srcOffset, srcLength / n);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void WebGLRenderingContext::uniform1iv(Optional<WebGLUniformLocation*> location,
                                       Int32List v)
{
    implementUniformNiv(1, &GL::uniform1iv, location, v, 0, 0);
}

void WebGLRenderingContext::uniform2iv(Optional<WebGLUniformLocation*> location,
                                       Int32List v)
{
    implementUniformNiv(2, &GL::uniform2iv, location, v, 0, 0);
}

void WebGLRenderingContext::uniform3iv(Optional<WebGLUniformLocation*> location,
                                       Int32List v)
{
    implementUniformNiv(3, &GL::uniform3iv, location, v, 0, 0);
}

void WebGLRenderingContext::uniform4iv(Optional<WebGLUniformLocation*> location,
                                       Int32List v)
{
    implementUniformNiv(4, &GL::uniform4iv, location, v, 0, 0);
}

void WebGLRenderingContext::implementUniformMatrixMxNfv(
    size_t m, size_t n,
    void (GL::*uniformMatrixMxNfv)(GLint, GLsizei, GLboolean, const GLfloat*),
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    throwIfNotConverted(data);
    ENTER_CONTEXT_SCOPE();
    /* location is nullable. */
    if (!location.hasValue()) {
        return;
    }
    if (!isFromCurrentProgram(location.value())) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLenum expectedType = GL_NONE;
    if (m == 2 && n == 2) {
        expectedType = GL_FLOAT_MAT2;
    } else if (m == 3 && n == 3) {
        expectedType = GL_FLOAT_MAT3;
    } else if (m == 4 && n == 4) {
        expectedType = GL_FLOAT_MAT4;
    } else if (m == 2 && n == 3) {
        expectedType = GL_FLOAT_MAT2x3;
    } else if (m == 3 && n == 2) {
        expectedType = GL_FLOAT_MAT3x2;
    } else if (m == 2 && n == 4) {
        expectedType = GL_FLOAT_MAT2x4;
    } else if (m == 4 && n == 2) {
        expectedType = GL_FLOAT_MAT4x2;
    } else if (m == 3 && n == 4) {
        expectedType = GL_FLOAT_MAT3x4;
    } else if (m == 4 && n == 3) {
        expectedType = GL_FLOAT_MAT4x3;
    }

    GLenum actualType =
        getUniformType(location.value()->program(), location.value());
    if (actualType != GL_NONE && actualType != expectedType) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    // OpenGL ES 2.0 §2.10.4: transpose must be FALSE (WebGL 2 allows TRUE).
    if (transpose && !isWebGL2RenderingContext()) {
        setGLError(GL_INVALID_VALUE);
        return;
    }
    if (data.isFloat32ArrayValue()) {
        const ScriptFloat32Array values = data.getFloat32ArrayValue();
        const size_t dataLength = values->arrayLength();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < (m * n) ||
            srcLength % (m * n) != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        /* count specifies the number of matrices. */
        (gl()->*uniformMatrixMxNfv)(
            location.value()->location(), srcLength / (m * n), transpose,
            reinterpret_cast<const GLfloat*>(values->rawBuffer()) + srcOffset);
    } else if (data.isSequenceOfGLfloatValue()) {
        const GCAtomicVector<double> values = data.getSequenceOfGLfloatValue();
        const size_t dataLength = values.size();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < (m * n) ||
            srcLength % (m * n) != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        const std::vector<GLfloat> floats(values.begin(), values.end());
        /* count specifies the number of matrices. */
        (gl()->*uniformMatrixMxNfv)(location.value()->location(),
                                    srcLength / (m * n), transpose,
                                    floats.data() + srcOffset);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void WebGLRenderingContext::uniformMatrix2fv(
    Optional<WebGLUniformLocation*> uniform, GLboolean transpose,
    Float32List value)
{
    implementUniformMatrixMxNfv(2, 2, &GL::uniformMatrix2fv, uniform, transpose,
                                value, 0, 0);
}

void WebGLRenderingContext::uniformMatrix3fv(
    Optional<WebGLUniformLocation*> uniform, GLboolean transpose,
    Float32List value)
{
    implementUniformMatrixMxNfv(3, 3, &GL::uniformMatrix3fv, uniform, transpose,
                                value, 0, 0);
}

void WebGLRenderingContext::uniformMatrix4fv(
    Optional<WebGLUniformLocation*> uniform, GLboolean transpose,
    Float32List value)
{
    implementUniformMatrixMxNfv(4, 4, &GL::uniformMatrix4fv, uniform, transpose,
                                value, 0, 0);
}

void WebGLRenderingContext::setGLError(GLenum code, const char* message)
{
    m_GLErrors.insert(code);
    if (message) {
        TRACE(WEBGL, "Error(%s): %s", glValueString(code), message);
    }
}

/*
Note: Use hasGLError() to internally check for GL errors. When `glGetError` is
called, the code returned is cleared inside it. If we use `glGetError` directly,
users would not be able to get error code properly. So, we first store the code
from `glGetError`, and then use it. The error code stored will be cleared when
users call gl.getError().
*/
bool WebGLRenderingContext::hasGLError()
{
    return hasNewGLError() || (!m_GLErrors.empty());
}

bool WebGLRenderingContext::hasNewGLError()
{
    GLenum code = m_gl->getError();
    if (code != GL_NO_ERROR) {
        TRACE(WEBGL, "Error:", glValueString(code));
        setGLError(code);
        return true;
    }
    return false;
}

bool WebGLRenderingContext::executeInContextScope(
    std::function<void()> callback)
{
    ENTER_CONTEXT_SCOPE(false);
    callback();
    return true;
}

WebGLRenderingContextState* WebGLRenderingContext::getState()
{
    return m_state;
}

// The GLSL ES 1.00 §3.1 source character set: printable ASCII except '"',
// '$', '\'', '@', '\\' and '`', plus tab, newline, vertical tab, form feed
// and carriage return.
static bool isGLSLSourceCharacter(char32_t c)
{
    if (c >= 0x20 && c < 0x7F) {
        return c != '"' && c != '$' && c != '\'' && c != '@' && c != '\\' &&
               c != '`';
    }
    return c >= '\t' && c <= '\r';
}

bool WebGLRenderingContext::checkAttribOrUniformName(String* name)
{
    // If the passed name is longer than the restriction defined in Maximum
    // Uniform and Attribute Location Lengths, generates an INVALID_VALUE error
    // and returns -1
    size_t maxLen = (webGLVersion() >= 2)
                        ? 1024
                        : kMaximumUniformAndAttributeLocationLengths;
    if (name->length() > maxLen) {
        setGLError(GL_INVALID_VALUE);
        return false;
    }

    // WebGL 1.0 and 2.0 "Characters Outside the GLSL Source Character Set".
    for (size_t i = 0; i < name->length(); i++) {
        if (!isGLSLSourceCharacter(name->charAt(i))) {
            setGLError(GL_INVALID_VALUE);
            return false;
        }
    }

    // Returns -1 if name starts with one of the reserved WebGL prefixes per
    // GLSL Constructs.
    if (name->startsWith("webgl_") || name->startsWith("_webgl_")) {
        return false;
    }

    return true;
}

bool WebGLRenderingContext::isFromCurrentContext(WebGLObject* object)
{
    STARFISH_ASSERT(object != nullptr);

    return object->context() == this;
}

bool WebGLRenderingContext::hasGLObject(WebGLProgram* program)
{
    return !program->isDeleted() || m_state->webGLProgram() == program;
}

bool WebGLRenderingContext::hasGLObject(WebGLShader* shader)
{
    return !shader->isDeleted() || shader->attachCount() > 0;
}

template <typename T>
static bool validateLiveObject(WebGLRenderingContext* context, T* object)
{
    if (!context->isFromCurrentContext(object)) {
        context->setGLError(GL_INVALID_OPERATION);
        return false;
    }
    if (!context->hasGLObject(object)) {
        context->setGLError(GL_INVALID_VALUE);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateProgramOrShader(WebGLProgram* program)
{
    return validateLiveObject(this, program);
}

bool WebGLRenderingContext::validateProgramOrShader(WebGLShader* shader)
{
    return validateLiveObject(this, shader);
}

bool WebGLRenderingContext::isBoundCubeMapTexture(GLenum target)
{
    if (target > GL_TEXTURE_BINDING_CUBE_MAP &&
        target < GL_MAX_CUBE_MAP_TEXTURE_SIZE) {
        if (m_boundTextures.find(GL_TEXTURE_CUBE_MAP) !=
            m_boundTextures.end()) {
            return true;
        }
    }
    return false;
}

bool WebGLRenderingContext::isFromCurrentProgram(WebGLUniformLocation* location)
{
    // Compare objects, not GL names: a freed name may be reused. The location
    // must also predate no relink of its program.
    Optional<WebGLProgram*> current = m_state->webGLProgram();
    return current && location->program() == current.value() &&
           location->linkCount() == current.value()->linkCount();
}

bool WebGLRenderingContext::areTextureUnits(const GLint* values, size_t count)
{
    GLint maxUnits = 0;
    m_gl->getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
    for (size_t i = 0; i < count; ++i) {
        if (values[i] < 0 || values[i] >= maxUnits) {
            return false;
        }
    }
    return true;
}

bool WebGLRenderingContext::isExtensionEnabled(const char* name)
{
    const auto& iter = m_enabledExtensions.find(name);
    if (iter != m_enabledExtensions.end()) {
        return true;
    }
    return false;
}

bool WebGLRenderingContext::isDefaultFramebufferBound()
{
    return !m_state->hasWebGLFramebuffer();
}

// WebGL 1.0 §5.14.11, §5.14.12, §6.8; OpenGL ES 3.0.6 §4.4.4.4: drawing to or
// reading from a framebuffer that is not complete for WebGL, which the driver
// may still take, is INVALID_FRAMEBUFFER_OPERATION.
bool WebGLRenderingContext::validateFramebufferComplete(GLenum target)
{
    WebGLFramebuffer* framebuffer = getBoundFramebuffer(target).valueOrNull();
    if (!framebuffer || framebuffer->webGLStatus() == GL_FRAMEBUFFER_COMPLETE) {
        return true;
    }
    setGLError(GL_INVALID_FRAMEBUFFER_OPERATION, "framebuffer is not complete");
    return false;
}

GLuint WebGLRenderingContext::getCurrentFBO()
{
    return m_state->hasWebGLFramebuffer()
               ? m_state->webGLFramebuffer()->glObject()
               : m_framebufferTexture->fbo();
}

GLint WebGLRenderingContext::getCurrentProgram()
{
    GLint program = 0;
    m_gl->getIntegerv(GL_CURRENT_PROGRAM, &program);
    return program;
}

WebGLTexture* WebGLRenderingContext::sampledTexture(GLuint unit,
                                                    GLenum target) const
{
    auto it = m_boundTextureObjects.find(
        textureBindingKey(GL_TEXTURE0 + unit, target));
    return it != m_boundTextureObjects.end() ? it->second : nullptr;
}

bool WebGLRenderingContext::isIncompleteForWebGL1(const WebGLTexture* texture,
                                                  GLenum target)
{
    const TextureImageLevelInfo* base = texture->getImageLevelInfo(
        target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X
                                      : GL_TEXTURE_2D,
        0);
    if (!base) {
        return false; // The driver finds a texture without level 0 incomplete.
    }
    const GLenum minFilter = texture->minFilter();
    const bool mipmapped = minFilter != GL_NEAREST && minFilter != GL_LINEAR;
    if (!(isPowerOfTwo(base->width) && isPowerOfTwo(base->height)) &&
        (mipmapped || !texture->wrapsClampToEdge())) {
        return true;
    }
    if (base->type != GL_FLOAT && base->type != GL_HALF_FLOAT_OES) {
        return false;
    }
    const bool linear =
        !texture->magFilterIsNearest() ||
        (minFilter != GL_NEAREST && minFilter != GL_NEAREST_MIPMAP_NEAREST);
    // Names made once: a lookup by const char* would allocate on every draw.
    static const std::string floatLinear("OES_texture_float_linear");
    static const std::string halfFloatLinear("OES_texture_half_float_linear");
    return linear &&
           m_enabledExtensions.find(base->type == GL_FLOAT ? floatLinear
                                                           : halfFloatLinear) ==
               m_enabledExtensions.end();
}

bool WebGLRenderingContext::bindIncompleteTextureStandIns()
{
    WebGLProgram* program = m_state->webGLProgram().valueOrNull();
    if (webGLVersion() != 1 || !program) {
        return false;
    }
    bool bound = false;
    for (const WebGLProgram::SamplerInfo& sampler : program->samplers()) {
        WebGLTexture* texture = sampledTexture(sampler.unit, sampler.target);
        if (!texture || !isIncompleteForWebGL1(texture, sampler.target)) {
            continue;
        }
        // No image ever: incomplete, so it samples (0,0,0,1) (OpenGL ES 2.0
        // §3.8.2). Its first bind fixes its target, hence one per target.
        GLuint& standIn =
            m_incompleteTextureStandIns[sampler.target == GL_TEXTURE_CUBE_MAP];
        if (!standIn) {
            m_gl->genTextures(1, &standIn);
        }
        m_gl->activeTexture(GL_TEXTURE0 + sampler.unit);
        m_gl->bindTexture(sampler.target, standIn);
        bound = true;
    }
    return bound;
}

void WebGLRenderingContext::restoreSampledTextureBindings()
{
    for (const WebGLProgram::SamplerInfo& sampler :
         m_state->webGLProgram().value()->samplers()) {
        WebGLTexture* texture = sampledTexture(sampler.unit, sampler.target);
        m_gl->activeTexture(GL_TEXTURE0 + sampler.unit);
        m_gl->bindTexture(sampler.target, texture ? texture->glObject() : 0);
    }
    m_gl->activeTexture(m_activeTextureUnit);
}

bool WebGLRenderingContext::validateFeedbackLoops()
{
    WebGLFramebuffer* framebuffer =
        getBoundFramebuffer(GL_DRAW_FRAMEBUFFER).valueOrNull();
    WebGLProgram* program = m_state->webGLProgram().valueOrNull();
    if (!framebuffer || !program) {
        return true; // No texture is attached to the default framebuffer.
    }
    for (const WebGLProgram::SamplerInfo& sampler : program->samplers()) {
        WebGLTexture* texture = sampledTexture(sampler.unit, sampler.target);
        if (!texture ||
            !framebuffer->attachesReadLevel(texture, [&](GLint level) {
                GLint base = 0;
                GLint last = 0;
                return sampledLevels(texture, sampler.unit, sampler.target,
                                     base, last) &&
                       level >= base && level <= last;
            })) {
            continue;
        }
        // A WebGL 1 texture replaced by a stand-in is not read either.
        if (webGLVersion() == 1 &&
            isIncompleteForWebGL1(texture, sampler.target)) {
            continue;
        }
        setGLError(GL_INVALID_OPERATION,
                   "feedback loop between a sampled texture and the draw "
                   "framebuffer");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::sampledLevels(const WebGLTexture* texture,
                                          GLuint unit, GLenum target,
                                          GLint& base, GLint& last)
{
    // OpenGL ES 3.0.6 §3.8.10.4: the base level, and with a mipmap filter the
    // levels up to q. Only a texture whose record defines all of them counts
    // as read (an incomplete one samples no image, and a loop wrongly seen
    // would drop a valid draw). shortcut: format filterability (an integer or
    // depth texture filtered LINEAR) and the size and format consistency of
    // the mip levels are not modelled; model them along with full
    // completeness.
    base = texture->baseLevel();
    GLint maxLevel = texture->maxLevel();
    const bool isCube = target == GL_TEXTURE_CUBE_MAP;
    const GLenum firstFace = isCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : target;
    if (texture->isImmutable()) {
        // The base and maximum levels clamp to the storage, every level of
        // which is defined (setStorage*()).
        GLint top = 0;
        while (texture->getImageLevelInfo(firstFace, top + 1)) {
            ++top;
        }
        base = std::min(base, top);
        maxLevel = std::min(std::max(maxLevel, base), top);
    }
    const TextureImageLevelInfo* info =
        texture->getImageLevelInfo(firstFace, base);
    if (!info) {
        return false;
    }
    last = base;
    const GLenum minFilter = samplerMinFilter(unit, texture);
    if (minFilter != GL_NEAREST && minFilter != GL_LINEAR) {
        if (maxLevel < base) {
            return false; // Incomplete.
        }
        GLsizei size = std::max(info->width, info->height);
        if (target == GL_TEXTURE_3D) {
            size = std::max(size, info->depth);
        }
        GLint levels = 0; // floor(log2(size))
        for (; size > 1; size >>= 1) {
            ++levels;
        }
        // Differences only: base + levels may not fit.
        last = levels > maxLevel - base ? maxLevel : base + levels;
    }
    const GLenum lastFace = isCube ? GL_TEXTURE_CUBE_MAP_NEGATIVE_Z : target;
    for (GLint level = base; level <= last; ++level) {
        for (GLenum face = firstFace; face <= lastFace; ++face) {
            if (!texture->getImageLevelInfo(face, level)) {
                return false;
            }
        }
    }
    return true;
}

GLenum WebGLRenderingContext::samplerMinFilter(
    GLuint unit, const WebGLTexture* texture) const
{
    return texture->minFilter();
}

bool WebGLRenderingContext::validateStencilSettings()
{
    // WebGL 1.0 "Stencil Separate Mask and Reference Value": Direct3D has a
    // single stencil reference and mask, so front and back must agree on the
    // bits the draw framebuffer's stencil buffer has.
    if (!m_gl->isEnabled(GL_STENCIL_TEST)) {
        return true;
    }
    GLint bits = 0;
    m_gl->getIntegerv(GL_STENCIL_BITS, &bits);
    if (bits <= 0) {
        return true;
    }
    const GLint maxValue = (1 << std::min(bits, 30)) - 1;
    GLint front[3] = {};
    GLint back[3] = {};
    m_gl->getIntegerv(GL_STENCIL_WRITEMASK, &front[0]);
    m_gl->getIntegerv(GL_STENCIL_BACK_WRITEMASK, &back[0]);
    m_gl->getIntegerv(GL_STENCIL_VALUE_MASK, &front[1]);
    m_gl->getIntegerv(GL_STENCIL_BACK_VALUE_MASK, &back[1]);
    m_gl->getIntegerv(GL_STENCIL_REF, &front[2]);
    m_gl->getIntegerv(GL_STENCIL_BACK_REF, &back[2]);
    auto clampRef = [maxValue](GLint ref) {
        return std::min(std::max(ref, 0), maxValue);
    };
    if ((front[0] & maxValue) != (back[0] & maxValue) ||
        (front[1] & maxValue) != (back[1] & maxValue) ||
        clampRef(front[2]) != clampRef(back[2])) {
        setGLError(GL_INVALID_OPERATION,
                   "front and back stencil settings do not match");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateDrawCallVertexAttribs()
{
    if (webGLVersion() < 2) {
        return true;
    }

    GLint program = getCurrentProgram();
    if (program == 0) {
        return true;
    }

    GLint activeAttribs = 0;
    m_gl->getProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttribs);
    for (GLint i = 0; i < activeAttribs; ++i) {
        char nameBuf[256];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        m_gl->getActiveAttrib(program, i, sizeof(nameBuf), &length, &size,
                              &type, nameBuf);
        GLint location = m_gl->getAttribLocation(program, nameBuf);
        if (location < 0) {
            continue;
        }

        bool isShaderInt = false;
        bool isShaderUint = false;
        switch (type) {
        case GL_INT:
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:
            isShaderInt = true;
            break;
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
            isShaderUint = true;
            break;
        default:
            break;
        }

        bool arrayEnabled = m_state->isVertexAttribArrayEnabled(location);
        if (arrayEnabled) {
            GLenum bufferType = m_state->getVertexAttribType(location);
            if (isShaderInt && bufferType != GL_INT) {
                setGLError(GL_INVALID_OPERATION,
                           "Active signed integer vertex attribute requires "
                           "signed integer buffer");
                return false;
            }
            if (isShaderUint && bufferType != GL_UNSIGNED_INT) {
                setGLError(GL_INVALID_OPERATION,
                           "Active unsigned integer vertex attribute requires "
                           "unsigned integer buffer");
                return false;
            }
            if (!isShaderInt && !isShaderUint && bufferType != GL_FLOAT) {
                setGLError(
                    GL_INVALID_OPERATION,
                    "Active float vertex attribute requires float buffer");
                return false;
            }
        } else {
            GLenum genericType = m_state->getGenericVertexAttribType(location);
            if (isShaderInt && genericType != GL_INT) {
                setGLError(GL_INVALID_OPERATION,
                           "Type mismatch for active integer attribute");
                return false;
            }
            if (isShaderUint && genericType != GL_UNSIGNED_INT) {
                setGLError(
                    GL_INVALID_OPERATION,
                    "Type mismatch for active unsigned integer attribute");
                return false;
            }
            if (!isShaderInt && !isShaderUint && genericType != GL_FLOAT) {
                setGLError(GL_INVALID_OPERATION,
                           "Type mismatch for active float attribute");
                return false;
            }
        }
    }
    return true;
}

template <typename Clear>
void WebGLRenderingContext::clearWithDefaultState(GLuint framebuffer,
                                                  const Clear& clear)
{
    GLint drawFramebuffer = 0;
    GLfloat clearColor[4];
    GLfloat clearDepth = 1;
    GLint clearStencil = 0;
    GLboolean colorMask[4];
    GLboolean depthMask = GL_TRUE;
    GLint stencilMask = 0;
    GLint stencilBackMask = 0;
    m_gl->getIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    m_gl->getFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    m_gl->getFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth);
    m_gl->getIntegerv(GL_STENCIL_CLEAR_VALUE, &clearStencil);
    m_gl->getBooleanv(GL_COLOR_WRITEMASK, colorMask);
    m_gl->getBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    m_gl->getIntegerv(GL_STENCIL_WRITEMASK, &stencilMask);
    m_gl->getIntegerv(GL_STENCIL_BACK_WRITEMASK, &stencilBackMask);
    const bool scissorTest = m_gl->isEnabled(GL_SCISSOR_TEST);
    const bool rasterizerDiscard =
        isWebGL2RenderingContext() && m_gl->isEnabled(GL_RASTERIZER_DISCARD);

    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    m_gl->disable(GL_SCISSOR_TEST);
    if (rasterizerDiscard) {
        m_gl->disable(GL_RASTERIZER_DISCARD);
    }
    m_gl->colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    m_gl->depthMask(GL_TRUE);
    m_gl->stencilMask(0xFFFFFFFF);
    m_gl->clearColor(0, 0, 0, 0);
    m_gl->clearDepthf(1);
    m_gl->clearStencil(0);

    clear();

    m_gl->clearColor(clearColor[0], clearColor[1], clearColor[2],
                     clearColor[3]);
    m_gl->clearDepthf(clearDepth);
    m_gl->clearStencil(clearStencil);
    m_gl->colorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    m_gl->depthMask(depthMask);
    m_gl->stencilMaskSeparate(GL_FRONT, static_cast<GLuint>(stencilMask));
    m_gl->stencilMaskSeparate(GL_BACK, static_cast<GLuint>(stencilBackMask));
    if (rasterizerDiscard) {
        m_gl->enable(GL_RASTERIZER_DISCARD);
    }
    if (scissorTest) {
        m_gl->enable(GL_SCISSOR_TEST);
    }
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER,
                          static_cast<GLuint>(drawFramebuffer));
}

void WebGLRenderingContext::completePendingJobs()
{
    /*
        +---------+--------------+----------------------+---------------------+
        | Buffer  | Clear value  | Minimum size         | Defined by default? |
        +---------+--------------+----------------------+---------------------+
        | Color   | (0, 0, 0, 0) | 8 bits per component | yes                 |
        | Depth   | 1.0          | 16 bit integer       | yes                 |
        | Stencil | 0            | 8 bits               | no                  |
        +---------+--------------+----------------------+---------------------+

        By default, after compositing the contents of the drawing buffer shall
        be cleared to their default values, as shown in the table above. This
        default behavior can be changed by setting the `preserveDrawingBuffer`
        attribute of the WebGLContextAttributes object.
    */
    if (!m_hasPendingJobsBetweenFrames) {
        return;
    }
    m_hasPendingJobsBetweenFrames = false;
    if (m_attributes.preserveDrawingBuffer()) {
        return;
    }

    GLbitfield mask = GL_COLOR_BUFFER_BIT;
    if (m_attributes.depth()) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (m_attributes.stencil()) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    // The clear targets the drawing buffer as a whole, whatever framebuffer,
    // scissor, write masks and clear values the page has set (WebGL 1.0
    // §2.2); all of that is page state and is put back afterwards.
    clearWithDefaultState(m_framebufferTexture->fbo(),
                          [&] { m_gl->clear(mask); });
}

// WebGL 1.0 §4.1: a new image reads as color 0, depth 1.0 and stencil 0.
void WebGLRenderingContext::initializeImage(
    GLenum internalFormat, const std::function<void(GLenum)>& attach)
{
    GLbitfield mask = GL_COLOR_BUFFER_BIT;
    switch (internalFormat) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
        mask = GL_DEPTH_BUFFER_BIT;
        break;
    case GL_STENCIL_INDEX8:
        mask = GL_STENCIL_BUFFER_BIT;
        break;
    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        break;
    }
    GLuint framebuffer = 0;
    m_gl->genFramebuffers(1, &framebuffer);
    clearWithDefaultState(framebuffer, [&] {
        if (mask & GL_COLOR_BUFFER_BIT) {
            attach(GL_COLOR_ATTACHMENT0);
        }
        if (mask & GL_DEPTH_BUFFER_BIT) {
            attach(GL_DEPTH_ATTACHMENT);
        }
        if (mask & GL_STENCIL_BUFFER_BIT) {
            attach(GL_STENCIL_ATTACHMENT);
        }
        // An image no framebuffer can be complete with is never drawn or
        // read.
        if (m_gl->checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            return;
        }
        // Clear leaves an integer color buffer undefined (OpenGL ES 3.0.6
        // §4.2.3).
        static const GLuint unsignedZeros[4] = {};
        static const GLint signedZeros[4] = {};
        if (WebGLFramebuffer::isUnsignedIntegerInternalFormat(internalFormat)) {
            m_gl->clearBufferuiv(GL_COLOR, 0, unsignedZeros);
        } else if (WebGLFramebuffer::isSignedIntegerInternalFormat(
                       internalFormat)) {
            m_gl->clearBufferiv(GL_COLOR, 0, signedZeros);
        } else {
            m_gl->clear(mask);
        }
    });
    // Unbound again by now, so this releases the image too.
    m_gl->deleteFramebuffers(1, &framebuffer);
}

void WebGLRenderingContext::initializeRenderbuffer(
    WebGLRenderbuffer* renderbuffer)
{
    if (!renderbuffer->width() || !renderbuffer->height()) {
        return;
    }
    initializeImage(renderbuffer->internalFormat(), [&](GLenum attachment) {
        m_gl->framebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attachment,
                                      GL_RENDERBUFFER,
                                      renderbuffer->glObject());
    });
}

GL* WebGLRenderingContext::gl()
{
    return m_gl;
}

} // namespace Starfish

#endif
