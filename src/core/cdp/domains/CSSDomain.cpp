/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "CSSDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/Text.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSRule.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/CSSStyleRule.h"

#include "rapidjson/document.h"
#include <cstring>

namespace Starfish {

static int paramNodeId(CDPCommand& cmd)
{
    if (cmd.params() && cmd.params()->HasMember("nodeId") &&
        (*cmd.params())["nodeId"].IsInt()) {
        return (*cmd.params())["nodeId"].GetInt();
    }
    return 0;
}

// Whitelist of CSS properties reported for getComputedStyleForNode. A computed
// declaration resolves each property lazily (updateValue) on access and has no
// up-front enumeration, so a fixed property set is queried. Covers the layout/
// box/typography/visual subset DevTools-style clients commonly read.
static const char* kComputedProperties[] = {
    "display",
    "position",
    "visibility",
    "opacity",
    "float",
    "clear",
    "color",
    "background-color",
    "background-image",
    "width",
    "height",
    "min-width",
    "min-height",
    "max-width",
    "max-height",
    "margin-top",
    "margin-right",
    "margin-bottom",
    "margin-left",
    "padding-top",
    "padding-right",
    "padding-bottom",
    "padding-left",
    "border-top-width",
    "border-right-width",
    "border-bottom-width",
    "border-left-width",
    "border-top-color",
    "border-right-color",
    "border-bottom-color",
    "border-left-color",
    "border-top-style",
    "border-right-style",
    "border-bottom-style",
    "border-left-style",
    "top",
    "right",
    "bottom",
    "left",
    "z-index",
    "box-sizing",
    "overflow-x",
    "overflow-y",
    "font-family",
    "font-size",
    "font-weight",
    "font-style",
    "line-height",
    "text-align",
    "text-decoration",
    "white-space",
    "vertical-align",
    "letter-spacing",
    "word-spacing",
    "flex-direction",
    "flex-wrap",
    "justify-content",
    "align-items",
    "align-content",
    "align-self",
    "flex-grow",
    "flex-shrink",
    "flex-basis",
    "list-style-type",
    "cursor",
    "transform",
    "transform-origin",
};

static std::string paramString(CDPCommand& cmd, const char* key)
{
    if (cmd.params() && cmd.params()->HasMember(key) &&
        (*cmd.params())[key].IsString()) {
        return (*cmd.params())[key].GetString();
    }
    return std::string();
}

// Resolves the main document for this session via the WebView's main browsing
// context (matches getStyleSheetText).
static Document* mainDocument(CDPDispatcher* dispatcher)
{
    WebView* wv = dispatcher->webView();
    BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
    return bc ? bc->document() : nullptr;
}

// Parses a "sheet-N" id into an index. Returns npos on malformed id.
static size_t parseSheetIndex(const std::string& id)
{
    if (id.rfind("sheet-", 0) == 0) {
        return (size_t)atoi(id.c_str() + 6);
    }
    return std::string::npos;
}

// Resolves a "sheet-N" id to the engine CSSStyleSheet (or nullptr).
static CSSStyleSheet* resolveSheet(Document* doc, const std::string& id)
{
    if (!doc) {
        return nullptr;
    }
    size_t idx = parseSheetIndex(id);
    if (idx == std::string::npos) {
        return nullptr;
    }
    GCVector<CSSStyleSheet*>& sheets = doc->styleResolver().sheets();
    if (idx >= sheets.size() || !sheets[idx]) {
        return nullptr;
    }
    return sheets[idx];
}

// Builds a CDP CSSStyle object {cssProperties, shorthandEntries, cssText,
// styleSheetId?} from an engine CSSStyleDeclaration. styleSheetId is added only
// when a non-empty id is supplied.
static rapidjson::Value buildCSSStyle(CSSStyleDeclaration* decl,
                                      const std::string& styleSheetId,
                                      rapidjson::Document::AllocatorType& alloc)
{
    rapidjson::Value style(rapidjson::kObjectType);
    rapidjson::Value cssProps(rapidjson::kArrayType);
    std::string cssText;
    if (decl) {
        String* ct = decl->cssText();
        cssText = ct ? ct->toUTF8NonGCString() : std::string();
        uint32_t n = decl->length();
        for (uint32_t i = 0; i < n; i++) {
            String* nameS = decl->item(i);
            std::string name =
                nameS ? nameS->toUTF8NonGCString() : std::string();
            if (name.empty()) {
                continue;
            }
            String* valS = decl->getPropertyValue(nameS);
            std::string val = valS ? valS->toUTF8NonGCString() : std::string();
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("name",
                            rapidjson::Value(name.c_str(), name.size(), alloc),
                            alloc);
            entry.AddMember("value",
                            rapidjson::Value(val.c_str(), val.size(), alloc),
                            alloc);
            cssProps.PushBack(entry, alloc);
        }
    }
    style.AddMember("cssProperties", cssProps, alloc);
    style.AddMember("shorthandEntries", rapidjson::Value(rapidjson::kArrayType),
                    alloc);
    style.AddMember("cssText",
                    rapidjson::Value(cssText.c_str(), cssText.size(), alloc),
                    alloc);
    if (!styleSheetId.empty()) {
        style.AddMember(
            "styleSheetId",
            rapidjson::Value(styleSheetId.c_str(), styleSheetId.size(), alloc),
            alloc);
    }
    return style;
}

// Builds a CDP CSSRule object {selectorList, origin, styleSheetId, style} from
// an engine StyleRule.
static rapidjson::Value buildCSSRule(StyleRule* sr,
                                     const std::string& styleSheetId,
                                     rapidjson::Document::AllocatorType& alloc)
{
    String* selTextS = sr->selectorList().selectorText();
    std::string sel = selTextS ? selTextS->toUTF8NonGCString() : std::string();

    rapidjson::Value rule(rapidjson::kObjectType);
    rapidjson::Value selectorList(rapidjson::kObjectType);
    rapidjson::Value selectors(rapidjson::kArrayType);
    rapidjson::Value selEntry(rapidjson::kObjectType);
    selEntry.AddMember("text", rapidjson::Value(sel.c_str(), sel.size(), alloc),
                       alloc);
    selectors.PushBack(selEntry, alloc);
    selectorList.AddMember("selectors", selectors, alloc);
    selectorList.AddMember(
        "text", rapidjson::Value(sel.c_str(), sel.size(), alloc), alloc);
    rule.AddMember("selectorList", selectorList, alloc);
    rule.AddMember("origin", "regular", alloc);
    rule.AddMember(
        "styleSheetId",
        rapidjson::Value(styleSheetId.c_str(), styleSheetId.size(), alloc),
        alloc);
    rule.AddMember("style",
                   buildCSSStyle(sr->styleDeclaration(), styleSheetId, alloc),
                   alloc);
    return rule;
}

void CSSDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    NodeRegistry* reg = m_dispatcher->nodeRegistry();

