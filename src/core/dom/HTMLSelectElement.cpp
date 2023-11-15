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
// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/dom/HTMLSelectElement.h"

#include "binding/WindowProxy.h"
#include "core/dom/Event.h"
#include "core/dom/Document.h"
#include "core/dom/DOMRect.h"
#include "core/dom/HTMLBRElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLOptionsCollection.h"
#include "core/dom/HTMLOptGroupElement.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "core/style/StyleUtil.h"
#include "platform/window/PlatformWindow.h"

namespace Starfish {

HTMLSelectElement::HTMLSelectElement(Document* document,
                                     const QualifiedName& qname)
    : HTMLFormControl(document, qname)
    , m_selectedOptions(nullptr)
    , m_options(nullptr)
{
}

void* HTMLSelectElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLSelectElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLSelectElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSelectElement, m_selectedOptions));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSelectElement, m_options));
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLSelectElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLSelectElement::value()
{
    HTMLOptionElement* firstOptionNode = nullptr;
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    for (Node* c : list) {
        HTMLOptionElement* opt = c->asHTMLOptionElement();
        if (opt->selected()) {
            firstOptionNode = opt;
            break;
        }
    }

    if (firstOptionNode) {
        HTMLOptionElement* opt = firstOptionNode->asHTMLOptionElement();
        return opt->value();
    } else {
        return String::emptyString;
    }
}

void HTMLSelectElement::setValue(String* value)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    for (Node* c : list) {
        HTMLOptionElement* opt = c->asHTMLOptionElement();
        opt->setSelected(false);
    }

    for (Node* c : list) {
        HTMLOptionElement* opt = c->asHTMLOptionElement();
        if (opt->value()->equals(value)) {
            opt->setSelected(true);
            opt->updateExtenedMarker();
            break;
        }
    }

    fireEvent(starfish()->staticStrings()->m_change, true, false);

    setNeedsFrameTreeBuild();
}

// use this function internally to retrive a list of option elements efficiently
void HTMLSelectElement::computeListOfOptionElements(
    Node* parent, GCVector<HTMLOptionElement*>& list)
{
    for (Node* c = parent->firstChild(); c; c = c->nextSibling()) {
        if (c->isHTMLOptionElement()) {
            list.push_back(c->asHTMLOptionElement());
        } else if (c->isHTMLOptGroupElement()) {
            computeListOfOptionElements(c, list);
        }
    }
}

// IDL method
HTMLCollection* HTMLSelectElement::selectedOptions()
{
    if (!m_selectedOptions) {
        m_selectedOptions = new HTMLCollection(
            this, NodeListImpl::SelectedOptionsFilter, nullptr, false);
    }
    return m_selectedOptions;
}

// internal use for faster computation
void HTMLSelectElement::computeSelectedOptions(
    GCVector<HTMLOptionElement*>& list)
{
    GCVector<HTMLOptionElement*> listAll;
    computeListOfOptionElements(this, listAll);

    for (auto item : listAll) {
        if (item->selectedness()) {
            list.push_back(item);
        }
    }
}

String* HTMLSelectElement::type()
{
    if (multiple()) {
        return String::createASCIIString("select-multiple");
    } else {
        return String::createASCIIString("select-one");
    }
}

int HTMLSelectElement::size()
{
    String* size = getAttributeOrEmpty(starfish()->staticStrings()->m_size);
    if (!size->equals(String::emptyString)) {
        return String::parseInt(size);
    }

    return 0;
}

void HTMLSelectElement::setSize(int size)
{
    if (size > 0) {
        setAttribute(starfish()->staticStrings()->m_size,
                     String::fromInt(size));
    }
}

int HTMLSelectElement::displaySize()
{
    int s = size();
    if (s > 0) {
        return s;
    }

    // https://html.spec.whatwg.org/multipage/form-elements.html#the-select-element
    return multiple() ? 4 : 1;
}

void HTMLSelectElement::reset()
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    for (auto item : list) {
        if (item->isHTMLOptionElement()) {
            auto o = item->asHTMLOptionElement();
            o->setSelectedness(o->defaultSelected());
            o->setDirtiness(false);
        }
    }
    for (auto item : list) {
        if (item->isHTMLOptionElement()) {
            auto o = item->asHTMLOptionElement();
            resetFromOption(list, o);
        }
    }
}

// IDL method
HTMLOptionsCollection* HTMLSelectElement::options()
{
    if (!m_options) {
        m_options = new HTMLOptionsCollection(
            this, NodeListImpl::OptionElementFilter, nullptr, false);
    }

    return m_options;
}

