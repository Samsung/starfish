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

#ifndef __StarfishMediaQueryList__
#define __StarfishMediaQueryList__

#include "core/dom/EventTarget.h"

namespace Starfish {

class Document;
class MediaQueryEvaluator;
class MediaQueryListMatcher;
class MediaQuerySet;

class MediaQueryList : public EventTarget, public DocumentHoldable {
public:
    MediaQueryList(Document* document, MediaQueryListMatcher* matcher,
                   MediaQuerySet* media);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMediaQueryList() const override;

    virtual ExecutionContext* executionContext() override;

    /* DOM APIs */
    String* media() const;
    bool matches();
    void addListener(EventListener* listener);
    void removeListener(EventListener* listener);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(change);
#undef VIRTUAL
#undef OVERRIDE

private:
    MediaQueryListMatcher* m_matcher;
    MediaQuerySet* m_media;
    bool m_matches;
};

} /* namespace Starfish */

#endif /* __StarfishMediaQueryList__ */
