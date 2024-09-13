/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCustomElementRegistry__
#define __StarfishCustomElementRegistry__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

class HTMLCustomElement;

struct ElementDefinitionOptions {
    ElementDefinitionOptions()
        : m_extends(nullptr)
    {
    }

    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, extends, Extends);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(String*, extends, Extends);
};

class CustomElementConstructor : public gc {
public:
    static CustomElementConstructor* toCustomElementConstructor(
        ScriptValue constructor);
    CustomElementConstructor(ScriptValue constructor);

    ScriptValue scriptValue()
    {
        return m_customElementConstructor;
    }

private:
    ScriptValue m_customElementConstructor = nullptr;
};

struct CustomElementRegistryData : public gc {
    CustomElementRegistry* registry;
    CustomElementConstructor* constructor;
    QualifiedName name;
    Optional<String*> extends;

    GCAtomicVector<AtomicString> observedAttributes;
    ScriptValue connectedCallback;
    ScriptValue disconnectedCallback;
    ScriptValue adoptedCallback;
    ScriptValue attributeChangedCallback;

    bool disableInternals;
    bool disableShadow;
    ScriptValue formAssociatedCallback;
    ScriptValue formResetCallback;
    ScriptValue formDisabledCallback;
    ScriptValue formStateRestoreCallback;

    CustomElementRegistryData()
        : registry(nullptr)
        , constructor(nullptr)
        , name(QualifiedName(AtomicString::emptyAtomicString()))
        , connectedCallback(scriptUndefined())
        , disconnectedCallback(scriptUndefined())
        , adoptedCallback(scriptUndefined())
        , attributeChangedCallback(scriptUndefined())
        , disableInternals(false)
        , disableShadow(false)
        , formAssociatedCallback(scriptUndefined())
        , formResetCallback(scriptUndefined())
        , formDisabledCallback(scriptUndefined())
        , formStateRestoreCallback(scriptUndefined())
    {
    }
};

enum class CustomElementCallbackType : uint8_t {
    kUpgraded,
    kConnected,
    kDisconnected,
    kAdoptped,
    kAttributeChanged,
};

// https://html.spec.whatwg.org/multipage/custom-elements.html
class CustomElementRegistry : public ScriptWrappable {
public:
    CustomElementRegistry(ExecutionContext* executionContext);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isCustomElementRegistry() const override;

    HTMLCustomElement* createCustomElement(Document* document,
                                           CustomElementRegistryData* data,
                                           bool invokeCallbacks = true);

    Optional<CustomElementRegistryData*> find(ScriptValue ctor);
    Optional<CustomElementRegistryData*> find(AtomicString name);
    Optional<CustomElementRegistryData*> find(String* name);

    // https://html.spec.whatwg.org/multipage/custom-elements.html#dom-customelementregistry-define
    void define(String* name, CustomElementConstructor* constructor,
                ElementDefinitionOptions options = {});

    CustomElementConstructor* get(String* name);
    Nullable<String*> getName(CustomElementConstructor* constructor);

    void upgrade(Node* node);
    void upgrade(Element* element, CustomElementRegistryData* data,
                 bool invokeCallbacks = true);
    void upgrade(CustomElementRegistryData* data);
    // https://html.spec.whatwg.org/multipage/custom-elements.html#enqueue-a-custom-element-callback-reaction
    void enqueueToCustomElementsReactionStack(HTMLCustomElement* element,
                                              CustomElementCallbackType type,
                                              Optional<ScriptValue*> data);

private:
    bool m_isElementDefinitionRunning;
    bool m_isProcessingBackupElementQueue;
    ExecutionContext* m_executionContext;
    GCUnorderedMap<AtomicString, CustomElementRegistryData*> m_registry;

    // https://html.spec.whatwg.org/multipage/custom-elements.html#enqueue-an-element-on-the-appropriate-element-queue
    void enqueueAnElementOnAppropriateElementQueue(
        HTMLCustomElement* element, CustomElementCallbackType type,
        Optional<ScriptValue*> data);

    struct CustomElementReactionData : public gc {
        GCVector<std::pair<CustomElementCallbackType, Optional<ScriptValue*>>>
            reactionData;
    };

    GCUnorderedMap<HTMLCustomElement*, CustomElementReactionData*>
        m_customElementReactions;

    GCVector<HTMLCustomElement*> m_customElementReactionStack;
    GCVector<HTMLCustomElement*> m_customElementReactionStackBackupQueue;

    // https://html.spec.whatwg.org/multipage/custom-elements.html#invoke-custom-element-reactions
    void invokeCustomElementReactions(GCVector<HTMLCustomElement*>& queue);
    friend class HTMLCustomElement;
    void invokeCustomElementReaction(HTMLCustomElement* element,
                                     CustomElementCallbackType type,
                                     Optional<ScriptValue*> data);
};
} // namespace Starfish

#endif
