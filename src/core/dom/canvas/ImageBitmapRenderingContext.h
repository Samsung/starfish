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

#ifndef __StarFishImageBitmapRenderingContext__
#define __StarFishImageBitmapRenderingContext__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/canvas/RenderingContext.h"

namespace StarFish {

class ImageBitmapRenderingContext : public RenderingContext {
public:
    ImageBitmapRenderingContext(HTMLCanvasElement* canvasElement)
        : RenderingContext(canvasElement)
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isImageBitmapRenderingContext() const override;
};
}

#endif
#endif
