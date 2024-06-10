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
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/ImageBitmap.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/canvas/webgl/WebGLExtensions.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/worker/util/Trace.h"
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
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"
#include "core/dom/canvas/webgl/WebGLRenderingContextState.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/generated/Float32ArrayOrSequenceOfGLfloatUnion.h"
#include "binding/generated/Int32ArrayOrSequenceOfGLintUnion.h"
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include "binding/generated/ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElementUnion.h"
#include "core/dom/canvas/webgl/WebGLOES_VertexArrayObject.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"

#include "platform/canvas/gl/IncludeGL.h"
#include "platform/canvas/gl/GL.h"

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
    m_isContextLost = false;
    m_hasPendingJobsBetweenFrames = false;
    m_pendingClearMask = 0;
    m_unpackColorSpace = String::createASCIIString("srgb");
    m_drawingBufferColorSpace = String::createASCIIString("srgb");
    m_state = new WebGLRenderingContextState();
    m_gl = m_ownerHTMLCanvasElement->webView()->renderer()->gl();
    if (!WebGLExtensionRegistry::instance().isInitialized()) {
        WebGLExtensionRegistry::instance().initialize(m_gl);
    }
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            auto self = reinterpret_cast<WebGLRenderingContext*>(obj);
            GLErrorSet().swap(self->m_GLErrors);
            GLTextureMap().swap(self->m_boundTextures);
        },
        NULL, NULL, NULL);
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
        m_frameBufferAttributes.antialias = m_attributes.antialias();
        m_frameBufferAttributes.depth = m_attributes.depth();
        m_frameBufferAttributes.stencil = m_attributes.stencil();
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

    // bind default frame buffer
    GLContextScope contextScope(m_context);
    m_gl->bindFramebuffer(GL_FRAMEBUFFER, m_framebufferTexture->fbo());
}

void WebGLRenderingContext::flush()
{
    WebGLRenderingContextBaseMixIn::flush();

    GLRevertableContextScope scope(
        m_context, executionContext()->webBase()->asWebView()->renderer());
    // we need to bind 0(screen) buffer for sending commands to gpu
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, getCurrentFBO());

    // NOTE: According to the specification, the content of the drawing buffer
    // should be cleared with default values after the end of the composition.
    // However, as of now, it's difficult to know when the composition ends and
    // the GL rendering actually ends, so we leave the clearing as a pending job
    // and let it be done lazily. The assumption here is that the engine will
    // get this `flush()` invoked every frame after we call `setNeedsComposite`.

    m_hasPendingJobsBetweenFrames = true;
}

void WebGLRenderingContext::onResize()
{
    WebGLRenderingContextBaseMixIn::onResize();

    // bind default frame buffer
    GLContextScope contextScope(m_context);
    m_gl->bindFramebuffer(GL_FRAMEBUFFER, m_framebufferTexture->fbo());
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
        if (hasGLError()) {                          \
            TRACE(WEBGL,                             \
                  "\033[33m"                         \
                  "GL error detected."               \
                  "\033[0m");                        \
        }                                            \
    });
#endif

/*
Note: Use hasGLError() to internally check for GL errors. When `glGetError` is
called, the code returned is cleared inside it. If we use `glGetError` directly,
users would not be able to get error code properly. So, we first store the code
from `glGetError`, and then use it. The error code stored will be cleared when
users call gl.getError().
*/
bool WebGLRenderingContext::hasGLError()
{
    updateGLError();

    if (!m_GLErrors.empty()) {
        return true;
    }
    return false;
}

void WebGLRenderingContext::updateGLError()
{
    GLenum code = m_gl->getError();
    if (code != GL_NO_ERROR) {
        TRACE(WEBGL, "Error:", glValueString(code));
        setGLError(code);
    }
}

GLenum WebGLRenderingContext::getError()
{
    GLContextScope contextScope(m_context);

    updateGLError();

    GLenum code = GL_NO_ERROR;
    if (!m_GLErrors.empty()) {
        code = *m_GLErrors.begin();
        m_GLErrors.erase(m_GLErrors.begin());
    }
    return code;
}

void WebGLRenderingContext::setGLError(GLenum code, const char* message)
{
    m_GLErrors.insert(code);
    if (message) {
        TRACE(WEBGL, "Error(%s): %s", glValueString(code), message);
    }
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

Nullable<WebGLContextAttributes> WebGLRenderingContext::getContextAttributes()
{
    ENTER_CONTEXT_SCOPE(Nullable<WebGLContextAttributes>());

    return m_attributes;
}

Nullable<GCVector<String*>> WebGLRenderingContext::getSupportedExtensions()
{
    ENTER_CONTEXT_SCOPE(Nullable<GCVector<String*>>());

    return WebGLExtensionRegistry::instance().getSupportedExtensions();
}

bool WebGLRenderingContext::isContextLost()
{
    return m_isContextLost;
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

    if (m_hasPendingJobsBetweenFrames) {
        // If `preserveDrawingBuffer` is true, the contents of the drawing
        // buffer shall be preserved until the author either clears or
        // overwrites them.
        if (!m_attributes.preserveDrawingBuffer()) {
            uint32_t mask = GL_COLOR_BUFFER_BIT;

            // NOTE: If a bit of m_pendingClearMask is 1, it means that users
            // have already set a value corresponding to that bit. Therefore, we
            // will keep the value set by users instead of the default value.

            if (!(m_pendingClearMask & GL_COLOR_BUFFER_BIT)) {
                m_gl->clearColor(0, 0, 0, 0);
            }

            if (m_attributes.depth()) {
                if (!(m_pendingClearMask & GL_DEPTH_BUFFER_BIT)) {
                    m_gl->clearDepthf(1.0);
                }
                mask |= GL_DEPTH_BUFFER_BIT;
            }

            if (m_attributes.stencil()) {
                if (!(m_pendingClearMask & GL_STENCIL_BUFFER_BIT)) {
                    m_gl->clearStencil(0);
                }
                mask |= GL_STENCIL_BUFFER_BIT;
            }

            if (mask != 0) {
                m_gl->clear(mask);
            }
        }
        m_hasPendingJobsBetweenFrames = false;
    }
}

void WebGLRenderingContext::setPendingClearMask(uint32_t mask)
{
    m_pendingClearMask |= mask;
}

Nullable<ScriptObject> WebGLRenderingContext::getExtension(
    String* requestedName)
{
    ENTER_CONTEXT_SCOPE(Nullable<ScriptObject>());

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
        WebGLExtensionRegistry::instance().getGenerator(name);

    if (!maybeGenerator.hasValue()) {
        return Nullable<ScriptObject>();
    }

    ScriptObject object = maybeGenerator.value()(scriptBindingInstance(), this);
    m_enabledExtensions.insert({ name, object });
    return object;
}

void WebGLRenderingContext::activeTexture(GLenum texture)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->activeTexture(texture);
}

void WebGLRenderingContext::attachShader(WebGLProgram* program,
                                         WebGLShader* shader)
{
    if (!checkWebGLObject(program) || !checkWebGLObject(shader)) {
        return;
    }

    ENTER_CONTEXT_SCOPE();

    m_gl->attachShader(program->glObject(), shader->glObject());
}

