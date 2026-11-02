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
#include "AuditsNotifier.h"
#include "CDPCommand.h"
#include "CDPDispatcher.h"
#include "CDPServer.h"
#include "CDPSession.h"
#include "NodeRegistry.h"
#include "TargetContext.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/Node.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"

#include "rapidjson/document.h"
#include <cctype>
#include <cstring>
#include <set>

namespace Starfish {

namespace {

    constexpr const char* kIssueAddedEvent = "Audits.issueAdded";
    constexpr const char* kElementAccessibilityIssue =
        "ElementAccessibilityIssue";
    constexpr const char* kDisallowedSelectChild = "DisallowedSelectChild";
    constexpr const char* kDetailsKey = "elementAccessibilityIssueDetails";
    constexpr const char* kTabindexAttribute = "tabindex";
    constexpr const char* kContenteditableAttribute = "contenteditable";

    // Elements allowed directly inside a customizable <select> besides
    // <option> and <optgroup>.
    const char* const kSelectChildTags[] = { "div",      "hr",     "button",
                                             "datalist", "script", "template",
                                             "style" };

    // Elements allowed inside an <option> as phrasing content.
    const char* const kPhrasingTags[] = {
        "abbr", "audio", "b",     "bdi",  "bdo",    "br",  "canvas",
        "cite", "code",  "data",  "dfn",  "em",     "i",   "img",
        "kbd",  "label", "mark",  "q",    "rp",     "rt",  "ruby",
        "s",    "samp",  "small", "span", "strong", "sub", "sup",
        "svg",  "time",  "u",     "var",  "video",  "wbr", "slot"
    };

    // Elements that are interactive by their tag alone.
    const char* const kInteractiveTags[] = { "a",      "button",   "input",
                                             "select", "textarea", "iframe",
                                             "embed",  "details",  "summary",
                                             "label" };

    bool inList(const std::string& tag, const char* const* list, size_t size)
    {
        for (size_t i = 0; i < size; i++) {
            if (tag == list[i]) {
                return true;
            }
        }
        return false;
    }

    template <size_t N>
    bool inList(const std::string& tag, const char* const (&list)[N])
    {
        return inList(tag, list, N);
    }

    std::string tagOf(Element* element)
    {
        std::string tag = element->localName()->toUTF8NonGCString();
        for (char& c : tag) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return tag;
    }

    bool hasAttr(Element* element, const char* name)
    {
        return element->getAttribute(String::fromUTF8(name, strlen(name)))
            .hasValue();
    }

    // Attributes that make any element interactive.
    bool hasInteractiveAttribute(Element* element)
    {
        return hasAttr(element, kTabindexAttribute) ||
               hasAttr(element, kContenteditableAttribute);
    }

    bool isInteractive(Element* element, const std::string& tag)
    {
        // A <label> only groups text, so the tag list does not apply to it.
        if (tag == "label") {
            return hasInteractiveAttribute(element);
        }
        if (tag == "a" && !hasAttr(element, "href")) {
            return hasInteractiveAttribute(element);
        }
        return inList(tag, kInteractiveTags) ||
               hasInteractiveAttribute(element);
    }

    class Scanner {
    public:
        Scanner(NodeRegistry* registry, std::vector<AuditsIssue>* out,
                const std::vector<const void*>* closedSelects)
            : m_registry(registry)
            , m_out(out)
            , m_closedSelects(closedSelects)
        {
        }

        void scan(Node* root)
        {
            for (Node* n = root; n; n = nextInTree(n, root)) {
                if (n->isElement() && tagOf(n->asElement()) == "select") {
                    checkSelect(n->asElement());
                    reportClosedBy(n->asElement());
                }
            }
        }

    private:
        static Node* nextInTree(Node* node, Node* root)
        {
            if (node->firstChild()) {
                return node->firstChild();
            }
            while (node && node != root) {
                if (node->nextSibling()) {
                    return node->nextSibling();
                }
                node = node->parentNode();
            }
            return nullptr;
        }

        static bool isInside(Node* node, Node* ancestor)
        {
            for (; ancestor && node; node = node->parentNode()) {
                if (node == ancestor) {
                    return true;
                }
            }
            return false;
        }

        // One issue for each <input> the parser moved out of this select.
        void reportClosedBy(Element* select)
        {
            for (const void* closed : *m_closedSelects) {
                if (closed == select) {
                    report(select, kDisallowedSelectChild, false);
                }
            }
        }

        void report(Element* element, const char* reason, bool disallowedAttr)
        {
            AuditsIssue issue;
            issue.reason = reason;
            issue.nodeId = m_registry->getOrCreate(element);
            issue.hasDisallowedAttributes = disallowedAttr;
            m_out->push_back(issue);
        }

        // Walk the tree under a <select> the way Chrome checks its content
        // model. Only <option>, <optgroup> and a few wrappers are allowed.
        void checkSelect(Element* select)
        {
            checkSelectChildren(select, select);
        }

        void checkSelectChildren(Element* select, Node* parent)
        {
            for (Node* c = parent->firstChild(); c; c = c->nextSibling()) {
                if (!c->isElement()) {
                    continue;
                }
                Element* e = c->asElement();
                std::string tag = tagOf(e);
                if (tag == "option") {
                    checkOption(e);
                } else if (tag == "optgroup") {
                    checkOptGroup(select, e);
                } else if (inList(tag, kSelectChildTags)) {
                    if (tag == "div") {
                        checkSelectChildren(select, e);
                    }
                } else {
                    report(select, kDisallowedSelectChild, false);
                }
            }
        }