unsigned HTMLSelectElement::length()
{
    return options()->length();
}

void HTMLSelectElement::setLength(unsigned newLength)
{
    unsigned currentLength = length();
    if (currentLength < newLength) {
        while (currentLength < newLength) {
            appendChild(new HTMLOptionElement(document()));
            currentLength++;
        }
    } else {
        while (currentLength > newLength) {
            remove(currentLength - 1);
            currentLength--;
        }
    }
}

HTMLOptionElement* HTMLSelectElement::item(unsigned index)
{
    if (Element* option = options()->item(index)) {
        return option->asHTMLOptionElement();
    }

    return nullptr;
}

HTMLOptionElement* HTMLSelectElement::namedItem(String* name)
{
    if (Element* option = options()->namedItem(name)) {
        return option->asHTMLOptionElement();
    }

    return nullptr;
}

void HTMLSelectElement::add(HTMLOptionElementOrHTMLOptGroupElement element,
                            Nullable<HTMLElementOrlong> before)
{
    HTMLElement* newElement;
    if (element.isHTMLOptionElementValue()) {
        newElement = element.getHTMLOptionElementValue();
    } else {
        STARFISH_ASSERT(element.isHTMLOptGroupElementValue());
        newElement = element.getHTMLOptGroupElementValue();
    }

    HTMLElement* beforeElement = nullptr;
    if (before.hasValue()) {
        if (before.getValue().isHTMLElementValue()) {
            beforeElement = before.getValue().getHTMLElementValue();
        } else if (before.getValue().islongValue()) {
            beforeElement = item(before.getValue().getlongValue());
        }
    }
    insertBefore(newElement, beforeElement);
}

void HTMLSelectElement::remove(int index)
{
    if (index < 0) {
        return;
    }

    if (HTMLOptionElement* option = item(index)) {
        option->remove();
    }
}

bool HTMLSelectElement::defaultIndexedSetter(unsigned index,
                                             HTMLOptionElement* option)
{
    if (!option) {
        remove(index);
        return true;
    }

    if (index > length()) {
        setLength(index);
    }

    if (index == length()) {
        appendChild(option);
        return true;
    }

    HTMLOptionElement* oldOption = item(index);
    Node* parent = oldOption->parentNode();

    STARFISH_ASSERT(oldOption && parent);

    parent->replaceChild(option, oldOption);
    return true;
}

int32_t HTMLSelectElement::selectedIndex()
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    for (size_t i = 0; i < list.size(); i++) {
        HTMLOptionElement* opt = list[i];

        if (opt->selected()) {
            return i;
        }
    }

    return -1;
}

void HTMLSelectElement::setSelectedIndex(size_t index)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    for (HTMLOptionElement* opt : list) {
        opt->setSelectedness(false);
    }

    for (size_t i = 0; i < list.size(); i++) {
        HTMLOptionElement* opt = list[i];
        if (i == index) {
            opt->setSelectedness(true);
            opt->setDirtiness(true);
        }
    }
}

void HTMLSelectElement::resetFromOption(GCVector<HTMLOptionElement*>& list,
                                        HTMLOptionElement* resetFrom)
{
    Nullable<String*> val =
        getAttribute(starfish()->staticStrings()->m_multiple);
    if (!val.hasValue()) {
        // single selection
        int selectedOptions = 0;
        for (HTMLOptionElement* opt : list) {
            if (opt->selected()) {
                selectedOptions++;
            }
        }

        if (displaySize() == 1 && selectedOptions == 0) {
            for (HTMLOptionElement* opt : list) {
                if (opt->defaultSelected() && !opt->disabled()) {
                    opt->setSelectedness(true);
                    break;
                }
            }
        } else if (selectedOptions >= 2) {
            for (HTMLOptionElement* opt : list) {
                if (opt != resetFrom) {
                    opt->setSelectedness(false);
                }
            }
        }
    } else {
        // Todo
    }
}

void HTMLSelectElement::resetFromOption(HTMLOptionElement* resetFrom)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    resetFromOption(list, resetFrom);
}

void HTMLSelectElement::didNodeInserted(Node* parent, Node* newChild)
{
    if (!newChild->isHTMLOptionElement()) {
        return;
    }

    HTMLOptionElement* newElement = newChild->asHTMLOptionElement();
    GCVector<HTMLOptionElement*> selectedOptions;
    computeSelectedOptions(selectedOptions);

    if (!multiple()) {
        if (displaySize() == 1 && selectedOptions.size() == 0) {
            GCVector<HTMLOptionElement*> list;
            computeListOfOptionElements(this, list);
            for (auto option : list) {
                if (!option->isDisabled()) {
                    option->setSelectedness(true);
                    break;
                }
            }
        } else if (selectedOptions.size() >= 2) {
            GCVector<HTMLOptionElement*> list;
            computeListOfOptionElements(this, list);

            for (size_t i = 0; i < list.size() - 1; i++) {
                list[i]->setSelectedness(false);
            }
        }
    }
}

