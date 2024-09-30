/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishShadowRoot__
#define __StarfishShadowRoot__

#include "core/dom/DocumentFragment.h"
#include "core/dom/ShadowRootInit.h"
#include "core/layout/Frame.h"

namespace Starfish {

class ShadowRoot : public DocumentFragment {
public:
    ShadowRoot(Document* document, ShadowRootMode mode)
        : DocumentFragment(document)
        , m_mode(mode)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isShadowRoot() const override;

    void clear()
    {
        while (firstChild()) {
            Frame* frame = firstChild()->frame();
            if (frame) {
                frame->parent()->removeChild(frame);
            }
            parserRemoveChild(firstChild());
        }
    }

    String* mode();

    bool isClosed() const
    {
        return m_mode == ShadowRootMode::Closed;
    }
    bool isOpened() const
    {
        return m_mode == ShadowRootMode::Open;
    }

private:
    ShadowRootMode m_mode;
};
} // namespace Starfish

#endif