void WebGLRenderingContext::bindAttribLocation(WebGLProgram* program,
                                               GLuint index, String* name)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(program)) {
        return;
    }

    if (!checkAttribOrUniformName(name)) {
        return;
    }

    m_gl->bindAttribLocation(program->glObject(), index, CSTR(name));
}

void WebGLRenderingContext::bindBuffer(GLenum target,
                                       Nullable<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

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
    GLenum target, Nullable<WebGLFramebuffer*> maybeFramebuffer)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeFramebuffer.hasValue()) {
        WebGLFramebuffer* frameBuffer = maybeFramebuffer.value();

        if (!checkWebGLObject(frameBuffer)) {
            return;
        }

        if (frameBuffer->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (target != GL_FRAMEBUFFER) {
            // NOTE: how to handle this case is not found in the specification.
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
    GLenum target, Nullable<WebGLRenderbuffer*> maybeRenderbuffer)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeRenderbuffer.hasValue()) {
        WebGLRenderbuffer* renderBuffer = maybeRenderbuffer.value();

        if (!checkWebGLObject(renderBuffer)) {
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
    } else {
        m_gl->bindRenderbuffer(target, 0);
    }
}

void WebGLRenderingContext::bindTexture(GLenum target,
                                        Nullable<WebGLTexture*> maybeTexture)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeTexture.hasValue()) {
        WebGLTexture* texture = maybeTexture.value();

        if (!checkWebGLObject(texture)) {
            return;
        }

        if (texture->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        m_gl->bindTexture(target, texture->glObject());
        m_boundTextures[target] = texture->glObject();
    } else {
        m_gl->bindTexture(target, 0);
        m_boundTextures.erase(target);
    }
}

void WebGLRenderingContext::blendColor(GLclampf red, GLclampf green,
                                       GLclampf blue, GLclampf alpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->blendColor(red, green, blue, alpha);
}

void WebGLRenderingContext::blendEquation(GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->blendEquation(mode);
}

void WebGLRenderingContext::blendEquationSeparate(GLenum modeRGB,
                                                  GLenum modeAlpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->blendEquationSeparate(modeRGB, modeAlpha);
}

void WebGLRenderingContext::blendFunc(GLenum sfactor, GLenum dfactor)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->blendFunc(sfactor, dfactor);
}

void WebGLRenderingContext::blendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
                                              GLenum srcAlpha, GLenum dstAlpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GLenum WebGLRenderingContext::checkFramebufferStatus(GLenum target)
{
    ENTER_CONTEXT_SCOPE(GL_FRAMEBUFFER_UNSUPPORTED);

    if (m_isContextLost) {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }

    return m_gl->checkFramebufferStatus(target);
}

void WebGLRenderingContext::clear(uint32_t mask)
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    m_gl->clear(mask);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::clearColor(float red, float green, float blue,
                                       float alpha)
{
    ENTER_CONTEXT_SCOPE();

    setPendingClearMask(GL_COLOR_BUFFER_BIT);

    m_gl->clearColor(red, green, blue, alpha);

    TRACE(WEBGL_V, KV(red), KV(green), KV(blue), KV(alpha));
}

void WebGLRenderingContext::clearDepth(GLclampf depth)
{
    ENTER_CONTEXT_SCOPE();

    if (!m_attributes.depth()) {
        return;
    }

    setPendingClearMask(GL_DEPTH_BUFFER_BIT);

    m_gl->clearDepthf(depth);

    TRACE(WEBGL_V, KV(depth));
}

void WebGLRenderingContext::clearStencil(GLint s)
{
    ENTER_CONTEXT_SCOPE();

    if (!m_attributes.stencil()) {
        return;
    }

    setPendingClearMask(GL_STENCIL_BUFFER_BIT);

    m_gl->clearStencil(s);
    TRACE(WEBGL_V, KV(s));
}

void WebGLRenderingContext::colorMask(GLboolean red, GLboolean green,
                                      GLboolean blue, GLboolean alpha)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->colorMask(red, green, blue, alpha);
}

void WebGLRenderingContext::compileShader(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(shader)) {
        return;
    }

    m_gl->compileShader(shader->glObject());
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

#define IMPLEMENT_DELETE_BUFFERS(Name, Deleter)                            \
    void WebGLRenderingContext::delete##Name(Nullable<WebGL##Name*> maybe) \
    {                                                                      \
        ENTER_CONTEXT_SCOPE();                                             \
        if (maybe.hasValue()) {                                            \
            WebGL##Name* value = maybe.value();                            \
            if (!checkWebGLObject(value) || value->isDeleted()) {          \
                return;                                                    \
            }                                                              \
            GLuint buffer = value->glObject();                             \
            Deleter(1, &buffer);                                           \
            value->markDeleted();                                          \
        }                                                                  \
    }

IMPLEMENT_DELETE_BUFFERS(Buffer, m_gl->deleteBuffers);
IMPLEMENT_DELETE_BUFFERS(Framebuffer, m_gl->deleteFramebuffers);
IMPLEMENT_DELETE_BUFFERS(Renderbuffer, m_gl->deleteRenderbuffers);
IMPLEMENT_DELETE_BUFFERS(Texture, m_gl->deleteTextures);
#undef IMPLEMENT_DELETE_BUFFERS

#define IMPLEMENT_DELETE_OBJECT(Name, Deleter)                             \
    void WebGLRenderingContext::delete##Name(Nullable<WebGL##Name*> maybe) \
    {                                                                      \
        ENTER_CONTEXT_SCOPE();                                             \
        if (maybe.hasValue()) {                                            \
            WebGL##Name* value = maybe.value();                            \
            if (!checkWebGLObject(value) || value->isDeleted()) {          \
                return;                                                    \
            }                                                              \
            Deleter(value->glObject());                                    \
            value->markDeleted();                                          \
        }                                                                  \
    }

IMPLEMENT_DELETE_OBJECT(Program, m_gl->deleteProgram);
IMPLEMENT_DELETE_OBJECT(Shader, m_gl->deleteShader);
#undef IMPLEMENT_DELETE_OBJECT

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

    m_gl->depthRangef(zNear, zFar);
}

void WebGLRenderingContext::detachShader(WebGLProgram* program,
                                         WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    STARFISH_ASSERT(program != nullptr);
    STARFISH_ASSERT(shader != nullptr);

    m_gl->detachShader(program->glObject(), shader->glObject());
}

void WebGLRenderingContext::disable(GLenum cap)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->disable(cap);
}

