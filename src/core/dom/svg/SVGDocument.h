/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGDocument__
#define __StarfishSVGDocument__

#include "core/dom/XMLDocument.h"

namespace Starfish {

class SVGDocument : public XMLDocument {
public:
    SVGDocument(Window* window, ScriptBindingInstance* scriptBindingInstance,
                ResourceURL* uri, String* charSet,
                bool doesParticipateInRendering)
        : XMLDocument(window, scriptBindingInstance, uri, charSet,
                      doesParticipateInRendering)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGDocument() const override;

    // HTML § 13.2.6.5 ("adjust SVG tag names"): restore spec-defined camelCase
    // names lowercased by HTMLTokenizer. Used only on the HTML parser path;
    // createSVGElement() preserves qname's exact case for createElementNS,
    // DOMParser and cloneNode.
    static QualifiedName adjustSVGTagName(Starfish* starfish,
                                          const QualifiedName& qname);
    static Element* createSVGElement(Document* document,
                                     const QualifiedName& qname);

protected:
};
} // namespace Starfish

#endif
