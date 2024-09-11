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

#include "StarfishConfig.h"
#include "core/dom/CustomElementRegistry.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLUnknownElement.h"

namespace Starfish {

CustomElementConstructor* CustomElementConstructor::toCustomElementConstructor(
    ScriptValue constructor)
{
    return new CustomElementConstructor(constructor);
}

CustomElementConstructor::CustomElementConstructor(ScriptValue constructor)
    : m_customElementConstructor(constructor)
{
}

CustomElementRegistry::CustomElementRegistry(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_isElementDefinitionRunning(false)
    , m_executionContext(executionContext)
{
}

ScriptBindingInstance* CustomElementRegistry::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Optional<CustomElementRegistryData*> CustomElementRegistry::find(
    ScriptValue ctor)
{
    for (const auto& item : m_registry) {
        if (item.second->constructor->scriptValue() == ctor) {
            return item.second;
        }
    }
    return nullptr;
}

Optional<CustomElementRegistryData*> CustomElementRegistry::find(
    AtomicString name)
{
    auto iter = m_registry.find(name);
    if (iter != m_registry.end()) {
        return iter->second;
    }
    return nullptr;
}

Optional<CustomElementRegistryData*> CustomElementRegistry::find(String* name)
{
    AtomicString as =
        AtomicString::createAtomicString(m_executionContext->starfish(), name);
    return find(as);
}

// https://html.spec.whatwg.org/multipage/custom-elements.html#valid-custom-element-name
static bool isValidCustomElementName(String* name)
{
    // name must match the PotentialCustomElementName production:
    // PotentialCustomElementName ::=
    // [a-z] (PCENChar)* '-' (PCENChar)*
    // PCENChar ::=
    // "-" | "." | [0-9] | "_" | [a-z] | #xB7 | [#xC0-#xD6] | [#xD8-#xF6] |
    // [#xF8-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x203F-#x2040] |
    // [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] |
    // [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
    // This uses the EBNF notation from the XML specification. [XML]
    auto bad = name->bufferAccessData();
    if (bad.length < 2) {
        return false;
    }
    char32_t first = bad.charAt(0);
    if (first < 'a' || first > 'z') {
        return false;
    }
    bool seenHypen = false;
    for (size_t i = 0; i < bad.length; i++) {
        char32_t ch = bad.charAt(i);
        if (ch >= 'A' && ch <= 'Z') {
            return false;
        }
        if (ch == '-') {
            seenHypen = true;
        } else if (ch == '.') {
        } else if (ch >= '0' && ch <= '9') {
        } else if (ch == '_') {
        } else if (ch >= 'a' && ch <= 'z') {
        } else if (ch == 0xB7) {
        } else if (ch >= 0xC0 && ch <= 0xD6) {
        } else if (ch >= 0xD8 && ch <= 0xF6) {
        } else if (ch >= 0xF8 && ch <= 0x37D) {
        } else if (ch >= 0x37F && ch <= 0x1FFF) {
        } else if (ch >= 0x200C && ch <= 0x200D) {
        } else if (ch >= 0x203F && ch <= 0x2040) {
        } else if (ch >= 0x2070 && ch <= 0x218F) {
        } else if (ch >= 0x2C00 && ch <= 0x2FEF) {
        } else if (ch >= 0x3001 && ch <= 0xD7FF) {
        } else if (ch >= 0xF900 && ch <= 0xFDCF) {
        } else if (ch >= 0xFDF0 && ch <= 0xFFFD) {
        } else if (ch >= 0x10000 && ch <= 0xEFFFF) {
        } else {
            return false;
        }
    }

    // name must not be any of the following:
    // annotation-xml
    // color-profile
    // font-face
    // font-face-src
    // font-face-uri
    // font-face-format
    // font-face-name
    // missing-glyph
    if (name->equals("annotation-xml") || name->equals("color-profile") ||
        name->equals("font-face") || name->equals("font-face-src") ||
        name->equals("font-face-uri") || name->equals("font-face-format") ||
        name->equals("font-face-name") || name->equals("missing-glyph")) {
        return false;
    }

    return true;
}

class FlagSetter {
public:
    FlagSetter(bool& flag, bool to)
        : flag(flag)
        , from(flag)
    {
        flag = to;
    }

    void restore()
    {
        flag = from;
    }

    ~FlagSetter()
    {
        restore();
    }

private:
    bool& flag;
    bool from;
};

// https://html.spec.whatwg.org/multipage/custom-elements.html#dom-customelementregistry-define
void CustomElementRegistry::define(String* name,
                                   CustomElementConstructor* constructorInput,
                                   ElementDefinitionOptions options)
{
    ScriptBindingInstance* instance =
        m_executionContext->scriptBindingInstance();
    // If IsConstructor(constructor) is false, then throw a TypeError.
    if (!isConstructibleScriptValue(constructorInput->scriptValue())) {
        throwScriptTypeError(instance, String::createASCIIString(
                                           "constructor is not constructible"));
    }
    ScriptObject constructor =
        scriptValueAsObject(constructorInput->scriptValue());
    // If name is not a valid custom element name, then throw a "SyntaxError"
    // DOMException.
    if (!isValidCustomElementName(name)) {
        throw new DOMException(m_executionContext, DOMException::SYNTAX_ERR,
                               "Invalid custom element name");
    }
    // If this CustomElementRegistry contains an entry with name name, then
    // throw a "NotSupportedError" DOMException.
    if (find(name)) {
        throw new DOMException(m_executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Same name already exists");
    }
    // If this CustomElementRegistry contains an entry with constructor
    // constructor, then throw a "NotSupportedError" DOMException.
    if (find(constructorInput->scriptValue())) {
        throw new DOMException(m_executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Same constructor already exists");
    }
    // Let localName be name.
    String* localName = name;
    // Let extends be the value of the extends member of options, or null if no
    // such member exists.
    Optional<String*> extends =
        options.hasExtends() ? options.extends() : nullptr;

    // If extends is not null, then:
    if (extends) {
        // If extends is a valid custom element name, then throw a
        // "NotSupportedError" DOMException.
        if (isValidCustomElementName(extends.value())) {
            throw new DOMException(
                m_executionContext, DOMException::NOT_SUPPORTED_ERR,
                "option extends has valid custom element name");
        }
        // If the element interface for extends and the HTML namespace is
        // HTMLUnknownElement (e.g., if extends does not indicate an element
        // definition in this specification), then throw a "NotSupportedError"
        // DOMException.
        // TODO find better way
        auto testElement =
            m_executionContext->document()->createElement(extends.value());
        if (testElement->isHTMLUnknownElement()) {
            throw new DOMException(m_executionContext,
                                   DOMException::NOT_SUPPORTED_ERR,
                                   "option extends has unknown name");
        }
        testElement = nullptr;

        // Set localName to extends.
        localName = extends.value();
    }

    // If this CustomElementRegistry's element definition is running flag is
    // set, then throw a "NotSupportedError" DOMException.
    if (m_isElementDefinitionRunning) {
        throw new DOMException(m_executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "element definition is already running");
    }
    // Set this CustomElementRegistry's element definition is running flag.
    FlagSetter flagSetter(m_isElementDefinitionRunning, true);

    // Let formAssociated be false.
    bool formAssociated = false;
    // Let disableInternals be false.
    bool disableInternals = false;
    // Let disableShadow be false.
    bool disableShadow = false;
    // Let observedAttributes be an empty sequence<DOMString>.
    GCAtomicVector<AtomicString> observedAttributes;

    // Run the following substeps while catching any exceptions
    // Let prototype be ? Get(constructor, "prototype").
    ScriptValue prototype = getScriptObjectPropertyThrowsException(
        instance, constructor, scriptStringPrototype(instance));

    // If Type(prototype) is not Object, then throw a TypeError exception.
    if (!isObjectScriptValue(prototype)) {
        throwScriptTypeError(
            instance, String::createASCIIString("prototype is not Object"));
    }

    ScriptObject prototypeObject = scriptValueAsObject(prototype);

    // Let lifecycleCallbacks be a map with the keys "connectedCallback",
    // "disconnectedCallback", "adoptedCallback", and
    // "attributeChangedCallback", each of which belongs to an entry whose value
    // is null. For each of the keys callbackName in lifecycleCallbacks, in the
    // order listed in the previous step: Let callbackValue be ? Get(prototype,
    // callbackName). If callbackValue is not undefined, then set the value of
    // the entry in lifecycleCallbacks with key callbackName to the result of
    // converting callbackValue to the Web IDL Function callback type. Rethrow
    // any exceptions from the conversion.
    ScriptValue connectedCallback = getScriptObjectPropertyThrowsException(
        instance, prototypeObject,
        createScriptASCIIString("connectedCallback"));
    ScriptValue disconnectedCallback = getScriptObjectPropertyThrowsException(
        instance, prototypeObject,
        createScriptASCIIString("disconnectedCallback"));
    ScriptValue adoptedCallback = getScriptObjectPropertyThrowsException(
        instance, prototypeObject, createScriptASCIIString("adoptedCallback"));
    ScriptValue attributeChangedCallback =
        getScriptObjectPropertyThrowsException(
            instance, prototypeObject,
            createScriptASCIIString("attributeChangedCallback"));

    // If the value of the entry in lifecycleCallbacks with key
    // "attributeChangedCallback" is not null, then:
    if (scriptNull() != attributeChangedCallback) {
        // Let observedAttributesIterable be ? Get(constructor,
        // "observedAttributes").
        ScriptValue observedAttributesIterable =
            getScriptObjectPropertyThrowsException(
                instance, constructor,
                createScriptASCIIString("observedAttributes"));
        // If observedAttributesIterable is not undefined,
        if (scriptUndefined() != observedAttributesIterable) {
            // then set observedAttributes to the result of converting
            // observedAttributesIterable to a sequence<DOMString>. Rethrow any
            // exceptions from the conversion.
            GCVector<ScriptValue> observedAttributesValue =
                scriptReadIterableValue(instance, observedAttributesIterable,
                                        true)
                    .value();
            for (auto v : observedAttributesValue) {
                bool result = true;
                auto s = toBrowserString(instance, v, &result);
                if (!result) {
                    throwScriptTypeError(
                        instance, String::createASCIIString(
                                      "An error occurred while toString"));
                }
                observedAttributes.push_back(AtomicString::createAtomicString(
                    m_executionContext->starfish(), s));
            }
        }
    }

    // Let disabledFeatures be an empty sequence<DOMString>.
    GCVector<ScriptValue> disabledFeatures;
    // Let disabledFeaturesIterable be ? Get(constructor, "disabledFeatures").
    ScriptValue disabledFeaturesIterable =
        getScriptObjectPropertyThrowsException(
            instance, constructor, createScriptASCIIString("disabledFeatures"));
    // If disabledFeaturesIterable is not undefined, then set disabledFeatures
    // to the result of converting disabledFeaturesIterable to a
    // sequence<DOMString>. Rethrow any exceptions from the conversion.
    if (scriptUndefined() != disabledFeaturesIterable) {
        disabledFeatures = scriptReadIterableValueThrowsException(
            instance, disabledFeaturesIterable);
    }
    // Set disableInternals to true if disabledFeatures contains "internals".
    // Set disableShadow to true if disabledFeatures contains "shadow".
    for (size_t i = 0; i < disabledFeatures.size(); i++) {
        bool result = true;
        auto s = toBrowserString(instance, disabledFeatures[i], &result);
        if (!result) {
            throwScriptTypeError(
                instance,
                String::createASCIIString("An error occurred while toString"));
        }
        if (s->equals("internals")) {
            disableInternals = true;
        } else if (s->equals("shadow")) {
            disableShadow = true;
        }
    }

    // Let formAssociatedValue be ? Get( constructor, "formAssociated").
    // Set formAssociated to the result of converting formAssociatedValue to a
    // boolean. Rethrow any exceptions from the conversion.
    formAssociated =
        scriptValueToBoolean(instance,
                             getScriptObjectPropertyThrowsException(
                                 instance, constructor,
                                 createScriptASCIIString("formAssociated")),
                             true)
            .value();

    // If formAssociated is true, for each of "formAssociatedCallback",
    // "formResetCallback", "formDisabledCallback", and
    // "formStateRestoreCallback" callbackName: Let callbackValue be ?
    // Get(prototype, callbackName). If callbackValue is not undefined, then set
    // the value of the entry in lifecycleCallbacks with key callbackName to the
    // result of converting callbackValue to the Web IDL Function callback type.
    // Rethrow any exceptions from the conversion.
    ScriptValue formAssociatedCallback = scriptUndefined();
    ScriptValue formResetCallback = scriptUndefined();
    ScriptValue formDisabledCallback = scriptUndefined();
    ScriptValue formStateRestoreCallback = scriptUndefined();
    if (formAssociated) {
        formAssociatedCallback = getScriptObjectPropertyThrowsException(
            instance, prototypeObject,
            createScriptASCIIString("formAssociatedCallback"));
        formResetCallback = getScriptObjectPropertyThrowsException(
            instance, prototypeObject,
            createScriptASCIIString("formResetCallback"));
        formDisabledCallback = getScriptObjectPropertyThrowsException(
            instance, prototypeObject,
            createScriptASCIIString("formDisabledCallback"));
        formStateRestoreCallback = getScriptObjectPropertyThrowsException(
            instance, prototypeObject,
            createScriptASCIIString("formStateRestoreCallback"));
    }
    // Then, perform the following substep, regardless of whether the above
    // steps threw an exception or not: Unset this CustomElementRegistry's
    // element definition is running flag.
    flagSetter.restore();
    // Finally, if the first set of substeps threw an exception, then rethrow
    // that exception (thus terminating this algorithm). Otherwise, continue
    // onward.

    // Let definition be a new custom element definition with name name, local
    // name localName, constructor constructor, observed attributes
    // observedAttributes, lifecycle callbacks lifecycleCallbacks,
    // form-associated formAssociated, disable internals disableInternals, and
    // disable shadow disableShadow. Add definition to this
    // CustomElementRegistry. Let document be this CustomElementRegistry's
    // relevant global object's associated Document.
    Starfish* sf = m_executionContext->starfish();
    QualifiedName qname(AtomicString::createAtomicString(sf, HTML_NAMESPACE),
                        AtomicString::createAttrAtomicString(sf, name));

    CustomElementRegistryData* data = new CustomElementRegistryData();
    data->constructor = constructorInput;
    data->name = qname;
    data->connectedCallback = connectedCallback;
    data->disconnectedCallback = disconnectedCallback;
    data->adoptedCallback = adoptedCallback;
    data->attributeChangedCallback = attributeChangedCallback;
    data->disableInternals = disableInternals;
    data->disableShadow = disableShadow;
    data->formAssociatedCallback = formAssociatedCallback;
    data->formResetCallback = formResetCallback;
    data->formDisabledCallback = formDisabledCallback;
    data->formStateRestoreCallback = formStateRestoreCallback;

    m_registry.insert(std::make_pair(qname.localNameAtomic(), data));

    // Let upgrade candidates be all elements that are shadow-including
    // descendants of document, whose namespace is the HTML namespace and whose
    // local name is localName, in shadow-including tree order. Additionally, if
    // extends is non-null, only include elements whose is value is equal to
    // name.
    upgrade(data);

    // TODO If this CustomElementRegistry's when-defined promise map contains an
    // entry with key name:
    //     TODO Let promise be the value of that entry.
    //     TODO Resolve promise with constructor.
    //     TODO Delete the entry with key name from this CustomElementRegistry's
    //     when-defined promise map.
}

void CustomElementRegistry::upgrade(CustomElementRegistryData* data)
{
    // TODO For each element element in upgrade candidates, enqueue a custom
    // element upgrade reaction given element and definition.
    Traverse::traverse(m_executionContext->document(), [&](Node* e) {
        if (e->isElement() && e->asElement()->name() == data->name) {
            STARFISH_ASSERT(e->isHTMLUnknownElement());
            e->asHTMLUnknownElement()->morphIntoCustomElement(data);
            STARFISH_ASSERT(!e->isHTMLUnknownElement());
        }
    });
}

CustomElementConstructor* CustomElementRegistry::get(String* name)
{
    auto item = find(name);
    if (item) {
        return item.value()->constructor;
    }
    return new CustomElementConstructor(scriptUndefined());
}

Nullable<String*> CustomElementRegistry::getName(
    CustomElementConstructor* constructor)
{
    auto item = find(constructor->scriptValue());
    if (item) {
        return item.value()->name.localName();
    }
    return nullptr;
}

} // namespace Starfish
