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

// https://drafts.csswg.org/css-font-loading/#fontfaceset-interface
// Only the CSS-connected fonts of the document are tracked; the setlike
// FontFace members are not implemented, so loadingdone/loadingerror are plain
// Events rather than FontFaceSetLoadEvent carrying the font faces.
class FontFaceSet : public EventTarget {
public:
    FontFaceSet(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isFontFaceSet() const;
    virtual ExecutionContext* executionContext() const override;

    String* status() const;
    Promise* ready();
    bool check(String* font, String* text);

    // Called when a web font starts or stops loading, and when the document
    // stops being pending on the environment.
    void didChangeFontLoadingState(bool fontLoadFailed = false);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(loading);
    DECLARE_EVENT_LISTENER(loadingdone);
    DECLARE_EVENT_LISTENER(loadingerror);
#undef VIRTUAL
#undef OVERRIDE

private:
    bool hasLoadingFonts() const;
    bool isPendingOnTheEnvironment() const;
    void fireEvent(String* type);

    Document* m_document;
    Promise* m_readyPromise{ nullptr };
    bool m_isLoading : 1;
    bool m_isReadyPromiseFulfilled : 1;
    bool m_hasFailedFonts : 1;
};

} // namespace Starfish

#endif
