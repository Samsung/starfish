/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#ifndef __StarFishHTMLAudioElement__
#define __StarFishHTMLAudioElement__

#include "core/dom/HTMLMediaElement.h"

namespace StarFish {

class HTMLAudioElement : public HTMLMediaElement {
public:
    HTMLAudioElement(Document* document)
        : HTMLMediaElement(document)
    {
    }

    HTMLAudioElement(Document* document, String* src)
        : HTMLMediaElement(document)
    {
        setSrc(src);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLAudioElement() const override;

    virtual QualifiedName name();
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
