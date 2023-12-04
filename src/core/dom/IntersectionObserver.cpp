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
#include "Starfish.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/IntersectionObserver.h"
#include "core/dom/IntersectionObserverEntry.h"
#include "core/page/WebBase.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/DOMRect.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "EscargotPublic.h"

namespace Starfish {

IntersectionObserverCallback*
IntersectionObserverCallback::toIntersectionObserverCallback(
    ScriptValue callback)
{
    if (!isCallableScriptValue(callback)) {
        return nullptr;
    }

    return new IntersectionObserverCallback(callback);
}
IntersectionObserverCallback::IntersectionObserverCallback(ScriptValue callback)
    : m_intersectionObserverCallback(callback)
{
}

IntersectionObserver::IntersectionObserver(
    ExecutionContext* executionContext, IntersectionObserverCallback* callBack)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_callback(callBack)
{
    initialize();
}

IntersectionObserver::IntersectionObserver(
    ExecutionContext* executionContext, IntersectionObserverCallback* callBack,
    IntersectionObserverInit options)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_callback(callBack)
{
    initialize(options);
}

void IntersectionObserver::initialize(
    Nullable<IntersectionObserverInit> maybeOptions)
{
    if (maybeOptions) {
        IntersectionObserverInit options = maybeOptions.getValue();
        if (options.hasRoot()) {
            m_root = options.root();
        }

        if (options.hasRootMargin()) {
            m_rootMargin = parseMarginToFoursides(options.rootMargin());
        }

        if (options.hasThreshold()) {
            m_thresholds.clear();
            doubleOrSequenceOfdouble thresholds = options.threshold();
            if (thresholds.isdoubleValue()) {
                m_thresholds.emplace_back(thresholds.getdoubleValue());
            } else if (thresholds.isSequenceOfdoubleValue()) {
                for (const auto& threshold :
                     thresholds.getSequenceOfdoubleValue()) {
                    m_thresholds.emplace_back(threshold);
                }
            }
        }
    }
}

String* IntersectionObserver::parseMarginToFoursides(String* rootMargin)
{
    CSSStyleDeclaration styleDeclaration(m_executionContext->document());
    String* margin = String::createASCIIString("margin");
    styleDeclaration.setProperty(margin, rootMargin, nullptr);

    StringBuilder builder;
    builder.appendString(styleDeclaration.getPropertyValueInternal(
        CSSStyleValuePair::KeyKind::MarginTop));
    builder.appendChar(' ');
    builder.appendString(styleDeclaration.getPropertyValueInternal(
        CSSStyleValuePair::KeyKind::MarginRight));
    builder.appendChar(' ');
    builder.appendString(styleDeclaration.getPropertyValueInternal(
        CSSStyleValuePair::KeyKind::MarginBottom));
    builder.appendChar(' ');
    builder.appendString(styleDeclaration.getPropertyValueInternal(
        CSSStyleValuePair::KeyKind::MarginLeft));
    return builder.finalize();
}

ScriptBindingInstance* IntersectionObserver::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Nullable<ElementOrDocument> IntersectionObserver::root()
{
    return m_root;
}

String* IntersectionObserver::rootMargin()
{
    return m_rootMargin;
}

GCAtomicVector<double> IntersectionObserver::thresholds()
{
    GCAtomicVector<double> thresholds;
    for (const auto& threshold : m_thresholds) {
        thresholds.emplace_back(threshold);
    }
    return thresholds;
}

void IntersectionObserver::observe(Element* target)
{
    auto iter = std::find(m_targets.begin(), m_targets.end(), target);
    if (iter != m_targets.end()) {
        return;
    }

    IntersectionObserverRegistration* intersectionObserverRegistration =
        new IntersectionObserverRegistration();
    intersectionObserverRegistration->observer = this;
    intersectionObserverRegistration->previousThresholdIndex = -1;
    intersectionObserverRegistration->previousIsIntersecting = false;

    target->appendIntersectionObserverRegistration(
        intersectionObserverRegistration);

    m_targets.emplace_back(target);

    Document* document = target->document();
    if (!document->hasIntersectionObserver(this)) {
        document->addIntersectionObserver(this);
    }
}

void IntersectionObserver::unobserve(Element* target)
{
    if (m_targets.size()) {
        target->removeIntersectionObserverRegistration(this);
        m_targets.erase(std::remove_if(m_targets.begin(), m_targets.end(),
                                       [target](const Element* item) {
                                           return item == target;
                                       }),
                        m_targets.end());
    }

    Document* document = target->document();
    if (!m_targets.size() && document->hasIntersectionObserver(this)) {
        document->removeIntersectionObserver(this);
    }
}

void IntersectionObserver::disconnect()
{
    GCVector<Element*> targets = m_targets;
    for (auto* target : targets) {
        unobserve(target);
    }
    m_queuedEntries.clear();
}

GCVector<IntersectionObserverEntry*> IntersectionObserver::takeRecords()
{
    GCVector<IntersectionObserverEntry*> records;
    records = m_queuedEntries;
    m_queuedEntries.clear();
    return records;
}

void IntersectionObserver::notify()
{
    ScriptValue callback = m_callback->scriptValue();
    if (isCallableScriptValue(callback) && m_queuedEntries.size()) {
        ScriptValue* argv = nullptr;
        size_t argc = 0;
        const auto& result = Escargot::Evaluator::execute(
            scriptBindingInstance()->scriptContext(),
            [](Escargot::ExecutionStateRef* state,
               IntersectionObserver* self) -> Escargot::ValueRef* {
                Escargot::ArrayObjectRef* arrayObj =
                    Escargot::ArrayObjectRef::create(state);
                for (size_t i = 0; i < self->m_queuedEntries.size(); i++) {
                    Escargot::ValueRef* item =
                        self->m_queuedEntries[i]->scriptValue();
                    arrayObj->set(state, Escargot::ValueRef::create(i), item);
                }
                return arrayObj;
            },
            this);
        argc = 1;
        argv = ALLOCA(sizeof(ScriptValue) * argc, ScriptValue);
        argv[0] = result.result;
        m_queuedEntries.clear();
        callScriptFunction(scriptBindingInstance(), callback, argv, 1,
                           scriptValue());
    }
}

void IntersectionObserver::queueIntersectionObserverEntry(
    IntersectionObserverEntry* entry)
{
    m_queuedEntries.emplace_back(entry);
}

ElementOrDocument IntersectionObserver::intersectionRoot()
{
    if (m_root.hasValue() && !m_root.getValue().isNoneValue()) {
        return m_root.getValue();
    } else {
        // Implict root.
        return ElementOrDocument::createDocument(m_executionContext->webBase()
                                                     ->asWebView()
                                                     ->mainBrowsingContext()
                                                     ->document());
    }
}

DOMRect* IntersectionObserver::rootBoundingClientRect()
{
    ElementOrDocument root = intersectionRoot();

    DOMRect* boundingClientRect = nullptr;
    if (root.isElementValue()) {
        boundingClientRect = root.getElementValue()->getBoundingClientRect();
    } else if (root.isDocumentValue()) {
        Document* document = root.getDocumentValue();
        boundingClientRect =
            new DOMRect(m_executionContext, 0, 0,
                        document->documentElement()->clientWidth(),
                        document->documentElement()->clientHeight());
    }

    return boundingClientRect;
}

bool IntersectionObserver::isValidTarget(Element* target)
{
    ElementOrDocument intersectionRoot = this->intersectionRoot();
    if (root().hasValue() && !root().getValue().isNoneValue()) {
        Document* rootDocument = nullptr;
        if (intersectionRoot.isDocumentValue()) {
            rootDocument = intersectionRoot.getDocumentValue();
        } else {
            rootDocument = intersectionRoot.getElementValue()->document();
        }
        if (target->document() != rootDocument) {
            return false;
        }
    }

    if (intersectionRoot.isElementValue()) {
        if (target->document() !=
            intersectionRoot.getElementValue()->document()) {
            return false;
        }

        Frame* rootElementFrame = intersectionRoot.getElementValue()->frame();
        Frame* ancestorContainingBlock = containingBlock(target->frame());
        bool isDescendantOfRoot = false;
        while (ancestorContainingBlock &&
               !(ancestorContainingBlock)->isFrameDocument()) {
            if (ancestorContainingBlock == rootElementFrame) {
                isDescendantOfRoot = true;
                break;
            }
            ancestorContainingBlock = containingBlock(
                static_cast<FrameBox*>(ancestorContainingBlock));
        }
        if (!isDescendantOfRoot) {
            return false;
        }
    }
    return true;
}

} // namespace Starfish