void WebGLRenderingContext::disableVertexAttribArray(GLuint index)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->disableVertexAttribArray(index);
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
    }

    m_gl->drawArrays(mode, first, count);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::drawElements(GLenum mode, GLsizei count,
                                         GLenum type, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    if (count > 0) {
        // TODO: verify a non-null WebGLBuffer is bound to the
        // ELEMENT_ARRAY_BUFFER binding point if count is greater than zero. If
        // not, an INVALID_OPERATION error will be generated.
    }

    if (offset < 0) {
        // the offset must be non-negative or an INVALID_VALUE error will be
        // generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!getCurrentProgram()) {
        // If the CURRENT_PROGRAM is null, an INVALID_OPERATION error will be
        // generated.
        setGLError(GL_INVALID_OPERATION);
    }

    m_gl->drawElements(mode, count, type, reinterpret_cast<void*>(offset));
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::enable(GLenum cap)
{
    ENTER_CONTEXT_SCOPE();

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
}

void WebGLRenderingContext::finish()
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    m_gl->finish();
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::flushWebGL()
{
    ENTER_CONTEXT_SCOPE();

    completePendingJobs();

    m_gl->flush();
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::framebufferRenderbuffer(
    GLenum target, GLenum attachment, GLenum renderbuffertarget,
    Nullable<WebGLRenderbuffer*> maybeRenderbuffer)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeRenderbuffer.hasValue()) {
        WebGLRenderbuffer* renderBuffer = maybeRenderbuffer.value();

        if (!checkWebGLObject(renderBuffer)) {
            return;
        }

        m_gl->framebufferRenderbuffer(target, attachment, renderbuffertarget,
                                      renderBuffer->glObject());
    } else {
        m_gl->framebufferRenderbuffer(target, attachment, renderbuffertarget,
                                      0);
        if (isDefaultFramebufferBound()) {
            setGLError(GL_INVALID_OPERATION);
        }
    }
}

