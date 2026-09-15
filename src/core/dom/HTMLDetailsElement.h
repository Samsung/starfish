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

#ifndef __StarfishHTMLDetailsElement__
#define __StarfishHTMLDetailsElement__

#include "core/dom/HTMLElement.h"

namespace Starfish {
class HTMLSlotElement;

class HTMLDetailsElement : public HTMLElement {
public:
    HTMLDetailsElement(Document* document, const QualifiedName& qname);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isHTMLDetailsElement() const override;

    bool open();
    void setOpen(bool value);
    Optional<Element*> firstSummary();
    static Optional<HTMLDetailsElement*> summaryOwner(Node* summary);
    // The slot of the internal shadow tree a light-DOM child renders in. The
    // second form takes a precomputed firstSummary() for batch assignment.
    HTMLSlotElement* slotFor(Node* child);
    HTMLSlotElement* slotFor(Node* child, Optional<Element*> firstSummary);
    void didAttributeChanged(QualifiedName name, Optional<String*> old,
                             String* value, bool attributeCreated,
                             bool attributeRemoved) override;
    void ensureExclusivity(bool closeOthers);

private:
    struct ToggleTask;
    void queueToggle(bool oldOpen);
    bool isDocumentDisposed();
    // The queued "details notification task", until it runs or is replaced.
    ToggleTask* m_pendingToggle{ nullptr };
};
} // namespace Starfish
#endif
