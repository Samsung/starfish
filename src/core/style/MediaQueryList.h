/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishMediaQueryList__
#define __StarFishMediaQueryList__

#include "binding/ScriptWrappable.h"
#include "core/dom/EventTarget.h"

namespace StarFish {

class Document;
class MediaQueryEvaluator;
class MediaQuerySet;

class MediaQueryList : public ScriptWrappable {
public:
    MediaQueryList(ScriptBindingInstance* instance, MediaQuerySet* media,
                   MediaQueryEvaluator* evaluator)
        : ScriptWrappable(this)
        , m_scriptBindingInstance(instance)
        , m_media(media)
        , m_evaluator(evaluator)
        , m_matches(false)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMediaQueryList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    /* DOM APIs */
    String* media() const;
    bool matches();
    void addListener(EventListener* listener);
    void removeListener(EventListener* listener);

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    MediaQuerySet* m_media;
    MediaQueryEvaluator* m_evaluator;
    bool m_matches;
};

} /* namespace StarFish */

#endif /* __StarFishMediaQueryList__ */
