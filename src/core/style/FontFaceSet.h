/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFontFaceSet__
#define __StarfishFontFaceSet__

#include "core/dom/EventTarget.h"

namespace Starfish {

class Document;
class Promise;
class Timer;

class FontFaceSet : public EventTarget {
public:
    enum class LoadStatus { Loading, Loaded };

    FontFaceSet(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isFontFaceSet() const;
    virtual ExecutionContext* executionContext() const override;

    String* status() const;
    Promise* ready();
    bool check(String* font, String* text = nullptr);

    void didStartFontLoading();
    void didFinishFontLoading();
    void checkReadyState();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(loading);
    DECLARE_EVENT_LISTENER(loadingdone);
    DECLARE_EVENT_LISTENER(loadingerror);
#undef VIRTUAL
#undef OVERRIDE

private:
    Document* m_document;
    LoadStatus m_status{ LoadStatus::Loaded };
    Promise* m_readyPromise{ nullptr };
    bool m_fulfilled{ false };
    uint32_t m_fallbackTimerId{ 0 };

    void fulfillReadyPromise();
};

} // namespace Starfish

#endif