void WebGLRenderingContext::framebufferTexture2D(
    GLenum target, GLenum attachment, GLenum textarget,
    Nullable<WebGLTexture*> maybeTexture, GLint level)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeTexture.hasValue()) {
        WebGLTexture* texture = maybeTexture.value();

        if (!checkWebGLObject(texture)) {
            return;
        }

        if (texture->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        GLuint textureId = texture->glObject();
        m_gl->framebufferTexture2D(target, attachment, textarget, textureId,
                                   level);
    } else {
        m_gl->framebufferTexture2D(target, attachment, textarget, 0, level);
        if (isDefaultFramebufferBound()) {
            setGLError(GL_INVALID_OPERATION);
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

    m_gl->generateMipmap(target);
}

void WebGLRenderingContext::shaderSource(WebGLShader* shader, String* source)
{
    ENTER_CONTEXT_SCOPE();

    std::string str = source->toUTF8NonGCString();
    const char* sourceArray[1] = { str.c_str() };

    m_gl->shaderSource(shader->glObject(), 1, sourceArray, nullptr);
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

    GLint value = -1;
    m_gl->getBufferParameteriv(target, pname, &value);

    if (hasGLError()) {
        // GL_INVALID_ENUM is generated in glGetBufferParameteriv if target or
        // pname is not an accepted value.
        return scriptNull();
    }

    return ValueRef::create(
        pname == GL_BUFFER_SIZE ? value : static_cast<GLenum>(value));
}

ScriptValue WebGLRenderingContext::getParameter(GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_DEPTH_BITS:
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
    case GL_STENCIL_BITS:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_REF:
    case GL_SUBPIXEL_BITS:
    case GL_UNPACK_ALIGNMENT: {
        std::vector<int> values(1);
        m_gl->getIntegerv(pname, &values[0]);
        return ValueRef::create(values[0]);
    }
    case GL_CURRENT_PROGRAM: {
        GLint value = -1;
        m_gl->getIntegerv(pname, &value);

        Nullable<WebGLProgram*> maybe = m_state->webGLProgram();
        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }
        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
    case GL_FRAMEBUFFER_BINDING: {
        GLint value = -1;
        m_gl->getIntegerv(pname, &value);

        if (isDefaultFramebufferBound()) {
            return scriptNull();
        }

        Nullable<WebGLFramebuffer*> maybe = m_state->webGLFramebuffer();
        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }
        TRACE(WEBGL, KV(maybe.value()->glObject()), KV(value));
        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
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

        Nullable<WebGLVertexArrayObjectOES*> maybe =
            m_state->webGLVertexArrayObjectOES();

        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }

        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
    case GL_ARRAY_BUFFER_BINDING:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING: {
        GLuint target = (pname == GL_ARRAY_BUFFER_BINDING)
                            ? GL_ARRAY_BUFFER
                            : GL_ELEMENT_ARRAY_BUFFER_BINDING;
        WebGLBuffer* buffer = m_state->getBoundBuffer(target).valueOr(nullptr);
        return buffer ? buffer->scriptValue() : scriptNull();
    }
    case kIMPLEMENTATION_COLOR_READ_TYPE: {
        // Our implementation-chosen is a combination of RGBA and UNSIGNED_BYTE.
        return ValueRef::create(GL_UNSIGNED_BYTE);
    }
    case kIMPLEMENTATION_COLOR_READ_FORMAT: {
        return ValueRef::create(GL_RGBA);
    }
    case GL_RENDERER:
    case GL_SHADING_LANGUAGE_VERSION:
    case GL_VERSION:
    case GL_VENDOR: {
        const std::string output =
            reinterpret_cast<const char*>(glGetString(pname));
        return StringRef::createFromASCII(output.c_str(), output.length());
    }
    case GL_MAX_VIEWPORT_DIMS: {
        std::vector<int> values(2);
        m_gl->getIntegerv(pname, &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT: {
        std::vector<int> values(4);
        m_gl->getIntegerv(pname, &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    default:
        STARFISH_UNSUPPORTED("unsupported pname: 0x%04X(%s)", pname,
                             __PRETTY_FUNCTION__);
        return scriptNull();
    }
    return scriptNull();
}

WebGLActiveInfo* WebGLRenderingContext::getActiveAttrib(WebGLProgram* program,
                                                        GLuint index)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLint maxNameLength;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                       &maxNameLength);

    GLint size;
    GLenum type;
    GLsizei length;

    std::vector<char> name;
    name.resize(maxNameLength, '\0');
    m_gl->getActiveAttrib(program->glObject(), index, maxNameLength, &length,
                          &size, &type, &name[0]);

    if (hasGLError()) {
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

    GLint maxNameLength;
    m_gl->getProgramiv(program->glObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH,
                       &maxNameLength);

    GLint size;
    GLenum type;
    GLsizei length;

    std::vector<char> name;
    name.resize(maxNameLength, '\0');
    m_gl->getActiveUniform(program->glObject(), index, maxNameLength, &length,
                           &size, &type, &name[0]);

    if (hasGLError()) {
        // a) If the passed index is out of range, generates an INVALID_VALUE
        // error and returns null. b) Returns null if any OpenGL errors are
        // generated during the execution of this function.
        return nullptr;
    }

    return new WebGLActiveInfo(scriptBindingInstance(), size, type,
                               String::createASCIIString(name.data(), length));
}

GLint WebGLRenderingContext::getAttribLocation(WebGLProgram* program,
                                               String* name)
{
    ENTER_CONTEXT_SCOPE(-1);

    if (!checkWebGLObject(program)) {
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

ScriptValue WebGLRenderingContext::getProgramParameter(WebGLProgram* program,
                                                       GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!checkWebGLObject(program)) {
        return scriptNull();
    }

    GLint params = 0;
    m_gl->getProgramiv(program->glObject(), pname, &params);

    if (hasGLError()) {
        /*
         - GL_INVALID_ENUM if pname is not an accepted value.
         - GL_INVALID_VALUE if program is not a value generated by OpenGL.
         - GL_INVALID_OPERATION if program does not refer to a program object.
        */
        return scriptNull();
    }

    switch (pname) {
    case GL_DELETE_STATUS:
    case GL_LINK_STATUS:
    case GL_VALIDATE_STATUS:
        return Escargot::ValueRef::create(static_cast<bool>(params));
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
        return Escargot::ValueRef::create(static_cast<GLint>(params));
    default:
        break;
    }
    return scriptNull();
}

String* WebGLRenderingContext::getProgramInfoLog(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    m_gl->getProgramiv(program->glObject(), GL_INFO_LOG_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    m_gl->getProgramInfoLog(program->glObject(), bufferSize, &length,
                            &buffer[0]);

    if (hasGLError()) {
        return nullptr;
    }

    TRACE(WEBGL, buffer);

    return String::fromUTF8(buffer.data(), length);
}

ScriptValue WebGLRenderingContext::getShaderParameter(WebGLShader* shader,
                                                      GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!checkWebGLObject(shader)) {
        return scriptNull();
    }

    GLint params = 0;
    m_gl->getShaderiv(shader->glObject(), pname, &params);

    if (hasGLError()) {
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
    case GL_COMPILE_STATUS:
        return Escargot::ValueRef::create(static_cast<bool>(params));
    default:
        break;
    }
    return scriptNull();
}

String* WebGLRenderingContext::getShaderInfoLog(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    m_gl->getShaderiv(shader->glObject(), GL_INFO_LOG_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    m_gl->getShaderInfoLog(shader->glObject(), bufferSize, &length, &buffer[0]);

    if (hasGLError()) {
        return nullptr;
    }

    TRACE(WEBGL, buffer);

    return String::fromUTF8(buffer.data(), length);
}

String* WebGLRenderingContext::getShaderSource(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    m_gl->getShaderiv(shader->glObject(), GL_SHADER_SOURCE_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    m_gl->getShaderSource(shader->glObject(), bufferSize, &length, &buffer[0]);

    if (hasGLError()) {
        return nullptr;
    }

    return String::fromUTF8(buffer.data(), length);
}

ScriptValue WebGLRenderingContext::getUniform(WebGLProgram* program,
                                              WebGLUniformLocation* location)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    // TODO: rename checkWebGLObject to isFromCurrentContext
    // TODO: move setting error from checkWebGLObject
    if (!checkWebGLObject(program)) {
        return nullptr;
    }

    if (location->program()->context() != this) {
        setGLError(GL_INVALID_OPERATION);
        return nullptr;
    }

    STARFISH_UNIMPLEMENTED(
        "TODO: Return the uniform value at the passed location in the passed "
        "program.");

    return nullptr;
}

WebGLUniformLocation* WebGLRenderingContext::getUniformLocation(
    WebGLProgram* program, String* name)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!checkWebGLObject(program)) {
        return nullptr;
    }

    if (!checkAttribOrUniformName(name)) {
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

    if (hasGLError()) {
        // Returns null if any OpenGL errors are generated during the execution
        // of this function.
        return nullptr;
    }

    return new WebGLUniformLocation(scriptBindingInstance(), program, location);
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

        Nullable<WebGLVertexArrayObjectOES*> maybe =
            m_state->webGLVertexArrayObjectOES();

        if (!maybe.hasValue()) {
            return scriptNull(); // No mention found for this in the spec.
        }

        Nullable<WebGLBuffer*> maybeBuffer =
            m_state->getBufferBoundToVertexAttributes(index);

        if (!maybeBuffer.hasValue()) {
            return scriptNull(); // No mention found for this in the spec.
        }

        TRACE(WEBGL, KV(index), KV(maybeBuffer.value()->glObject()));

        STARFISH_ASSERT(index == maybeBuffer.value()->glObject());

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
    default:
        setGLError(GL_INVALID_ENUM);
        break;
    }
    return scriptNull();
}

GLintptr WebGLRenderingContext::getVertexAttribOffset(GLuint index,
                                                      GLenum pname)
{
    ENTER_CONTEXT_SCOPE(0);

    GLvoid* pointer = nullptr;
    m_gl->getVertexAttribPointerv(index, pname, &pointer);
    return reinterpret_cast<GLintptr>(pointer);
}

void WebGLRenderingContext::hint(GLenum target, GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->hint(target, mode);
}

bool WebGLRenderingContext::isBuffer(Nullable<WebGLBuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    // The generated bindings do CHECK_TYPEOF the value before calling this
    // function, so we don't have to do any type checking. Other `isXXXX`
    // functions do the same.
    return true;
}

bool WebGLRenderingContext::isEnabled(GLenum cap)
{
    ENTER_CONTEXT_SCOPE(false);

    if (m_isContextLost) {
        return false;
    }

    return m_gl->isEnabled(cap);
}

bool WebGLRenderingContext::isFramebuffer(Nullable<WebGLFramebuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return true;
}

bool WebGLRenderingContext::isProgram(Nullable<WebGLProgram*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return true;
}

bool WebGLRenderingContext::isRenderbuffer(Nullable<WebGLRenderbuffer*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return true;
}

bool WebGLRenderingContext::isShader(Nullable<WebGLShader*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return true;
}

bool WebGLRenderingContext::isTexture(Nullable<WebGLTexture*> maybe)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!maybe.hasValue() || !isFromCurrentContext(maybe.value()) ||
        maybe.value()->invalidated()) {
        return false;
    }

    return true;
}

void WebGLRenderingContext::lineWidth(GLfloat width)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->lineWidth(width);
}

void WebGLRenderingContext::linkProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(program)) {
        return;
    }

    /*
        NOTE: No idea to handle the following for now. It may already be handled
        in GLES3: 6.26 Packing Restrictions for Uniforms and Varyings: The WebGL
        API further requires that if the packing algorithm fails either for the
        uniform variables of a shader or for the varying variables of a program,
        compilation or linking must fail.
    */

    m_gl->linkProgram(program->glObject());

    if (hasGLError()) {
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
        STARFISH_UNIMPLEMENTED();
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
    default:
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

    m_gl->renderbufferStorage(target, internalformat, width, height);
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
    ENTER_CONTEXT_SCOPE();

    if (m_boundTextures.find(target) == m_boundTextures.end()) {
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

    if (m_boundTextures.find(target) == m_boundTextures.end()) {
        // If an attempt is made to call this function with no WebGLTexture
        // bound, an INVALID_OPERATION error is generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    m_gl->texParameteri(target, pname, param);
}

void WebGLRenderingContext::uniform1f(
    Nullable<WebGLUniformLocation*> maybeUniform, GLfloat x)
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
    Nullable<WebGLUniformLocation*> maybeUniform, GLfloat x, GLfloat y)
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
    Nullable<WebGLUniformLocation*> maybeUniform, GLfloat x, GLfloat y,
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
    Nullable<WebGLUniformLocation*> maybeUniform, GLfloat x, GLfloat y,
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
    Nullable<WebGLUniformLocation*> maybeUniform, GLint x)
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

    m_gl->uniform1i(uniform->location(), x);
}

