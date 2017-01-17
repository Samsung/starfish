/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "Document.h"
#include "Traverse.h"

#include "dom/builder/html/HTMLDocumentBuilder.h"
#include "layout/FrameDocument.h"
#include "dom/Attribute.h"
#include "platform/message_loop/MessageLoop.h"
#include "loader/ImageResource.h"

#ifdef STARFISH_EXP
#include "dom/DOMImplementation.h"
#endif

namespace StarFish {

Document::Document(Window* window, ScriptBindingInstance* scriptBindingInstance, URL* uri, String* charSet, bool isXMLDocument, bool doesParticipateInRendering)
    : Node(this, scriptBindingInstance)
    , m_inParsing(false)
    , m_didLoadBrokenImage(false)
    , m_isXMLDocument(isXMLDocument)
    , m_doesParticipateInRendering(doesParticipateInRendering)
    , m_compatibilityMode(Document::NoQuirksMode)
    , m_window(window)
    , m_documentURI(uri)
    , m_charset(charSet)
    , m_resourceLoader(*this)
    , m_styleResolver(*this)
    , m_documentBuilder(nullptr)
    , m_pageVisibilityState(PageVisibilityStateVisible)
    , m_domVersion(0)
#ifdef STARFISH_TIZEN
    , m_tizenWidgetTransparentBackground(0)
#endif
{
    m_scriptBindingInstance = scriptBindingInstance;
    setStyle(m_styleResolver.resolveDocumentStyle(this));

    CSSStyleSheet* userAgentStyleSheet = new CSSStyleSheet(this, String::emptyString);
    userAgentStyleSheet->parseSheetIfneeds();
    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("html"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("head"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("style"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("script"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("meta"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("body"), CSSSelector::PseudoType::PseudoNone, document());

        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("div"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("p"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("span"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::InlineDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("img"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::InlineDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("table"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::BorderCollapse);
        pair.setValueKind(CSSStyleValuePair::ValueKind::BorderCollapseValueKind);
        data.m_borderCollapse = BorderCollapseValue::SeparateBorderCollapseValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::BorderSpacing);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("2px");
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("caption"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableCaptionDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
        data.m_side = SideValue::CenterSideValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("thead"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableHeaderGroupDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("tbody"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableRowGroupDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("tfoot"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableFooterGroupDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("th"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableCellDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("td"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableCellDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("tr"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableRowDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("col"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableColumnDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("colgroup"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::TableColumnGroupDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("h1"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontSize);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("2em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0.67em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0.67em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("h2"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontSize);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1.5em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0.83em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0.83em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("h3"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontSize);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1.17em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("h4"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1.33em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1.33em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("h5"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontSize);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0.83em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1.67em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1.67em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("h6"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontSize);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0.67em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("2.33em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("2.33em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("ul"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("0em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::PaddingLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("40px");
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("li"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSSelector::Type::Tag, String::createASCIIString("strong"), CSSSelector::PseudoType::PseudoNone, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::InlineDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::FontWeight);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
        data.m_fontWeight = FontWeightValue::BoldFontWeightValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    m_styleResolver.addSheet(userAgentStyleSheet);

    auto df = new FrameDocument(this);
    setFrame(df);

#ifdef STARFISH_EXP
    m_domImplementation = new DOMImplementation(m_window, m_scriptBindingInstance);
#endif

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("Document::~Document\n");
    }, NULL, NULL, NULL);
}

void Document::open()
{
    m_resourceLoader.markDocumentOpenState();

    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->build(documentURI());

}

void Document::resumeDocumentParsing()
{
    window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
        Document* document = (Document*)data;
        STARFISH_ASSERT(document->m_documentBuilder);
        document->m_documentBuilder->resume();
    }, this);
}

void Document::notifyDomContentLoaded()
{
    String* eventType = window()->starFish()->staticStrings()->m_DOMContentLoaded.localName();
    Event* e = new Event(eventType, EventInit(true, true));
    EventTarget::dispatchEvent(e);

    m_resourceLoader.notifyEndParseDocument();
    m_documentBuilder = nullptr;

#ifdef STARFISH_ENABLE_MULTIMEDIA
    // Trigger HTMLMediaElement's preload
    // FIXME : Should consider detached HTMLMediaElements as well
    std::vector<Element*, gc_allocator_ignore_off_page<Element*>> mediaElements;
    Traverse::getherDescendant(mediaElements, this, [&](Node* child) {
        if (child->isElement() && child->asElement()->isHTMLElement() && child->asElement()->asHTMLElement()->isHTMLMediaElement()) {
            return true;
        } else
            return false;
    });
    for (size_t i = 0; i < mediaElements.size(); i++) {
        HTMLMediaElement* target = mediaElements[i]->asHTMLElement()->asHTMLMediaElement();
        target->onDOMContentLoaded();
    }
#endif

    STARFISH_LOG_INFO("Document::notifyDomContentLoaded\n");
    if (m_compatibilityMode != NoQuirksMode) {
        STARFISH_LOG_ERROR("%s is not specified standard mode doctype. currently, StarFish could not support quirks mode.\n", m_documentURI->urlString()->utf8Data());
        STARFISH_LOG_ERROR("You could got unexpected rendering result. please use standard mode doctype[<!DOCTYPE html>]\n");
    }

    // if there is a fragment identifier, set cssTarget.
    String* fragment = documentURI()->getHash();
    if (!fragment->equals(String::emptyString))
        window()->processUrlFragment(fragment->substring(1, fragment->length()-1));
}

void Document::close()
{
    Element* bodyElem = bodyElement();
    if (bodyElem) {
        String* eventType = window()->starFish()->staticStrings()->m_unload.localName();
        Event* e = new Event(eventType, EventInit(false, false));
        EventTarget::dispatchEvent(bodyElem, e);
    }

    resourceLoader()->clear();

    while (m_activeNetworkRequests.size()) {
        m_activeNetworkRequests.back()->abort();
    }
}

String* Document::nodeName()
{
    return window()->starFish()->staticStrings()->m_documentLocalName.string();
}

String* Document::localName()
{
    return window()->starFish()->staticStrings()->m_documentLocalName.string();
}

Node* Document::clone()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

Element* Document::getElementById(String* id)
{
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isElement() && child->asElement()->isHTMLElement() && child->asElement()->asHTMLElement()->id()->equals(id)) {
            return true;
        } else
            return false;
    });
}

DocumentFragment* Document::createDocumentFragment()
{
    return new DocumentFragment(this);
}

Element* Document::createElement(AtomicString localName, bool shouldCheckName)
{
    if (shouldCheckName && !QualifiedName::checkNameProductionRule(localName.string(), localName.string()->length()))
        throw new DOMException(document()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

    return new NamedElement(this, QualifiedName(AtomicString(), localName));
}

Text* Document::createTextNode(String* data)
{
    return new Text(this, data);
}

CDataSection* Document::createCDataSectionNode(String* data)
{
    return new CDataSection(this, data);
}

Comment* Document::createComment(String* data)
{
    return new Comment(this, data);
}

Attr* Document::createAttribute(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.localName(), localName.localName()->length()))
        throw new DOMException(document()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

    return new Attr(this, scriptBindingInstance(), localName);
}

HTMLHtmlElement* Document::rootElement()
{
    // root element of html document is HTMLHtmlElement
    // https://www.w3.org/TR/html-markup/html.html
    Node* n = firstChild();
    while (n) {
        if (n->isElement() && n->asElement() && n->asElement()->isHTMLElement() && n->asElement()->asHTMLElement()->isHTMLHtmlElement()) {
            return n->asElement()->asHTMLElement()->asHTMLHtmlElement();
        }
        n = n->nextSibling();
    }
    return nullptr;
}

Element* Document::documentElement()
{
    if (isXMLDocument()) {
        Node* n = firstChild();
        while (n) {
            if (n && n->isElement() && !n->isComment()) {
                return n->asElement();
            }
            n = n->nextSibling();
        }
    }
    return rootElement();
}

HTMLHeadElement* Document::headElement()
{
    Node* head = childMatchedBy(this, [](Node* nd) -> bool {
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLHeadElement()) {
            return true;
        }
        return false;
    });
    if (head) {
        return head->asElement()->asHTMLElement()->asHTMLHeadElement();
    }
    return nullptr;
}

HTMLBodyElement* Document::bodyElement()
{
    Node* body = childMatchedBy(this, [](Node* nd) -> bool {
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            return true;
        }
        return false;
    });
    if (body) {
        return body->asElement()->asHTMLElement()->asHTMLBodyElement();
    }
    return nullptr;
}

bool Document::hidden() const
{
    return m_pageVisibilityState == PageVisibilityState::PageVisibilityStateHidden;
}

String* Document::visibilityState()
{
    switch (m_pageVisibilityState) {
    case PageVisibilityState::PageVisibilityStateHidden:
        return String::createASCIIString("hidden");
    case PageVisibilityState::PageVisibilityStatePrerender:
        return String::createASCIIString("prerender");
    case PageVisibilityState::PageVisibilityStateUnloaded:
        return String::createASCIIString("unloaded");
    case PageVisibilityState::PageVisibilityStateVisible:
        return String::createASCIIString("visible");
    }

    STARFISH_ASSERT_NOT_REACHED();
    return String::emptyString;
}

void Document::setVisibleState(PageVisibilityState visibilityState)
{
    if (m_pageVisibilityState != visibilityState) {
        m_pageVisibilityState = visibilityState;
        String* eventType = window()->starFish()->staticStrings()->m_visibilitychange.localName();
        Event* e = new Event(eventType, EventInit(true, false));
        EventTarget::dispatchEvent(this->asNode(), e);
    }
}

void Document::didNodeInserted(Node* parent, Node* newChild)
{
    Node::didNodeInserted(parent, newChild);
    updateDOMVersion();
    invalidNamedAccessCacheIfNeeded();
}

void Document::didNodeRemoved(Node* parent, Node* oldChild)
{
    Node::didNodeRemoved(parent, oldChild);
    updateDOMVersion();
    invalidNamedAccessCacheIfNeeded();
}

HTMLCollection* Document::namedAccess(String* name)
{
    for (size_t i = 0; i < m_namedAccessActiveHTMLCollectionList.size(); i ++) {
        if (m_namedAccessActiveHTMLCollectionList.at(i).first->equals(name)) {
            return m_namedAccessActiveHTMLCollectionList[i].second;
        }
    }

    // https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
    auto filter = [](Node* node, void* data)
    {
        QualifiedName* qualifiedName = (QualifiedName*)data;

        if (node->isElement() && node->asElement()->isHTMLElement()) {

            // a, applet, area, embed, form, frameset, img, or object elements that have a name content attribute whose value is name, or
            QualifiedName nm = node->asElement()->asHTMLElement()->name();
            StaticStrings* ss = node->document()->window()->starFish()->staticStrings();
            bool shouldConsiderNameAttribute = false;
            if (nm == ss->m_aTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_areaTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_embedTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_formTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_framesetTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_imgTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_objectTagName) {
                shouldConsiderNameAttribute = true;
            }

            if (shouldConsiderNameAttribute) {
                if (node->asElement()->getAttribute(ss->m_name)->equals(qualifiedName->localName())) {
                    return true;
                }
            }

            // HTML elements that have an id content attribute whose value is name
            if (node->asElement()->id()->equals(qualifiedName->localName())) {
                return true;
            }
        }
        return false;
    };

    // TODO
    // now, we always create html collection for every query
    // but, if len(result) == 0:
    // we should not need to make HTMLCollection
    // just return nullptr;
    void* ptr = new QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), name));
    auto list = new HTMLCollection(scriptBindingInstance(), document(), filter, ptr, true);
    m_namedAccessActiveHTMLCollectionList.push_back(std::make_pair(name, list));

    return list;
}

void Document::invalidNamedAccessCacheIfNeeded()
{
    for (size_t i = 0; i < m_namedAccessActiveHTMLCollectionList.size(); i ++) {
        m_namedAccessActiveHTMLCollectionList.at(i).second->getNodeListImpl().invalidateCache();
    }
}

Element* Document::elementFromPoint(float x, float y)
{
    Node* node = window()->hitTest(x, y);
    while (node) {
        if (node->isElement()) {
            return node->asElement();
        }
        node = node->parentNode();
    }

    return rootElement();
}

ImageData* Document::brokenImage()
{
    if (m_didLoadBrokenImage) {
        return m_brokenImage;
    } else {
        String* brokenImg = String::fromUTF8("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABQAAAAUCAYAAACNiR0NAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4AYQCBEZPGjJdQAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAAAVSURBVDjLY2AYBaNgFIyCUTAKqAMABlQAAUOHH5wAAAAASUVORK5CYII=");
        ImageResource* res = resourceLoader()->fetchImage(URL::createURL(String::emptyString, brokenImg));
        res->request(Resource::ResourceRequestSyncLevel::AlwaysSync);
        m_brokenImage = res->imageData();
        m_didLoadBrokenImage = true;
        return m_brokenImage;
    }
}

QualifiedName Document::createAttributeName(String* name)
{
    if (isXMLDocument()) {
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(window()->starFish(), name));
    } else {
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(window()->starFish(), name));
    }
}

Element* Document::activeElement()
{
    // TODO
    return this->bodyElement()->asElement();
}

bool Document::hasFocus() const
{
    // TODO
    return true;
}

}
