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

#ifndef __StarfishFontFace__
#define __StarfishFontFace__

#include "binding/ScriptWrappable.h"
#include "core/style/WebFont.h"

namespace Starfish {

class Document;

// A CSS-connected face exposed by FontFaceSet.load(). Script-created faces
// and descriptor mutation are outside the CSS-connected loading path.
class FontFace : public ScriptWrappable {
public:
    FontFace(Document* document, const WebFont& font)
        : ScriptWrappable(this)
        , m_document(document)
        , m_font(font)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isFontFace() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    String* status();
    WebFont& font()
    {
        return m_font;
    }
    bool loadLocal();

private:
    Document* m_document;
    WebFont m_font;
    enum class LocalStatus { Unloaded, Loaded, Error };
    LocalStatus m_localStatus{ LocalStatus::Unloaded };
};

} // namespace Starfish

#endif