void WebGLRenderingContext::uniform2i(
    Nullable<WebGLUniformLocation*> maybeUniform, GLint x, GLint y)
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
    Nullable<WebGLUniformLocation*> maybeUniform, GLint x, GLint y, GLint z)
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
    Nullable<WebGLUniformLocation*> maybeUniform, GLint x, GLint y, GLint z,
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

void WebGLRenderingContext::useProgram(Nullable<WebGLProgram*> maybeProgram)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeProgram.hasValue()) {
        WebGLProgram* program = maybeProgram.value();
        if (!checkWebGLObject(program)) {
            return;
        }
        m_gl->useProgram(program->glObject());
        m_state->setWebGLProgram(program);
    } else {
        m_gl->useProgram(0);
        m_state->setWebGLProgram(nullptr);
    }
}

void WebGLRenderingContext::validateProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    STARFISH_ASSERT(program != nullptr);

    // If program was generated by a different WebGLRenderingContext than this
    // one, generates an INVALID_OPERATION error.
    m_gl->validateProgram(program->glObject());
}

void WebGLRenderingContext::vertexAttrib1f(GLuint index, GLfloat x)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib1f(index, x);
}

void WebGLRenderingContext::vertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib2f(index, x, y);
}

void WebGLRenderingContext::vertexAttrib3f(GLuint index, GLfloat x, GLfloat y,
                                           GLfloat z)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib3f(index, x, y, z);
}

void WebGLRenderingContext::vertexAttrib4f(GLuint index, GLfloat x, GLfloat y,
                                           GLfloat z, GLfloat w)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->vertexAttrib4f(index, x, y, z, w);
}

#define IMPLEMENT_VERTEX_ATTRIB_NFV(N)                                      \
    void WebGLRenderingContext::vertexAttrib##N##fv(GLuint index,           \
                                                    Float32List variant)    \
    {                                                                       \
        ENTER_CONTEXT_SCOPE();                                              \
        if (variant.isFloat32ArrayValue()) {                                \
            Float32ArrayObjectRef* values = variant.getFloat32ArrayValue(); \
            const size_t arrayLength = values->arrayLength();               \
            uint8_t* rawBuffer = const_cast<uint8_t*>(values->rawBuffer()); \
            if (arrayLength > 0) {                                          \
                m_gl->vertexAttrib##N##fv(                                  \
                    index, reinterpret_cast<GLfloat*>(rawBuffer));          \
            }                                                               \
        } else {                                                            \
            STARFISH_ASSERT(variant.isSequenceOfGLfloatValue());            \
            /* Due to the memory size of double and float types, the raw    \
             * buffer returned by GCAtomicVector<double>::data() cannot be  \
             * used directly. Reconstructs a float buffer including values  \
             * converted from double. */                                    \
            const GCAtomicVector<double> v =                                \
                variant.getSequenceOfGLfloatValue();                        \
            std::vector<GLfloat> vector;                                    \
            vector.reserve(v.size());                                       \
            for (const double& value : v) {                                 \
                vector.push_back(static_cast<GLfloat>(value));              \
            }                                                               \
            if (!vector.empty()) {                                          \
                m_gl->vertexAttrib##N##fv(index, vector.data());            \
            }                                                               \
        }                                                                   \
    }

IMPLEMENT_VERTEX_ATTRIB_NFV(1)
IMPLEMENT_VERTEX_ATTRIB_NFV(2)
IMPLEMENT_VERTEX_ATTRIB_NFV(3)
IMPLEMENT_VERTEX_ATTRIB_NFV(4)

#undef IMPLEMENT_VERTEX_ATTRIB_NFV

void WebGLRenderingContext::vertexAttribPointer(GLuint index, GLint size,
                                                GLenum type,
                                                GLboolean normalized,
                                                GLsizei stride, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (stride > kMaximumSupportedStride) {
        // In WebGL, the maximum supported stride is 255
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (offset < 0) {
        // see Buffer Offset and Stride Requirements. If offset is negative, an
        // INVALID_VALUE error will be generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    m_state->setBufferBoundToVertexAttributes(
        index, m_state->getBoundBuffer(GL_ARRAY_BUFFER));

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
                              reinterpret_cast<void*>(offset));
}

void WebGLRenderingContext::viewport(uint32_t x, uint32_t y, uint32_t width,
                                     uint32_t height)
{
    ENTER_CONTEXT_SCOPE();

    m_gl->viewport(x, y, width, height);
}

// WebGLRenderingContextOverloads

void WebGLRenderingContext::bufferData(GLenum target, GLsizeiptr size,
                                       GLenum usage)
{
    ENTER_CONTEXT_SCOPE();

    // Set the size of the currently bound WebGLBuffer object for the passed
    // target. The buffer is initialized to 0.
    m_gl->bufferData(target, size, nullptr, usage);
}

void WebGLRenderingContext::bufferData(GLenum target,
                                       Nullable<AllowSharedBufferSource> data,
                                       GLenum usage)
{
    ENTER_CONTEXT_SCOPE();

    if (!data.hasValue()) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (data.value().isArrayBufferValue()) {
        ScriptArrayBuffer buffer = data.value().getArrayBufferValue();
        m_gl->bufferData(target, buffer->byteLength(), buffer->rawBuffer(),
                         usage);
    } else if (data.value().isArrayBufferViewValue()) {
        ScriptArrayBufferView view = data.value().getArrayBufferViewValue();
        m_gl->bufferData(target, view->byteLength(), view->rawBuffer(), usage);
    } else if (data.value().isSharedArrayBufferValue()) {
        ScriptSharedArrayBuffer buffer =
            data.value().getSharedArrayBufferValue();
        m_gl->bufferData(target, buffer->byteLength(), buffer->rawBuffer(),
                         usage);
    } else {
        setGLError(GL_INVALID_VALUE);
    }
}

void WebGLRenderingContext::bufferSubData(GLenum target, GLintptr offset,
                                          AllowSharedBufferSource data)
{
    ENTER_CONTEXT_SCOPE();

    if (data.isArrayBufferValue()) {
        ScriptArrayBuffer buffer = data.getArrayBufferValue();
        m_gl->bufferSubData(target, offset, buffer->byteLength(),
                            buffer->rawBuffer());
    } else if (data.isArrayBufferViewValue()) {
        ScriptArrayBufferView view = data.getArrayBufferViewValue();
        m_gl->bufferSubData(target, offset, view->byteLength(),
                            view->rawBuffer());
    } else if (data.isSharedArrayBufferValue()) {
        ScriptSharedArrayBuffer buffer = data.getSharedArrayBufferValue();
        m_gl->bufferSubData(target, offset, buffer->byteLength(),
                            buffer->rawBuffer());
    } else {
        setGLError(GL_INVALID_VALUE);
    }
}