HTMLOptionElement* HTMLSelectElement::firstSelectedOptionElement()
{
    GCVector<HTMLOptionElement*> selectedOptions;
    computeSelectedOptions(selectedOptions);
    if (selectedOptions.size() > 0) {
        return selectedOptions[0];
    }

    return nullptr;
}

// https://html.spec.whatwg.org/multipage/form-elements.html#send-select-update-notifications
void HTMLSelectElement::fireSelectUpdateNotification()
{
    queueEvent(starfish()->staticStrings()->m_input, true, false);
    queueEvent(starfish()->staticStrings()->m_change, true, false);
}

// https://html.spec.whatwg.org/multipage/form-elements.html#the-select-element
bool HTMLSelectElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (isDisabled()) {
        // Don't do anything. Just propagate.
        return false;
    }

    if (event->isMouseEvent() || event->isTouchEvent()) {
        if (event->type()->equalsIgnoreCase("click")) {
            if (event->target()->isHTMLOptionElement()) {
                HTMLOptionElement* option =
                    event->target()->asHTMLOptionElement();
                if (option->selectElement() == this) {
                    if (displaySize() == 1) {
                        showDropdownMenu();
                    } else {
                        // TODO: select / deselect items from a list
                    }
                }
            }
        }
    }

    return false;
}

void HTMLSelectElement::showDropdownMenu()
{
    if (webView()->useExternalPopup()) {
        // register the callback to be called when an item is selected
        webView()->platformWindow()->registerCallbackHandler(
            WindowHandlerOnDropdownMenuItemSelected,
            [this](void* param) -> void {
                struct Param {
                    int position;
                };
                Param* p = (Param*)param;
                onDropdownMenuItemSelected(p->position);
                delete p;
            });

        // calls the platform's dropdownmenu UI
        struct Param {
            std::vector<std::string>* list;
            int checkedPosition;
        };

        Param* p = new Param();
        p->list = new std::vector<std::string>();
        p->checkedPosition = selectedIndex();

        GCVector<HTMLOptionElement*> list;
        computeListOfOptionElements(this, list);
        for (auto item : list) {
            if (item->isHTMLOptionElement()) {
                auto o = item->asHTMLOptionElement();
                GCVector<StringView> tokens;
                StringUtils::wordTokenizer(o->text(), tokens);
                StringBuilder sb;
                for (size_t i = 0; i < tokens.size(); i++) {
                    sb.appendString(tokens[i].substring());
                    if (i < tokens.size() - 1) {
                        sb.appendChar(' ');
                    }
                }
                p->list->push_back(sb.finalize()->toUTF8NonGCString().data());
            }
        }

        webView()->platformWindow()->callHandler(WindowHandlerShowDropdownMenu,
                                                 (void*)p);
    } else {
        if (frame() == nullptr) {
            return;
        }

        StringBuilder builder;
        builder.appendString("window.dialogArguments = {\n");
        addSelectedIndex(builder);
        addBaseStyle(builder);
        addChildren(builder);
        addProperty("anchorRectInScreen", getBoundingClientRect(), builder);
        addProperty("zoomFactor", 1, builder);
        addProperty("scaleFactor", webView()->screenInfo().devicePixelRatio,
                    builder);
        bool isRTL = style()->direction() == DirectionValue::RtlDirectionValue;
        addProperty("isRTL", isRTL, builder);
        if (frame()->isFrameBox()) {
            addProperty("paddingStart",
                        isRTL ? frame()->asFrameBox()->paddingRight().toDouble()
                              : frame()->asFrameBox()->paddingLeft().toDouble(),
                        builder);
        }
        builder.appendString("};\n");

        String* iframeId = String::fromUTF8("STARFISH_DROPDOWN_MENU");
        builder.appendString("var iframeId ='");
        builder.appendString(iframeId);
        builder.appendString("';\n");

        DOMRect* rect = getBoundingClientRect();
        builder.appendString("var rect");
        builder.appendString("= {");
        addProperty("x", rect->x() + window()->scrollX(), builder);
        addProperty("y", rect->y() + window()->scrollY(), builder);
        addProperty("width", rect->width(), builder);
        addProperty("height", rect->height(), builder);
        builder.appendString("};\n");

        int padding = static_cast<int>(
            roundf(2 * webView()->screenInfo().devicePixelRatio));
        int min_height = static_cast<int>(
            roundf(12 * webView()->screenInfo().devicePixelRatio));
        builder.appendString(String::fromUTF8("var optionStyle = '"));
        builder.appendString(
            String::fromUTF8("option, optgroup {padding-top:"));
        builder.appendString(String::fromInt(padding));
        builder.appendString(String::fromUTF8("px;} option {padding-bottom: "));
        builder.appendString(String::fromInt(padding));
        builder.appendString(String::fromUTF8("px;min-height: "));
        builder.appendString(String::fromInt(min_height));
        builder.appendString(
            String::fromUTF8("px;display: flex;align-items: center;}'\n"));

        constexpr char pickerJs[] =
#include "core/dom/picker.js"
            ;
        String* picker = String::createASCIIStringWithNoCopy(pickerJs);
        builder.appendString(picker);

        String* script = builder.finalize();
        window()->webView()->evaluateJavaScript(script);

        Element* e = document()->getElementById(iframeId);
        if (e && e->isHTMLIFrameElement()) {
            auto win = e->asHTMLIFrameElement()->contentWindow();
            jsGlobalObjectDefinePropertyIfNotExists(
                win->scriptBindingInstance(), String::fromUTF8("picker"),
                scriptValue());
        }
    }
}

