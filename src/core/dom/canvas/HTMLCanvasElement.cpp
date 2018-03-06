/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "binding/CanvasRenderingContext2DOrWebGLRenderingContextOrImageBitmapRenderingContextUnion.h"

namespace StarFish {

QualifiedName HTMLCanvasElement::name()
{
    return starFish()->staticStrings()->m_canvasTagName;
}

uint32_t HTMLCanvasElement::width()
{
    Nullable<String*> width =
        getAttribute(starFish()->staticStrings()->m_width);
    if (!width.hasValue()) {
        return STARFISH_CANVAS_DEFAULT_WIDTH;
    }
    return String::parseInt(width.getValue());
}

void HTMLCanvasElement::setWidth(uint32_t value)
{
    if (value >= 0) {
        setAttribute(starFish()->staticStrings()->m_width,
                     String::fromInt(value));
    }
}

uint32_t HTMLCanvasElement::height()
{
    Nullable<String*> height =
        getAttribute(starFish()->staticStrings()->m_height);
    if (!height.hasValue()) {
        return STARFISH_CANVAS_DEFAULT_HEIGHT;
    }
    return String::parseInt(height.getValue());
}

void HTMLCanvasElement::setHeight(uint32_t value)
{
    if (value >= 0) {
        setAttribute(starFish()->staticStrings()->m_height,
                     String::fromInt(value));
    }
}

Nullable<RenderingContextBindindingUnion> HTMLCanvasElement::getContext(
    String* contextId, GCVector<ScriptValue> arguments)
{
    if (contextId->equals("2d")) {
        if (m_contextMode == CanvasContextModeNone) {
            m_contextMode = CanvasContextMode2D;
            m_renderingContext = new CanvasRenderingContext2D(this);
        }
        if (m_contextMode == CanvasContextMode2D) {
            return RenderingContextBindindingUnion::
                createCanvasRenderingContext2D(
                    (CanvasRenderingContext2D*)m_renderingContext);
        }
    } else if (contextId->equals("bitmaprenderer")) {
        if (m_contextMode == CanvasContextModeNone) {
            m_contextMode = CanvasContextModeBitmapRenderer;
            m_renderingContext = new ImageBitmapRenderingContext(this);
        }
        if (m_contextMode == CanvasContextModeBitmapRenderer) {
            return RenderingContextBindindingUnion::
                createImageBitmapRenderingContext(
                    (ImageBitmapRenderingContext*)m_renderingContext);
        }
    } else if (contextId->equals("webgl")) {
        // NOT SUPPORT
    }
    return nullptr;
}
}

#endif
