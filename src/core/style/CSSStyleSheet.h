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

#ifndef __StarfishCSSStyleSheet__
#define __StarfishCSSStyleSheet__

#include "core/style/StyleSheet.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/CSSStyleSheetInit.h"

namespace Starfish {

class CSSImportRule;
class CSSRule;
class CSSRuleList;
class MediaList;
class MediaQueryEvaluator;
class MediaQuerySet;
class Node;
class StyleRule;
class StyleRuleBase;
class StyleRuleImport;
class StyleRuleKeyframes;
class URL;

class RuleSetData
    : public GCUnorderedMap<AtomicString,
                            GCVector<std::pair<StyleRule*, ResourceURL*>>> {
public:
    void insert(const std::pair<AtomicString,
                                std::pair<StyleRule*, ResourceURL*>>& pair)
    {
        auto iter = find(pair.first);
        if (iter != end()) {
            iter.value().push_back(pair.second);
        } else {
            GCVector<std::pair<StyleRule*, ResourceURL*>> v;
            v.push_back(pair.second);
            GCUnorderedMap<AtomicString,
                           GCVector<std::pair<StyleRule*, ResourceURL*>>>::
                insert(std::make_pair(pair.first, std::move(v)));
        }
    }
};

class RuleSet : public gc {
public:
    RuleSetData& idRules()
    {
        return m_idRules;
    }

    RuleSetData& classRules()
    {
        return m_classRules;
    }

    RuleSetData& tagRules()
    {
        return m_tagRules;
    }

    GCVector<std::pair<StyleRule*, ResourceURL*>>& universalRules()
    {
        return m_universalRules;
    }

    GCVector<StyleRuleKeyframes*>& keyframes()
    {
        return m_keyframes;
    }

    void clear()
    {
        m_idRules.clear();
        m_classRules.clear();
        m_tagRules.clear();
        m_universalRules.clear();
    }

private:
    RuleSetData m_idRules;
    RuleSetData m_classRules;
    RuleSetData m_tagRules;
    GCVector<std::pair<StyleRule*, ResourceURL*>> m_universalRules;
    GCVector<StyleRuleKeyframes*> m_keyframes;
};

class CSSStyleSheet : public StyleSheet {
public:
    // Constructor for constructable stylesheets
    CSSStyleSheet(ExecutionContext* executionContext,
                  const CSSStyleSheetInit& options = {});

    // Constructor for stylesheets from style/link elements
    CSSStyleSheet(Node* origin, String* str);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleSheet() const override;

    void addRule(StyleRuleBase* rule);

    ResourceURL* url();
    Node* origin()
    {
        return m_origin;
    }
    Node* root()
    {
        return m_root;
    }

    void parseSheetIfneeds();

    GCVector<std::pair<StyleRule*, ResourceURL*>>& styleRules()
    {
        return m_styleRules;
    }

    GCVector<StyleRuleImport*>& importRules()
    {
        return m_importRules;
    }

    GCVector<StyleRuleKeyframes*>& keyframes()
    {
        return m_keyframes;
    }

    GCVector<StyleRuleBase*>& childRules()
    {
        return m_childRules;
    }

    CSSRule* ownerRule()
    {
        return m_ownerRule;
    }

    void clearOwnerRule()
    {
        m_ownerRule = nullptr;
    }

    void clearAllRules()
    {
        m_childRuleWrappers.clear();
        m_childRules.clear();
        m_importRules.clear();
        clearStyleRules();
        clearKeyframesRules();
    }

    // Separate from clearAllRules() -- replaceSync()/replace() parse the
    // replacement text (which registers its own @namespace declarations)
    // *before* calling clearAllRules(), to keep "parse first, commit after"
    // atomicity for the rule list. Namespace state has no such commit step
    // (registerNamespace() applies immediately during parsing, see
    // CSSParser::parseNamespaceRule()), so it must be cleared *before*
    // parsing the replacement text instead, or the old namespaces would
    // leak into resolving the new text's own prefixed selectors.
    void clearNamespaces()
    {
        m_namespacePrefixMap.clear();
        m_defaultNamespaceURI = Optional<AtomicString>();
    }

    void clearStyleRules()
    {
        m_styleRules.clear();
        m_styleRules.shrink_to_fit();
    }

