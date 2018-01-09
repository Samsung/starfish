/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