class TexImageHelper final {
public:
    // NOTE: Better to use common utilities for image manipulation. Canvas
    // is not possible due to its WebView dependency.
    struct ImageData {
        ImageData() = default;
        size_t width = 0;
        size_t height = 0;
        size_t stride = 0;
        GLenum format = 0;
        unsigned char* data = nullptr;
    };

    TexImageHelper(size_t width, size_t height, size_t stride, GLenum format,
                   void* data)
    {
        STARFISH_ASSERT(data != nullptr);

        m_sourceImage.width = width;
        m_sourceImage.height = height;
        m_sourceImage.stride = stride;
        m_sourceImage.format = format;
        m_sourceImage.data = static_cast<unsigned char*>(data);
        m_isNativeImageDataUsed = false;
    }

    TexImageHelper(NativeImageData* imageData, GLenum format)
    {
        STARFISH_ASSERT(imageData != nullptr);

        m_sourceImage.width = imageData->width();
        m_sourceImage.height = imageData->height();
        m_sourceImage.stride = imageData->stride();
        m_sourceImage.format = format;
        m_sourceImage.data = static_cast<unsigned char*>(imageData->data());
        m_isNativeImageDataUsed = true;
    }

    ~TexImageHelper()
    {
    }

    void draw(const bool needsFlipY, const bool needsPremultiplyAlpha)
    {
        const size_t width = m_sourceImage.width;
        const size_t height = m_sourceImage.height;
        const size_t stride = m_sourceImage.stride;
        const unsigned char* image = m_sourceImage.data;

        size_t offset = 0, newOffset = 0, srcOffset = 0, destOffset = 0;

        if (m_sourceImage.format != GL_RGB && m_sourceImage.format != GL_RGBA) {
            return;
        }

#if !defined(PORT_PIXEL_ORDER_RGBA) && !defined(PORT_PIXEL_ORDER_BGRA)
        STARFISH_ASSERT_NOT_REACHED();
#endif

        bool needsColorConversion = false;

#if defined(PORT_PIXEL_ORDER_BGRA)
        if (m_isNativeImageDataUsed) {
            // NativeImageData is formatted as BGRA.
            if (WebGLExtensionRegistry::instance()
                    .hasEXT_texture_format_BGRA8888()) {
                m_dataFormat = GL_BGRA_EXT;
                needsColorConversion = false;
            } else {
                needsColorConversion = true;
            }
        }
#endif
        if (!needsFlipY && !needsPremultiplyAlpha && !needsColorConversion) {
            return;
        }

        m_data.resize(height * stride);

        std::vector<uint8_t> order;

        if (needsColorConversion) {
            order = { 2, 1, 0, 3 };
        } else {
            order = { 0, 1, 2, 3 };
        }

        for (size_t row = 0; row < height; row++) {
            // Calculate the memory offset for the current row
            newOffset = offset = row * stride;

            // NOTE: For increasing more performance of this feature, we may
            // consider using fragment shader.
            if (needsFlipY) {
                newOffset = (height - row - 1) * stride;
            }

            for (size_t column = 0; column < width; column++) {
                // Calculate the memory offset for the current pixel
                srcOffset = offset + column * 4;
                destOffset = newOffset + column * 4;

                if (needsPremultiplyAlpha) {
                    float alpha = image[srcOffset + order[3]] / 255.f;
                    m_data[destOffset + 0] =
                        multiplyAlpha(image[srcOffset + order[0]], alpha);
                    m_data[destOffset + 1] =
                        multiplyAlpha(image[srcOffset + order[1]], alpha);
                    m_data[destOffset + 2] =
                        multiplyAlpha(image[srcOffset + order[2]], alpha);
                    m_data[destOffset + 3] = image[srcOffset + order[3]];
                } else {
                    m_data[destOffset + 0] = image[srcOffset + order[0]];
                    m_data[destOffset + 1] = image[srcOffset + order[1]];
                    m_data[destOffset + 2] = image[srcOffset + order[2]];
                    m_data[destOffset + 3] = image[srcOffset + order[3]];
                }
            }
        }
    }

    const void* data()
    {
        return m_data.empty() ? m_sourceImage.data : m_data.data();
    }

    Optional<GLenum> dataFormat()
    {
        return m_dataFormat;
    }

private:
    unsigned char multiplyAlpha(unsigned char color, float alpha)
    {
        return ((color / 255.f) * alpha) * 255;
    }

    ImageData m_sourceImage;
    std::vector<unsigned char> m_data;
    bool m_isNativeImageDataUsed;
    Optional<GLenum> m_dataFormat;
};

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLsizei width,
                                       GLsizei height, GLint border,
                                       GLenum format, GLenum type,
                                       Nullable<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (m_boundTextures.find(target) == m_boundTextures.end() &&
        !isBoundCubeMapTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (static_cast<GLenum>(internalFormat) != format) {
        setGLError(GL_INVALID_OPERATION,
                   StringUtils::formatString(
                       "The given parameters, internal format (0x%0fX) and "
                       "format (0x%04X) are not same.",
                       internalFormat, format)
                       .c_str());
        return;
    }

    if (pixels.hasValue()) {
        ArrayBufferViewRef* pixelsView = pixels.getValue();

        if (type == GL_UNSIGNED_BYTE &&
            (!pixelsView->isUint8ArrayObject() &&
             !pixelsView->isUint8ClampedArrayObject())) {
            // If it is UNSIGNED_BYTE, a Uint8Array or Uint8ClampedArray
            // must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        } else if ((type == GL_UNSIGNED_SHORT_5_6_5 ||
                    type == GL_UNSIGNED_SHORT_4_4_4_4 ||
                    type == GL_UNSIGNED_SHORT_5_5_5_1) &&
                   !pixelsView->isUint16ArrayObject()) {
            // If it is UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, or
            // UNSIGNED_SHORT_5_5_5_1, a Uint16Array must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if ((type == GL_FLOAT) && !isExtensionEnabled("OES_texture_float")) {
            setGLError(GL_INVALID_ENUM);
            return;
        }

        // If pixels is non-null but its size is less than what is required by
        // the specified width, height, format, type, and pixel storage
        // parameters, generates an INVALID_OPERATION error.
        size_t bytesPerPixel = Pixel::getBytesPerPixel(format, type);
        size_t byteLengthOfPixels = width * height * bytesPerPixel;
        size_t byteLengthOfView = pixels->byteLength();

        TRACEF(WEBGL, "\n%s",
               StringUtils::createTableString(20, KV(hex(target)), KV(width),
                                              KV(height), KV(bytesPerPixel),
                                              KV(byteLengthOfPixels)));

        if (byteLengthOfView < byteLengthOfPixels) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        GLvoid* data = pixelsView->rawBuffer() + pixelsView->byteOffset();

        // Handle WebGL-specific pixel storage parameters that affect the
        // behavior of this function.
        TexImageHelper image(width, height, width * bytesPerPixel, format,
                             data);
        image.draw(m_unpackFlipY, m_unpackPremultiplyAlpha);

        m_gl->texImage2D(target, level, internalFormat, width, height, 0,
                         format, type, image.data());
    } else {
        m_gl->texImage2D(target, level, internalFormat, width, height, 0,
                         format, type, nullptr);
    }
}

void WebGLRenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                       GLsizei height, GLenum format,
                                       GLenum type,
                                       Nullable<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (pixels.hasValue()) {
        ArrayBufferViewRef* pixelsView = pixels.getValue();

        // 1. If the types don't match, an INVALID_OPERATION error is generated.
        if (type == GL_UNSIGNED_BYTE &&
            (!pixelsView->isUint8ArrayObject() &&
             !pixelsView->isUint8ClampedArrayObject())) {
            // If it is UNSIGNED_BYTE, a Uint8Array or Uint8ClampedArray
            // must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        } else if ((type == GL_UNSIGNED_SHORT_5_6_5 ||
                    type == GL_UNSIGNED_SHORT_4_4_4_4 ||
                    type == GL_UNSIGNED_SHORT_5_5_5_1) &&
                   !pixelsView->isUint16ArrayObject()) {
            // If it is UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, or
            // UNSIGNED_SHORT_5_5_5_1, a Uint16Array must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        } else if ((type == GL_FLOAT) && !pixelsView->isFloat32ArrayObject()) {
            // if it is FLOAT, a Float32Array must be supplied.
            setGLError(GL_INVALID_OPERATION);
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

        // NOTE: Our implementation-chosen is a combination of RGBA and
        // UNSIGNED_BYTE. See kIMPLEMENTATION_COLOR_READ_TYPE and
        // kIMPLEMENTATION_COLOR_READ_FORMAT.
        if (format != GL_RGBA && type != GL_UNSIGNED_BYTE) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        size_t bytesPerPixel = Pixel::getBytesPerPixel(format, type);
        size_t byteLengthOfPixels = width * height * bytesPerPixel;
        size_t byteLengthOfView = pixels->byteLength();

        TRACEF(WEBGL, "\n%s",
               StringUtils::createTableString(
                   20, KV(width), KV(height), KV(bytesPerPixel),
                   KV(byteLengthOfView), KV(byteLengthOfPixels)));

        if (byteLengthOfView < byteLengthOfPixels) {
            // If pixels is non-null, but is not large enough to retrieve all of
            // the pixels in the specified rectangle taking into account pixel
            // store modes, an INVALID_OPERATION error is generated.
            setGLError(GL_INVALID_OPERATION);
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

        GLvoid* data = pixelsView->rawBuffer() + pixelsView->byteOffset();

        // completePendingJobs() is not related as this function is a read
        // operation.
        m_gl->readPixels(x, y, width, height, format, type, data);
    } else {
        // If pixels is null, an INVALID_VALUE error is generated.
        setGLError(GL_INVALID_VALUE);
    }
}

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLenum format,
                                       GLenum type, TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    // TODO: handle DOM exception with referring to CanvasImageSource. If this
    // function is called with an HTMLImageElement or HTMLVideoElement whose
    // origin differs from the origin of the containing Document, or with an
    // HTMLCanvasElement, ImageBitmap or OffscreenCanvas whose bitmap's
    // origin-clean flag is set to false, a SECURITY_ERR exception must be
    // thrown. See Origin Restrictions.

    if (m_boundTextures.find(target) == m_boundTextures.end() &&
        !isBoundCubeMapTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (static_cast<GLenum>(internalFormat) != format) {
        // The format, in WebGL 1, must be the same as internalformat. See:
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/texImage2D
        // TODO: add an identifier for WebGL version and use it.
        setGLError(GL_INVALID_OPERATION,
                   StringUtils::formatString(
                       "The given parameters, internal format (0x%0fX) and "
                       "format (0x%04X) are not same.",
                       internalFormat, format)
                       .c_str());
        return;
    }

    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei stride = 0;
    NativeImageData* imageData = nullptr;

    if (source.isNoneValue()) {
        setGLError(GL_INVALID_VALUE);
        return;
    } else if (source.isImageBitmapValue()) {
        TRACE(WEBGL, "source.isImageBitmapValue");
        ImageBitmap* imageBitmap = source.getImageBitmapValue();
        imageData = imageBitmap->nativeImageData();
        width = imageData->width();
        height = imageData->height();
    } else if (source.isImageDataValue()) {
        STARFISH_UNIMPLEMENTED("ImageData");
    } else if (source.isHTMLImageElementValue()) {
        TRACE(WEBGL, "source.isHTMLImageElementValue");
        HTMLImageElement* element = source.getHTMLImageElementValue();
        imageData = element->imageData();
        if (imageData->isSVGNativeImageData()) {
            STARFISH_UNIMPLEMENTED("SVGNativeImageData");
            width = element->width();
            height = element->height();
        } else {
            width = imageData->width();
            height = imageData->height();
        }
    } else if (source.isHTMLCanvasElementValue()) {
        TRACE(WEBGL, "source.isHTMLCanvasElementValue");
        // TODO: Consider using CanvasImageSourceUtils::toNativeImageData
        HTMLCanvasElement* element = source.getHTMLCanvasElementValue();
        CanvasRenderingContext* context = element->canvasRenderingContext();
        STARFISH_ASSERT(context != nullptr);
        auto context2d = static_cast<CanvasRenderingContext2DMixIn*>(context);
        context2d->flush();
        imageData = NativeImageData::attach(context2d->canvas());
        width = element->width();
        height = element->height();
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (source.isHTMLVideoElementValue()) {
        STARFISH_UNIMPLEMENTED("HTMLVideoElement");
    }
#endif
    else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    STARFISH_ASSERT(imageData != nullptr);

    // Handle WebGL-specific pixel storage parameters that affect the behavior
    // of this function.
    TexImageHelper image(imageData, format);
    image.draw(m_unpackFlipY, m_unpackPremultiplyAlpha);

    // Uploads the given image data to the currently bound texture.
    m_gl->texImage2D(target, level, image.dataFormat().valueOr(internalFormat),
                     width, height, 0, image.dataFormat().valueOr(format), type,
                     image.data());
}

#define IMPLEMENT_UNIFORM_NFV(N, Suffix, SrcType)                           \
    void WebGLRenderingContext::uniform##N##Suffix(                         \
        WebGLUniformLocation* location, SrcType variant)                    \
    {                                                                       \
        ENTER_CONTEXT_SCOPE();                                              \
        if (location == nullptr) {                                          \
            return;                                                         \
        }                                                                   \
        if (!isFromCurrentProgram(location)) {                              \
            setGLError(GL_INVALID_OPERATION);                               \
            return;                                                         \
        }                                                                   \
        if (variant.isFloat32ArrayValue()) {                                \
            Float32ArrayObjectRef* values = variant.getFloat32ArrayValue(); \
            const size_t arrayLength = values->arrayLength();               \
            uint8_t* rawBuffer = const_cast<uint8_t*>(values->rawBuffer()); \
            if (arrayLength > 0) {                                          \
                /* count specifies the number of matrices. */               \
                m_gl->uniform##N##Suffix(                                   \
                    location->location(), arrayLength / N,                  \
                    reinterpret_cast<GLfloat*>(rawBuffer));                 \
            }                                                               \
        } else {                                                            \
            STARFISH_ASSERT(variant.isSequenceOfGLfloatValue());            \
            const GCAtomicVector<double> v =                                \
                variant.getSequenceOfGLfloatValue();                        \
            std::vector<GLfloat> vector;                                    \
            vector.reserve(v.size());                                       \
            for (const double& value : v) {                                 \
                vector.push_back(static_cast<GLfloat>(value));              \
            }                                                               \
            if (!vector.empty()) {                                          \
                m_gl->uniform##N##Suffix(location->location(),              \
                                         vector.size() / N, vector.data()); \
            }                                                               \
        }                                                                   \
    }

