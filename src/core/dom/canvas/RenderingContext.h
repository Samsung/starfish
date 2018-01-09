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

#ifndef __StarFishRenderingContext__
#define __StarFishRenderingContext__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"

namespace StarFish {

class HTMLCanvasElement;

class RenderingContext : public ScriptWrappable {
public:
    RenderingContext(HTMLCanvasElement* canvasElement)
        : ScriptWrappable(this)
        , m_canvasElement(canvasElement)
    {
    }

    HTMLCanvasElement* canvas()
    {
        return m_canvasElement;
    }

    virtual ScriptBindingInstance* scriptBindingInstance();

protected:
    HTMLCanvasElement* m_canvasElement;
};
}

#endif
#endif
