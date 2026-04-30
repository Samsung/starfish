/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "binding/generated/CanvasRenderingContext2DOrWebGLRenderingContextOrWebGL2RenderingContextOrImageBitmapRenderingContextUnion.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageEncoder.h"
#include "core/dom/canvas/webgl/WebGLRenderingContext.h"
#include "core/dom/canvas/webgl/WebGL2RenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2D.h"
#include "core/dom/canvas/ImageBitmapRenderingContext.h"
#include "core/dom/DOMException.h"
#include "StaticStrings.h"
#include "core/dom/Document.h"

namespace Starfish {

void HTMLCanvasElement::didAttributeChanged(QualifiedName name,
                                            Optional<String*> old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_width == name || ss->m_height == name) {
        if (m_canvasRenderingContext) {
            m_canvasRenderingContext->onResize();
        }
        setNeedsStyleRecalc();
        setNeedsLayout();
    }
}

void HTMLCanvasElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues, matchedRules,
                                               cssCustomValues);

    // TODO: Since this patch, the width and height retrieved from the
    // attributes of the canvas element will be included in the calculation of
    // styles. Therefore, it is no longer necessary to directly call
    // setneedslayout in HTMLCanvasElement::didAttributeChanged. Also, in
    // FrameReplacedCanvas::intrinsicSize(), you should no longer refer to the
    // the canvas element's width and height directly.

    // Note: Ignore unit. The unit is always px.
    // width
    Optional<String*> maybeWidth =
        getAttribute(starfish()->staticStrings()->m_width);
    if (maybeWidth) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::Width);
        uint32_t width = String::parseInt(maybeWidth.getValue());
        pair.setLengthValue(CSSLength(width));
        cssValues.push_back(pair);
    }
    // height
    Optional<String*> maybeHeight =
        getAttribute(starfish()->staticStrings()->m_height);
    if (maybeHeight) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::Height);
        uint32_t height = String::parseInt(maybeHeight.getValue());
        pair.setLengthValue(CSSLength(height));
        cssValues.push_back(pair);
    }
}

uint32_t HTMLCanvasElement::width()
{
    Optional<String*> width =
        getAttribute(starfish()->staticStrings()->m_width);
    if (!width.hasValue()) {
        return STARFISH_CANVAS_DEFAULT_WIDTH;
    }
    return String::parseInt(width.getValue());
}

void HTMLCanvasElement::setWidth(uint32_t value)
{
    setAttribute(starfish()->staticStrings()->m_width, String::fromInt(value));
}

uint32_t HTMLCanvasElement::height()
{
    Optional<String*> height =
        getAttribute(starfish()->staticStrings()->m_height);
    if (!height.hasValue()) {
        return STARFISH_CANVAS_DEFAULT_HEIGHT;
    }
    return String::parseInt(height.getValue());
}

void HTMLCanvasElement::setHeight(uint32_t value)
{
    setAttribute(starfish()->staticStrings()->m_height, String::fromInt(value));
}

