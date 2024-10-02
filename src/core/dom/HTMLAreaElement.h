/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHTMLAreaElement__
#define __StarfishHTMLAreaElement__

#include "core/dom/HTMLHyperlinkContainer.h"

namespace Starfish {

class Path;

class HTMLAreaElement : public HTMLHyperlinkContainer {
public:
    enum class Shape { Default, Poly, Rect, Circle };

    HTMLAreaElement(Document* document, const QualifiedName& qname)
        : HTMLHyperlinkContainer(document, qname)
        , m_relList(nullptr)
        , m_path(nullptr)
        , m_shape(Shape::Rect)
    {
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                HTMLAreaElement* self = static_cast<HTMLAreaElement*>(obj);
                self->~HTMLAreaElement();
            },
            nullptr, nullptr, nullptr);
    }
    virtual ~HTMLAreaElement()
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLAreaElement() const override;

    DOMTokenList* relList();

    String* referrerPolicy();
    void setReferrerPolicy(String* policy);

    virtual bool handleDefaultEvent(Event* event) override;
    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    void invalidatePath();
    HTMLImageElement* imageElement();
    bool supportsFocus() override;
    bool isDefault()
    {
        return m_shape == Shape::Default;
    }
    bool includePoint(Frame* cb, float x, float y);

private:
    DOMTokenList* m_relList;
    Path* m_path;
    Shape m_shape;
    std::vector<double> m_coords;

    Path* areaPath(Frame* f);
};
} // namespace Starfish
#endif