void HTMLSelectElement::addSelectedIndex(StringBuilder& data)
{
    addProperty("selectedIndex", selectedIndex(), data);
}

void HTMLSelectElement::addBaseStyle(StringBuilder& data)
{
    ComputedStyle* s = style();
    data.appendString("baseStyle: {\n");

    addProperty("backgroundColor", s->backgroundColor().toHTMLColorCodeString(),
                data);
    addProperty("color", s->color().toHTMLColorCodeString(), data);
    addProperty("textTransform",
                StyleUtil::textTransformToString(s->textTransform()), data);
    addProperty("fontSize", s->fontSize().toString(), data);
    addProperty("fontStyle", StyleUtil::fontStyleToString(s->fontStyle()),
                data);
    addProperty("fontVariant", String::fromUTF8("normal"), data);

    size_t len = s->fontFamily()->m_length;
    data.appendString("fontFamily: [\n");
    for (size_t i = 0; i < len; ++i) {
        data.appendString("'");
        data.appendString(s->fontFamily()[i + 1].m_familyName);
        data.appendString("'");
        if (i + 1 < len) {
            data.appendString(", ");
        }
    }
    data.appendString("]\n");

    data.appendString("},\n");
}

void HTMLSelectElement::addChildren(StringBuilder& data)
{
    data.appendString("children: [\n");
    size_t len = options()->length();
    for (size_t i = 0; i < len; ++i) {
        Element* child = options()->item(i);
        if (child->isHTMLOptionElement()) {
            addOption(child->asHTMLOptionElement(), data);
        } else if (child->isHTMLBRElement()) {
            addSeparator(child->asHTMLBRElement(), data);
        } else if (child->isHTMLOptGroupElement()) {
            // TODO: Implement optGroup
        }
    }
    data.appendString("],\n");
}

void HTMLSelectElement::addOption(HTMLOptionElement* element,
                                  StringBuilder& data)
{
    STARFISH_ASSERT(element);
    data.appendString("{");
    addProperty("label", element->label(), data);
    addProperty("value", element->value(), data);
    if (!element->title()->isEmpty()) {
        addProperty("title", element->title(), data);
    }
    String* ariaLabel =
        element->getAttributeOrEmpty(starfish()->staticStrings()->m_ariaLabel);
    if (!ariaLabel->isEmpty()) {
        addProperty("ariaLabel", ariaLabel, data);
    }
    if (element->disabled()) {
        addProperty("disabled", true, data);
    }
    addElementStyle(element, data);
    data.appendString("},");
}

void HTMLSelectElement::addSeparator(HTMLBRElement* element,
                                     StringBuilder& data)
{
    STARFISH_ASSERT(element);
    data.appendString("{\n");
    data.appendString("type: \"separator\",\n");
    addProperty("title", element->title(), data);
    String* ariaLabel =
        element->getAttributeOrEmpty(starfish()->staticStrings()->m_ariaLabel);
    if (!ariaLabel->isEmpty()) {
        addProperty("ariaLabel", ariaLabel, data);
    }
    addProperty("disabled", true, data);
    addElementStyle(element, data);
    data.appendString("},");
}