        void checkOptGroup(Element* select, Element* group)
        {
            for (Node* c = group->firstChild(); c; c = c->nextSibling()) {
                if (!c->isElement()) {
                    continue;
                }
                Element* e = c->asElement();
                std::string tag = tagOf(e);
                if (tag == "option") {
                    checkOption(e);
                } else if (tag == "legend") {
                    checkLegend(e);
                } else if (!inList(tag, kSelectChildTags)) {
                    report(select, kDisallowedSelectChild, false);
                }
            }
        }

        void checkOption(Element* option)
        {
            Node* marker = option->isHTMLOptionElement()
                               ? option->asHTMLOptionElement()->marker()
                               : nullptr;
            for (Node* n = nextInTree(option, option); n;
                 n = nextInTree(n, option)) {
                if (!n->isElement() || isInside(n, marker)) {
                    continue;
                }
                Element* e = n->asElement();
                std::string tag = tagOf(e);
                // Chrome calls an interactive tag non-phrasing here. Only an
                // interactive attribute gives the interactive reason.
                if (hasInteractiveAttribute(e)) {
                    report(option, "InteractiveContentOptionChild", true);
                    return;
                }
                if (!inList(tag, kPhrasingTags) || isInteractive(e, tag)) {
                    report(option, "NonPhrasingContentOptionChild", false);
                    return;
                }
            }
        }

        void checkLegend(Element* legend)
        {
            for (Node* n = nextInTree(legend, legend); n;
                 n = nextInTree(n, legend)) {
                if (!n->isElement()) {
                    continue;
                }
                Element* e = n->asElement();
                if (isInteractive(e, tagOf(e))) {
                    report(legend, "InteractiveContentLegendChild",
                           hasInteractiveAttribute(e));
                    return;
                }
            }
        }

        NodeRegistry* m_registry;
        std::vector<AuditsIssue>* m_out;
        const std::vector<const void*>* m_closedSelects;
    };

    void emitIssue(CDPDispatcher* dispatcher, const std::string& sessionId,
                   const AuditsIssue& issue)
    {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

        rapidjson::Value accessibility(rapidjson::kObjectType);
        accessibility.AddMember("nodeId", issue.nodeId, alloc);
        accessibility.AddMember("elementAccessibilityIssueReason",
                                rapidjson::Value(issue.reason.c_str(), alloc),
                                alloc);
        accessibility.AddMember("hasDisallowedAttributes",
                                issue.hasDisallowedAttributes, alloc);
        rapidjson::Value details(rapidjson::kObjectType);
        details.AddMember(rapidjson::StringRef(kDetailsKey), accessibility,
                          alloc);
        rapidjson::Value body(rapidjson::kObjectType);
        body.AddMember("code", rapidjson::StringRef(kElementAccessibilityIssue),
                       alloc);
        body.AddMember("details", details, alloc);
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("issue", body, alloc);

        CDPCommand event(dispatcher, Optional<int64_t>(), sessionId, nullptr);
        event.sendEvent(kIssueAddedEvent, params, doc);
    }

} // namespace

void notifyAuditsDocumentLoaded(Document* document)
{
    if (!document) {
        return;
    }
    BrowsingContext* context = document->browsingContext();
    if (!context || !context->webView()) {
        return;
    }
    CDPServer* server = context->webView()->cdpServer();
    if (!server || !server->dispatcher()) {
        return;
    }
    CDPDispatcher* dispatcher = server->dispatcher();
    TargetContext* target = dispatcher->contextForWebView(context->webView());
    if (!target || !target->session) {
        return;
    }

    // Issues belong to the document, so the ones of the old page go away.
    CDPSession* session = target->session;
    session->auditsIssues.clear();
    Scanner(target->nodeRegistry, &session->auditsIssues,
            &session->auditsClosedSelects)
        .scan(document);
    session->auditsClosedSelects.clear();
    if (!session->auditsEnabled) {
        return;
    }
    for (const AuditsIssue& issue : session->auditsIssues) {
        emitIssue(dispatcher, session->sessionId, issue);
    }
}

void notifyAuditsSelectClosedByInput(Element* select)
{
    Document* document = select->document();
    BrowsingContext* context = document->browsingContext();
    if (!context || !context->webView()) {
        return;
    }
    CDPServer* server = context->webView()->cdpServer();
    if (!server || !server->dispatcher()) {
        return;
    }
    TargetContext* target =
        server->dispatcher()->contextForWebView(context->webView());
    if (!target || !target->session) {
        return;
    }
    target->session->auditsClosedSelects.push_back(select);
}

void replayAuditsIssues(CDPDispatcher* dispatcher, const std::string& sessionId)
{
    TargetContext* target = dispatcher->contextForSession(sessionId);
    if (!target || !target->session) {
        return;
    }
    for (const AuditsIssue& issue : target->session->auditsIssues) {
        emitIssue(dispatcher, sessionId, issue);
    }
}

} // namespace Starfish

#endif
