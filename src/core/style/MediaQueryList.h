/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