Optional<RenderingContextBindindingUnion> HTMLCanvasElement::getContext(
    String* contextId, GCVector<ScriptValue> arguments)
{
    if (contextId->equals("2d")) {
        if (m_contextMode == CanvasContextModeNone) {
            m_contextMode = CanvasContextMode2D;
            m_canvasRenderingContext = new CanvasRenderingContext2D(this);
            m_canvasRenderingContext->setOriginCleanFlag(true);
        }
        if (m_contextMode == CanvasContextMode2D) {
            return RenderingContextBindindingUnion::
                createCanvasRenderingContext2D(
                    (CanvasRenderingContext2D*)m_canvasRenderingContext);
        }
    } else if (contextId->equals("bitmaprenderer")) {
        if (m_contextMode == CanvasContextModeNone) {
            m_contextMode = CanvasContextModeBitmapRenderer;
            m_canvasRenderingContext = new ImageBitmapRenderingContext(this);
            m_canvasRenderingContext->setOriginCleanFlag(true);
        }
        if (m_contextMode == CanvasContextModeBitmapRenderer) {
            return RenderingContextBindindingUnion::
                createImageBitmapRenderingContext(
                    (ImageBitmapRenderingContext*)m_canvasRenderingContext);
        }
#if defined(STARFISH_ENABLE_WEBGL)
    } else if (contextId->equals("webgl") ||
               contextId->equals("experimental-webgl")) {
        if (m_contextMode == CanvasContextModeNone) {
            m_contextMode = CanvasContextModeWebGL;
            auto context = new WebGLRenderingContext(this);
            context->preInitialize(arguments.empty() ? scriptUndefined()
                                                     : arguments[0]);
            m_canvasRenderingContext = context;
            m_canvasRenderingContext->initialize();
            m_canvasRenderingContext->setOriginCleanFlag(true);
        }
        if (m_contextMode == CanvasContextModeWebGL) {
            return RenderingContextBindindingUnion::createWebGLRenderingContext(
                static_cast<WebGLRenderingContext*>(m_canvasRenderingContext));
        }
    } else if (contextId->equals("webgl2")) {
        if (m_contextMode == CanvasContextModeNone) {
            m_contextMode = CanvasContextModeWebGL2;
            auto context = new WebGL2RenderingContext(this);
            context->preInitialize(arguments.empty() ? scriptUndefined()
                                                     : arguments[0]);
            m_canvasRenderingContext = context;
            m_canvasRenderingContext->initialize();
            m_canvasRenderingContext->setOriginCleanFlag(true);
        }
        if (m_contextMode == CanvasContextModeWebGL2) {
            return RenderingContextBindindingUnion::
                createWebGL2RenderingContext(
                    static_cast<WebGL2RenderingContext*>(
                        m_canvasRenderingContext));
        }
#endif
    }
    return nullptr;
}

String* HTMLCanvasElement::toDataURL(String* type)
{
    STARFISH_ASSERT(type != nullptr);
    return toDataURL(type, createScriptValue(DefaultQuality));
}

String* HTMLCanvasElement::toDataURL(String* type, ScriptValue quality)
{
    STARFISH_ASSERT(type != nullptr);
    if (!m_canvasRenderingContext->originCleanFlag()) {
        throw new DOMException(executionContext(), DOMException::SECURITY_ERR);
    }

    std::string result = "data:,";
    m_canvasRenderingContext->flush();
    CanvasSurface* canvasSurface = m_canvasRenderingContext->surface();
    if (canvasSurface != nullptr) {
        auto width = canvasSurface->bufferWidth();
        auto height = canvasSurface->bufferHeight();
#if defined(PORT_PIXEL_ORDER_RGBA)
        ImageEncoder::ImageColorSpace colorSpace =
            ImageEncoder::ImageColorSpace::RGBA;
#else
        ImageEncoder::ImageColorSpace colorSpace =
            ImageEncoder::ImageColorSpace::BGRA;
#endif
#if !defined(STARFISH_HEADLESS)
        if (type->equals("image/png")) {
            result =
                "data:image/png;base64," +
                Base64Utils::encodeBase64(ImageEncoder::encodePNG(
                    canvasSurface->mapBuffer(), width, height, colorSpace));
        } else if (type->equals("image/jpeg")) {
            result =
                "data:image/jpeg;base64," +
                Base64Utils::encodeBase64(ImageEncoder::encodeJPEG(
                    canvasSurface->mapBuffer(), width, height, colorSpace));
        } else {
            STARFISH_UNSUPPORTED("Unsupported data type %s(%s)",
                                 type->toUTF8NonGCString().data(),
                                 __PRETTY_FUNCTION__);
        }
#endif
    }
    return String::fromUTF8(result.data(), result.size());
}

#ifdef STARFISH_ENABLE_TEST
void HTMLCanvasElement::dump(String* path)
{
    if (m_canvasRenderingContext &&
        m_canvasRenderingContext->isCanvasRenderingContext2D()) {
        m_canvasRenderingContext->asCanvasRenderingContext2D()->dump(path);
    }
}
#endif

} // namespace Starfish

#endif