void HTMLSelectElement::addElementStyle(HTMLElement* element,
                                        StringBuilder& data)
{
    ComputedStyle* s = style();
    data.appendString("style: {\n");

    if (s->visibility() == VisibilityValue::HiddenVisibilityValue) {
        addProperty("visibility", String::fromUTF8("hidden"), data);
    }
    if (s->display() == DisplayValue::NoneDisplayValue) {
        addProperty("display", String::fromUTF8("none"), data);
    }

    ComputedStyle* baseStyle = element->parentElement()->style();
    if (baseStyle->direction() == s->direction()) {
        if (s->direction() == DirectionValue::RtlDirectionValue) {
            addProperty("direction", String::fromUTF8("rtl"), data);

        } else {
            addProperty("direction", String::fromUTF8("ltr"), data);
        }
    }
    if (s->unicodeBidi() == UnicodeBidiValue::IsolateUnicodeBidiValue) {
        addProperty("unicodeBidi", String::fromUTF8("bidi-override"), data);
    }

    Unit::Color fgColor = s->color();
    if (baseStyle->color() != fgColor) {
        addProperty("color", fgColor.toHTMLColorCodeString(), data);
    }
    Unit::Color bgColor = s->backgroundColor();
    if (baseStyle->color() != bgColor && bgColor != Unit::Color()) {
        addProperty("backgroundColor", bgColor.toHTMLColorCodeString(), data);
    }

    if (baseStyle->fontSize() != s->fontSize()) {
        addProperty("fontSize", s->fontSize().toString(), data);
    }

    if (baseStyle->fontWeight() != s->fontWeight()) {
        addProperty("fontWeight",
                    StyleUtil::fontWeightToString(s->fontWeight()), data);
    }

    if (baseStyle->fontFamily() != s->fontFamily()) {
        size_t len = s->fontFamily()->m_length;
        data.appendString("fontFamily: [\n");
        for (size_t i = 0; i < len; ++i) {
            data.appendString("'");
            data.appendString(s->fontFamily()[i + 1].m_familyName);
            data.appendString("'");
            if (i + 1 < len) {
                data.appendString(", ");
            }
        }
        data.appendString("]\n");
    }

    if (baseStyle->fontStyle() != s->fontStyle()) {
        addProperty("fontStyle", StyleUtil::fontStyleToString(s->fontStyle()),
                    data);
    }
    if (baseStyle->textTransform() != s->textTransform()) {
        addProperty("textTransform",
                    StyleUtil::textTransformToString(s->textTransform()), data);
    }

    data.appendString("},\n");
}

void HTMLSelectElement::addProperty(const char* name, String* value,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": '");
    data.appendString(value);
    data.appendString("',\n");
}

void HTMLSelectElement::addProperty(const char* name, int value,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": ");
    data.appendString(String::fromInt(value));
    data.appendString(",\n");
}

void HTMLSelectElement::addProperty(const char* name, bool value,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": ");
    if (value) {
        data.appendString("true");
    } else {
        data.appendString("false");
    }
    data.appendString(",\n");
}

void HTMLSelectElement::addProperty(const char* name, float value,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": ");
    data.appendString(String::fromFloat(value));
    data.appendString(",\n");
}

void HTMLSelectElement::addProperty(const char* name, double value,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": ");
    data.appendString(String::fromDouble(value));
    data.appendString(",\n");
}

void HTMLSelectElement::addProperty(const char* name,
                                    const GCVector<String*>& values,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": [");
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) {
            data.appendString(", ");
        }
        data.appendString("'");
        data.appendString(values[i]);
        data.appendString("'");
    }
    data.appendString("],\n");
}

void HTMLSelectElement::addProperty(const char* name, const Unit::Rect& rect,
                                    StringBuilder& data)
{
    data.appendString(name, strlen(name));
    data.appendString(": {");
    addProperty("x", rect.x(), data);
    addProperty("y", rect.y(), data);
    addProperty("width", rect.width(), data);
    addProperty("height", rect.height(), data);
    data.appendString("},\n");
}

void HTMLSelectElement::onDropdownMenuItemSelected(int position)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    if (0 <= position && (size_t)position < list.size()) {
        if (!multiple()) {
            if (!list[position]->selectedness()) {
                setSelectedIndex(position);
                setNeedsFrameTreeBuild();
                fireSelectUpdateNotification();
            }
        } else {
            // TODO: multiple selection
        }
    }
}
} // namespace Starfish
