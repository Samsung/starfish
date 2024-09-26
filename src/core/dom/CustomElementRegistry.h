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
class CustomElementRegistry;

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
    kUpgradedWhenDefined,
    kUpgradedWhenConnected,
    kConnected,
    kDisconnected,
    kAdoptped,
    kAttributeChanged,
};

class CustomElementQueue : public GCVector<HTMLCustomElement*> {
public:
    void process();
};

class CustomElementReactionStack {
    friend class CustomElementRegistry;

public:
    ALWAYS_INLINE CustomElementReactionStack()
        : m_previousProcessingStack(s_currentProcessingStack)
    {
        s_currentProcessingStack = this;
    }

    ~CustomElementReactionStack()
    {
        if (UNLIKELY(m_queue.size())) {
            m_queue.process();
        }
        s_currentProcessingStack = m_previousProcessingStack;
    }

private:
    CustomElementQueue m_queue;
    CustomElementReactionStack* const m_previousProcessingStack;

    static CustomElementReactionStack* s_currentProcessingStack;
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

    void upgrade(Node* node, bool inCaseOfConnectedToDocument = false);
    void upgrade(CustomElementRegistryData* data);
    void upgrade(Element* element, CustomElementRegistryData* data,
                 bool invokeCallbacks, bool inCaseOfConnectedToDocument);

    Promise* whenDefined(String* name);

    // https://html.spec.whatwg.org/multipage/custom-elements.html#enqueue-a-custom-element-callback-reaction
    void enqueueToCustomElementsReactionStack(HTMLCustomElement* element,
                                              CustomElementCallbackType type,
                                              Optional<ScriptValue*> data);

private:
    bool m_isElementDefinitionRunning;
    bool m_isProcessingBackupElementQueue;
    ExecutionContext* m_executionContext;
    GCUnorderedMap<AtomicString, CustomElementRegistryData*> m_registry;
    GCUnorderedMap<AtomicString, Promise*> m_whenDefinedMap;

    // https://html.spec.whatwg.org/multipage/custom-elements.html#enqueue-an-element-on-the-appropriate-element-queue
    void enqueueElementOnAppropriateElementQueue(HTMLCustomElement* element);

    using ElementReactionQueue =
        GCVector<std::pair<CustomElementCallbackType, Optional<ScriptValue*>>>;
    GCUnorderedMap<HTMLCustomElement*, ElementReactionQueue*>
        m_customElementReactionQueues;
    CustomElementQueue m_customElementReactionStackBackupQueue;

    ElementReactionQueue& elementReactionQueue(HTMLCustomElement* element);

    // https://html.spec.whatwg.org/multipage/custom-elements.html#invoke-custom-element-reactions
    void invokeCustomElementReactions(GCVector<HTMLCustomElement*>& queue);
    friend class CustomElementQueue;
    void invokeCustomElementReaction(HTMLCustomElement* element,
                                     CustomElementCallbackType type,
                                     Optional<ScriptValue*> data);
};
} // namespace Starfish

#endif