    if (method == "enable") {
        s->cssEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->cssEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getComputedStyleForNode") {
        Node* node = reg->lookup(paramNodeId(cmd));
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        CSSStyleDeclaration* style = node->asElement()->getComputedStyle();

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value props(rapidjson::kArrayType);
        for (size_t i = 0;
             i < sizeof(kComputedProperties) / sizeof(kComputedProperties[0]);
             i++) {
            const char* name = kComputedProperties[i];
            String* nameStr = String::fromUTF8(name, strlen(name));
            String* val = style->getPropertyValue(nameStr);
            std::string v = val ? val->toUTF8NonGCString() : std::string();
            if (v.empty()) {
                continue; // engine returned no value for this property
            }
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("name", rapidjson::Value(name, strlen(name), alloc),
                            alloc);
            entry.AddMember(
                "value", rapidjson::Value(v.c_str(), v.size(), alloc), alloc);
            props.PushBack(entry, alloc);
        }
        result.AddMember("computedStyle", props, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getInlineStylesForNode") {
        Node* node = reg->lookup(paramNodeId(cmd));
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        Element* el = node->asElement();
        InlineCSSStyleDeclaration* inl = el->inlineStyleWithoutCreation();

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value inlineStyle(rapidjson::kObjectType);

        rapidjson::Value cssProps(rapidjson::kArrayType);
        std::string cssText;
        if (inl) {
            String* ct = inl->cssText();
            cssText = ct ? ct->toUTF8NonGCString() : std::string();
            uint32_t n = inl->length();
            for (uint32_t i = 0; i < n; i++) {
                String* nameS = inl->item(i);
                std::string name =
                    nameS ? nameS->toUTF8NonGCString() : std::string();
                if (name.empty()) {
                    continue;
                }
                String* valS = inl->getPropertyValue(nameS);
                std::string val =
                    valS ? valS->toUTF8NonGCString() : std::string();
                rapidjson::Value entry(rapidjson::kObjectType);
                entry.AddMember(
                    "name", rapidjson::Value(name.c_str(), name.size(), alloc),
                    alloc);
                entry.AddMember(
                    "value", rapidjson::Value(val.c_str(), val.size(), alloc),
                    alloc);
                cssProps.PushBack(entry, alloc);
            }
        }
        inlineStyle.AddMember("cssProperties", cssProps, alloc);
        inlineStyle.AddMember("shorthandEntries",
                              rapidjson::Value(rapidjson::kArrayType), alloc);
        inlineStyle.AddMember(
            "cssText", rapidjson::Value(cssText.c_str(), cssText.size(), alloc),
            alloc);
        result.AddMember("inlineStyle", inlineStyle, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getMatchedStylesForNode") {
        Node* node = reg->lookup(paramNodeId(cmd));
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        Element* el = node->asElement();
        Document* doc = el->document();

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);

        // inlineStyle (same shape as getInlineStylesForNode)
        InlineCSSStyleDeclaration* inl = el->inlineStyleWithoutCreation();
        result.AddMember("inlineStyle",
                         buildCSSStyle(inl, std::string(), alloc), alloc);

        rapidjson::Value matched(rapidjson::kArrayType);
        if (doc) {
            GCVector<CSSStyleSheet*>& sheets = doc->styleResolver().sheets();
            for (size_t si = 0; si < sheets.size(); si++) {
                CSSStyleSheet* sheet = sheets[si];
                if (!sheet) {
                    continue;
                }
                sheet->parseSheetIfneeds();
                std::string sheetId = "sheet-" + std::to_string(si);
                GCVector<std::pair<StyleRule*, ResourceURL*>>& rules =
                    sheet->styleRules();
                for (size_t ri = 0; ri < rules.size(); ri++) {
                    StyleRule* sr = rules[ri].first;
                    if (!sr) {
                        continue;
                    }
                    String* selText = sr->selectorList().selectorText();
                    if (!selText || selText->length() == 0) {
                        continue;
                    }
                    bool isMatch = false;
                    try {
                        isMatch = el->matches(selText);
                    } catch (...) {
                        isMatch = false; // unsupported selector
                    }
                    if (!isMatch) {
                        continue;
                    }
                    std::string sel = selText->toUTF8NonGCString();

                    rapidjson::Value rule(rapidjson::kObjectType);
                    // selectorList
                    rapidjson::Value selectorList(rapidjson::kObjectType);
                    rapidjson::Value selectors(rapidjson::kArrayType);
                    rapidjson::Value selEntry(rapidjson::kObjectType);
                    selEntry.AddMember(
                        "text",
                        rapidjson::Value(sel.c_str(), sel.size(), alloc),
                        alloc);
                    selectors.PushBack(selEntry, alloc);
                    selectorList.AddMember("selectors", selectors, alloc);
                    selectorList.AddMember(
                        "text",
                        rapidjson::Value(sel.c_str(), sel.size(), alloc),
                        alloc);
                    rule.AddMember("selectorList", selectorList, alloc);
                    rule.AddMember("origin", "regular", alloc);
                    rule.AddMember("styleSheetId",
                                   rapidjson::Value(sheetId.c_str(),
                                                    sheetId.size(), alloc),
                                   alloc);
                    rule.AddMember(
                        "style",
                        buildCSSStyle(sr->styleDeclaration(), sheetId, alloc),
                        alloc);

                    rapidjson::Value matching(rapidjson::kArrayType);
                    matching.PushBack(0, alloc);

                    rapidjson::Value mr(rapidjson::kObjectType);
                    mr.AddMember("rule", rule, alloc);
                    mr.AddMember("matchingSelectors", matching, alloc);
                    matched.PushBack(mr, alloc);
                }
            }
        }
        result.AddMember("matchedCSSRules", matched, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "getStyleSheetText") {
        std::string id = paramString(cmd, "styleSheetId");
        // Expect "sheet-N"
        size_t idx = std::string::npos;
        if (id.rfind("sheet-", 0) == 0) {
            idx = (size_t)atoi(id.c_str() + 6);
        }
        WebView* wv = m_dispatcher->webView();
        BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
        Document* doc = bc ? bc->document() : nullptr;
        if (!doc || idx == std::string::npos) {
            cmd.sendError(-32000, "No stylesheet with given id found");
            return;
        }
        GCVector<CSSStyleSheet*>& sheets = doc->styleResolver().sheets();
        if (idx >= sheets.size() || !sheets[idx]) {
            cmd.sendError(-32000, "No stylesheet with given id found");
            return;
        }
        CSSStyleSheet* sheet = sheets[idx];
        sheet->parseSheetIfneeds();
        // Reconstruct text by joining each rule's cssText (the original source
        // string is discarded after parsing).
        std::string text;
        unsigned n = sheet->length();
        for (unsigned i = 0; i < n; i++) {
            CSSRule* r = sheet->item(i);
            if (!r) {
                continue;
            }
            String* ct = r->cssText();
            if (ct) {
                if (!text.empty()) {
                    text += "\n";
                }
                text += ct->toUTF8NonGCString();
            }
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "text", rapidjson::Value(text.c_str(), text.size(), alloc), alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "createStyleSheet") {
        // Create an empty <style> element and append it to <head> (falling back
        // to documentElement). The engine generates a CSSStyleSheet for it and
        // registers it in styleResolver().sheets(); we return its "sheet-N" id.
        Document* doc = mainDocument(m_dispatcher);
        if (!doc) {
            cmd.sendError(-32000, "No document");
            return;
        }
        Element* styleEl = doc->createElement(String::fromUTF8("style", 5));
        if (!styleEl) {
            cmd.sendError(-32000, "Failed to create stylesheet");
            return;
        }
        Node* parent =
            doc->head() ? (Node*)doc->head() : (Node*)doc->documentElement();
        if (!parent) {
            cmd.sendError(-32000, "No insertion point for stylesheet");
            return;
        }
        parent->appendChild(styleEl);

        CSSStyleSheet* sheet = nullptr;
        if (styleEl->isHTMLStyleElement()) {
            sheet = styleEl->asHTMLStyleElement()->generatedSheet();
        }
        // Locate the sheet's index within the resolver sheet list.
        GCVector<CSSStyleSheet*>& sheets = doc->styleResolver().sheets();
        size_t idx = std::string::npos;
        for (size_t i = 0; i < sheets.size(); i++) {
            if (sheets[i] == sheet && sheet) {
                idx = i;
                break;
            }
        }
        if (idx == std::string::npos) {
            cmd.sendError(-32000, "Failed to register stylesheet");
            return;
        }
        std::string sheetId = "sheet-" + std::to_string(idx);

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "styleSheetId",
            rapidjson::Value(sheetId.c_str(), sheetId.size(), alloc), alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "addRule") {
        std::string id = paramString(cmd, "styleSheetId");
        std::string ruleText = paramString(cmd, "ruleText");
        Document* doc = mainDocument(m_dispatcher);
        CSSStyleSheet* sheet = resolveSheet(doc, id);
        if (!sheet) {
            cmd.sendError(-32000, "No stylesheet with given id found");
            return;
        }
        sheet->parseSheetIfneeds();
        unsigned index = sheet->length();
        try {
            sheet->insertRule(
                String::fromUTF8(ruleText.data(), ruleText.size()), index);
        } catch (...) {
            cmd.sendError(-32000, "Failed to parse the rule");
            return;
        }

        // Fetch the freshly inserted rule's engine StyleRule for the response.
        StyleRule* sr = nullptr;
        CSSRule* wrapper = sheet->item(index);
        if (wrapper && wrapper->type() == CSSRule::Type::STYLE_RULE) {
            sr = ((CSSStyleRule*)wrapper)->styleRule();
        }
        if (!sr) {
            cmd.sendError(-32000, "Failed to insert the rule");
            return;
        }

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("rule", buildCSSRule(sr, id, alloc), alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "setStyleSheetText") {
        std::string id = paramString(cmd, "styleSheetId");
        std::string text = paramString(cmd, "text");
        Document* doc = mainDocument(m_dispatcher);
        CSSStyleSheet* sheet = resolveSheet(doc, id);
        if (!sheet) {
            cmd.sendError(-32000, "No stylesheet with given id found");
            return;
        }
        // Replace the backing <style> element's text content and regenerate the
        // sheet through the normal engine pipeline (recalc is triggered).
        Node* origin = sheet->origin();
        if (!origin || !origin->isHTMLStyleElement()) {
            cmd.sendError(-32000,
                          "Stylesheet is not backed by a <style> element");
            return;
        }
        HTMLStyleElement* styleEl = origin->asHTMLStyleElement();
        // Remove existing children, then append the new text node.
        while (styleEl->firstChild()) {
            styleEl->removeChild(styleEl->firstChild());
        }
        styleEl->appendChild(
            doc->createTextNode(String::fromUTF8(text.data(), text.size())));
        styleEl->generateStyleSheet();

        cmd.sendResultEmpty();
        return;
    }

    if (method == "setStyleTexts") {
        // edits: [{styleSheetId, range:{startLine,...}, text}]
        // We map each edit's range.startLine (or array order) to the rule index
        // within the sheet and replace that rule's declaration text. The
        // reparse (StyleRuleCSSStyleDeclaration::setCssText) triggers a full
        // recalc.
        Document* doc = mainDocument(m_dispatcher);
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value styles(rapidjson::kArrayType);

        if (cmd.params() && cmd.params()->HasMember("edits") &&
            (*cmd.params())["edits"].IsArray()) {
            const rapidjson::Value& edits = (*cmd.params())["edits"];
            for (rapidjson::SizeType e = 0; e < edits.Size(); e++) {
                const rapidjson::Value& edit = edits[e];
                std::string id = (edit.HasMember("styleSheetId") &&
                                  edit["styleSheetId"].IsString())
                                     ? edit["styleSheetId"].GetString()
                                     : std::string();
                std::string text =
                    (edit.HasMember("text") && edit["text"].IsString())
                        ? edit["text"].GetString()
                        : std::string();
                unsigned ruleIndex = 0;
                if (edit.HasMember("range") && edit["range"].IsObject() &&
                    edit["range"].HasMember("startLine") &&
                    edit["range"]["startLine"].IsInt()) {
                    int sl = edit["range"]["startLine"].GetInt();
                    if (sl > 0) {
                        ruleIndex = (unsigned)sl;
                    }
                }
                CSSStyleSheet* sheet = resolveSheet(doc, id);
                if (!sheet) {
                    continue;
                }
                sheet->parseSheetIfneeds();
                if (ruleIndex >= sheet->length()) {
                    continue;
                }
                CSSRule* wrapper = sheet->item(ruleIndex);
                if (!wrapper || wrapper->type() != CSSRule::Type::STYLE_RULE) {
                    continue;
                }
                CSSStyleRule* styleRule = (CSSStyleRule*)wrapper;
                CSSStyleDeclaration* decl = styleRule->style();
                if (!decl) {
                    continue;
                }
                decl->setCssText(String::fromUTF8(text.data(), text.size()));
                styles.PushBack(
                    buildCSSStyle(styleRule->styleRule()->styleDeclaration(),
                                  id, alloc),
                    alloc);
            }
        }
        result.AddMember("styles", styles, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "setPropertyText") {
        // Convenience single-property setter on a rule (not a standard CDP
        // method). params: {styleSheetId, ruleIndex, name, value, priority?}
        std::string id = paramString(cmd, "styleSheetId");
        std::string name = paramString(cmd, "name");
        std::string value = paramString(cmd, "value");
        std::string priority = paramString(cmd, "priority");
        int ruleIndex = 0;
        if (cmd.params() && cmd.params()->HasMember("ruleIndex") &&
            (*cmd.params())["ruleIndex"].IsInt()) {
            ruleIndex = (*cmd.params())["ruleIndex"].GetInt();
        }
        Document* doc = mainDocument(m_dispatcher);
        CSSStyleSheet* sheet = resolveSheet(doc, id);
        if (!sheet) {
            cmd.sendError(-32000, "No stylesheet with given id found");
            return;
        }
        sheet->parseSheetIfneeds();
        if (ruleIndex < 0 || (unsigned)ruleIndex >= sheet->length()) {
            cmd.sendError(-32000, "Rule index out of range");
            return;
        }
        CSSRule* wrapper = sheet->item((unsigned)ruleIndex);
        if (!wrapper || wrapper->type() != CSSRule::Type::STYLE_RULE) {
            cmd.sendError(-32000, "Rule is not a style rule");
            return;
        }
        CSSStyleRule* styleRule = (CSSStyleRule*)wrapper;
        CSSStyleDeclaration* decl = styleRule->style();
        if (!decl) {
            cmd.sendError(-32000, "Rule has no style");
            return;
        }
        // Apply the property on the engine declaration, then re-serialize and
        // push the whole text back through the rule wrapper's setCssText, which
        // re-parses the rule and triggers a rule-set recalc. (setProperty alone
        // on a rule-backed declaration does not invalidate the rule set.)
        CSSStyleDeclaration* engineDecl =
            styleRule->styleRule()->styleDeclaration();
        engineDecl->setProperty(
            String::fromUTF8(name.data(), name.size()),
            String::fromUTF8(value.data(), value.size()),
            String::fromUTF8(priority.data(), priority.size()));
        String* combined = engineDecl->cssText();
        decl->setCssText(combined ? combined : String::emptyString);

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "style",
            buildCSSStyle(styleRule->styleRule()->styleDeclaration(), id,
                          alloc),
            alloc);
        cmd.sendResult(result, out);
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
