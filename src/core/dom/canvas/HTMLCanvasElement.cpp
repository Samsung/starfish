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
#include "binding/CanvasRenderingContext2DOrWebGLRenderingContextOrImageBitmapRenderingContextUnion.h"
#include "core/modules/canvas/Canvas.h"
#include "platform/canvas/image/ImageUtils.h"

namespace Starfish {

void HTMLCanvasElement::didAttributeChanged(QualifiedName name, String* old,
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
        setNeedsLayout();
    }
}

uint32_t HTMLCanvasElement::width()
{
    Nullable<String*> width =
        getAttribute(starfish()->staticStrings()->m_width);
    if (!width.hasValue()) {
        return STARFISH_CANVAS_DEFAULT_WIDTH;
    }
    return String::parseInt(width.getValue());
}

void HTMLCanvasElement::setWidth(uint32_t value)
{
    if (value >= 0) {
        setAttribute(starfish()->staticStrings()->m_width,
                     String::fromInt(value));
    }
}

uint32_t HTMLCanvasElement::height()
{
    Nullable<String*> height =
        getAttribute(starfish()->staticStrings()->m_height);
    if (!height.hasValue()) {
        return STARFISH_CANVAS_DEFAULT_HEIGHT;
    }
    return String::parseInt(height.getValue());
}

void HTMLCanvasElement::setHeight(uint32_t value)
{
    if (value >= 0) {
        setAttribute(starfish()->staticStrings()->m_height,
                     String::fromInt(value));
    }
}

Nullable<RenderingContextBindindingUnion> HTMLCanvasElement::getContext(
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
    } else if (contextId->equals("webgl")) {
        // NOT SUPPORT
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

    if (type->equals("image/png")) {
        m_canvasRenderingContext->flush();
        CanvasSurface* canvasSurface = m_canvasRenderingContext->surface();
        if (canvasSurface != nullptr) {
            auto width = canvasSurface->bufferWidth();
            auto height = canvasSurface->bufferHeight();
            size_t stride = 0;
            if (width && height) {
                stride = width / height;
            } else {
                stride = 4;
            }
            std::string result =
                "data:image/png;base64," +
                Base64Utils::encodeBase64(ImageUtils::encodePNG(
                    canvasSurface->mapBuffer(), width, height, stride));
            return String::fromUTF8(result.data(), result.size());
        }
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    return String::fromUTF8("data:,");
}
} // namespace Starfish

#endif