    void clearKeyframesRules()
    {
        m_keyframes.clear();
        m_keyframes.shrink_to_fit();
    }

    void willRemovedFromDocument();
    void willAddToDocument();

    void setOwnerRule(CSSRule* ownerRule);
    void sortStyleRulesBySpecificity();
    bool matchesMediaQueries(const MediaQueryEvaluator& evaluator,
                             MediaQuerySet* mediaQueres,
                             MediaQueryResultList* viewportDependentResult,
                             MediaQueryResultList* deviceDependentResult);
    void collectRulesFromImportedSheet(
        GCVector<StyleRuleImport*>& rules,
        GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>>& webFonts,
        MediaQueryResultList* viewportDependentResult,
        MediaQueryResultList* deviceDependentResult);
    void collectStyleRules(
        GCVector<StyleRuleBase*>& rules,
        GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>>& webFonts,
        ResourceURL* url,
        MediaQueryResultList* viewportDependentResult = nullptr,
        MediaQueryResultList* deviceDependentResult = nullptr);

    /* DOM APIs */
    String* type() const override
    {
        return String::fromUTF8("text/css");
    }

    String* href() const override;

    CSSStyleSheet* parentStyleSheet() const override;

    String* title() const override;

    Optional<ElementOrProcessingInstruction> ownerNode() const override;

    unsigned length() const;
    CSSRule* item(unsigned index);
    StyleRuleBase* ruleAt(unsigned index) const;

    bool wrapperInsertRule(StyleRuleBase* rule, unsigned index);
    bool wrapperDeleteRule(unsigned index);

    MediaQuerySet* mediaQuerySet()
    {
        return m_mediaQuerySet;
    }

    void setMediaQuerySet(MediaQuerySet* mediaQuerySet);
    MediaList* media() override;

    CSSRuleList* cssRules();
    CSSRuleList* rules()
    {
        return cssRules();
    }
    unsigned insertRule(String* ruleString, unsigned index);
    void deleteRule(unsigned index);
    void replaceSync(String* text);
    Promise* replace(String* text);

    bool disabled() override;
    void setDisabled(bool disabled) override;

    // https://drafts.csswg.org/css-namespaces/ -- prefix -> namespace URI,
    // scoped to this stylesheet only (a prefix declared in one stylesheet is
    // not visible from another). An empty `prefix` registers the *default*
    // namespace instead (`@namespace "uri";`, no prefix token) -- stored
    // separately from the prefix map since it needs its own presence/absence
    // state (`@namespace "";` legitimately declares an empty-string default
    // namespace, which must stay distinguishable from "no default namespace
    // declared at all").
    void registerNamespace(const AtomicString& prefix, const AtomicString& uri);
    Optional<AtomicString> namespaceURIFromPrefix(
        const AtomicString& prefix) const;
    Optional<AtomicString> defaultNamespaceURI() const
    {
        return m_defaultNamespaceURI;
    }
    bool hasNamespacePrefix(const AtomicString& prefix) const
    {
        return m_namespacePrefixMap.find(prefix) != m_namespacePrefixMap.end();
    }

protected:
    void syncChildRuleWrappers();
    void notifyStyleSheetChanged();
    // m_stringString != String::emptyString means we need to parse style sheet
    // before access style rules.
    String* m_sourceString;
    Node* m_origin;
    Node* m_root;
    CSSRule* m_ownerRule;
    CSSRuleList* m_ruleList;
    MediaQuerySet* m_mediaQuerySet;
    MediaList* m_mediaWrapper;

    GCVector<StyleRuleBase*> m_childRules;
    GCVector<StyleRuleImport*> m_importRules;
    GCVector<std::pair<StyleRule*, ResourceURL*>> m_styleRules;
    GCVector<StyleRuleKeyframes*> m_keyframes;
    GCVector<CSSRule*> m_childRuleWrappers;

    // @namespace prefix -> URI, scoped to this stylesheet. See
    // registerNamespace()/namespaceURIFromPrefix() above.
    GCUnorderedMap<AtomicString, AtomicString> m_namespacePrefixMap;
    Optional<AtomicString> m_defaultNamespaceURI;

    bool m_disabled : 1;
};

} /* namespace Starfish */

#endif /* __StarfishCSSStyleSheet__ */