IMPLEMENT_UNIFORM_NFV(1, fv, Float32List)
IMPLEMENT_UNIFORM_NFV(2, fv, Float32List)
IMPLEMENT_UNIFORM_NFV(3, fv, Float32List)
IMPLEMENT_UNIFORM_NFV(4, fv, Float32List)

#undef IMPLEMENT_UNIFORM_NFV

#define IMPLEMENT_UNIFORM_NIV(N, Suffix, SrcType)                             \
    void WebGLRenderingContext::uniform##N##Suffix(                           \
        WebGLUniformLocation* location, SrcType variant)                      \
    {                                                                         \
        ENTER_CONTEXT_SCOPE();                                                \
        if (location == nullptr) {                                            \
            return;                                                           \
        }                                                                     \
        if (!isFromCurrentProgram(location)) {                                \
            setGLError(GL_INVALID_OPERATION);                                 \
            return;                                                           \
        }                                                                     \
        if (variant.isInt32ArrayValue()) {                                    \
            Int32ArrayObjectRef* values = variant.getInt32ArrayValue();       \
            const size_t arrayLength = values->arrayLength();                 \
            const uint8_t* rawBuffer = values->rawBuffer();                   \
            if (arrayLength > 0) {                                            \
                /* count specifies the number of matrices. */                 \
                m_gl->uniform##N##Suffix(location->location(),                \
                                         arrayLength / N, (GLint*)rawBuffer); \
            }                                                                 \
        } else {                                                              \
            STARFISH_ASSERT(variant.isSequenceOfGLintValue());                \
            GCAtomicVector<int32_t> vector =                                  \
                variant.getSequenceOfGLintValue();                            \
            if (!vector.empty()) {                                            \
                m_gl->uniform##N##Suffix(location->location(),                \
                                         vector.size() / N, vector.data());   \
            }                                                                 \
        }                                                                     \
    }

IMPLEMENT_UNIFORM_NIV(1, iv, Int32List)
IMPLEMENT_UNIFORM_NIV(2, iv, Int32List)
IMPLEMENT_UNIFORM_NIV(3, iv, Int32List)
IMPLEMENT_UNIFORM_NIV(4, iv, Int32List)

#undef IMPLEMENT_UNIFORM_NIV

#define IMPLEMENT_UNIFORM_MATRIX_NFV(N)                                        \
    void WebGLRenderingContext::uniformMatrix##N##fv(                          \
        WebGLUniformLocation* location, GLboolean transpose,                   \
        Float32List variant)                                                   \
    {                                                                          \
        ENTER_CONTEXT_SCOPE();                                                 \
        /* location is nullable. */                                            \
        if (location == nullptr) {                                             \
            return;                                                            \
        }                                                                      \
        if (!isFromCurrentProgram(location)) {                                 \
            setGLError(GL_INVALID_OPERATION);                                  \
            return;                                                            \
        }                                                                      \
        if (variant.isFloat32ArrayValue()) {                                   \
            Float32ArrayObjectRef* values = variant.getFloat32ArrayValue();    \
            const size_t arrayLength = values->arrayLength();                  \
            uint8_t* rawBuffer = const_cast<uint8_t*>(values->rawBuffer());    \
            if (arrayLength > 0) {                                             \
                /* count specifies the number of matrices. */                  \
                m_gl->uniformMatrix##N##fv(                                    \
                    location->location(), arrayLength / (N * N), transpose,    \
                    reinterpret_cast<GLfloat*>(rawBuffer));                    \
            }                                                                  \
        } else {                                                               \
            STARFISH_ASSERT(variant.isSequenceOfGLfloatValue());               \
            const GCAtomicVector<double> v =                                   \
                variant.getSequenceOfGLfloatValue();                           \
            std::vector<GLfloat> vector;                                       \
            vector.reserve(v.size());                                          \
            for (const double& value : v) {                                    \
                vector.push_back(static_cast<GLfloat>(value));                 \
            }                                                                  \
            if (!vector.empty()) {                                             \
                /* count specifies the number of matrices. */                  \
                m_gl->uniformMatrix##N##fv(location->location(),               \
                                           vector.size() / (N * N), transpose, \
                                           vector.data());                     \
            }                                                                  \
        }                                                                      \
    }

IMPLEMENT_UNIFORM_MATRIX_NFV(2)
IMPLEMENT_UNIFORM_MATRIX_NFV(3)
IMPLEMENT_UNIFORM_MATRIX_NFV(4)

#undef IMPLEMENT_UNIFORM_MATRIX_NFV

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

bool WebGLRenderingContext::checkWebGLObject(WebGLObject* object)
{
    if (object->context() != this) {
        // If object was generated by a different WebGLRenderingContext than
        // this one, generates an INVALID_OPERATION error.
        setGLError(GL_INVALID_OPERATION);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::isFromCurrentContext(WebGLObject* object)
{
    STARFISH_ASSERT(object != nullptr);

    return object->context() == this;
}

bool WebGLRenderingContext::checkAttribOrUniformName(String* name)
{
    // If the passed name is longer than the restriction defined in Maximum
    // Uniform and Attribute Location Lengths, generates an INVALID_VALUE error
    // and returns -1
    if (name->length() > kMaximumUniformAndAttributeLocationLengths) {
        setGLError(GL_INVALID_VALUE);
        return false;
    }

    // Returns -1 if name starts with one of the reserved WebGL prefixes per
    // GLSL Constructs.
    if (name->startsWith("webgl_") || name->startsWith("_webgl_")) {
        return false;
    }

    // TODO?: See Characters Outside the GLSL Source Character Set for
    // additional validation performed by WebGL implementations. WebGL
    // implementations generally must ensure that the shader source sent to a
    // GLSL driver only contains ASCII for safety.
    return true;
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
    // Consider caching this program id when useProgram is called.
    GLint program = getCurrentProgram();
    if (program == 0) {
        // no program object is active.
        return false;
    }

    if (location->program()->context() != this ||
        location->program()->glObject() != static_cast<GLuint>(program)) {
        // If program were generated by a different WebGLRenderingContext than
        // this one, and location were generated by a different program.
        return false;
    }
    return true;
}

GL* WebGLRenderingContext::gl()
{
    return m_gl;
}

} // namespace Starfish

#endif
