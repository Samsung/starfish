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

#ifndef __StarfishIntersectionObserver__
#define __StarfishIntersectionObserver__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"
#include "binding/generated/ElementOrDocumentUnion.h"
#include "binding/generated/doubleOrSequenceOfdoubleUnion.h"

namespace Starfish {

struct IntersectionObserverInit {
    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(Nullable<ElementOrDocument>, root, Root);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, rootMargin, RootMargin);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(doubleOrSequenceOfdouble, threshold,
                                      Threshold);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(Nullable<ElementOrDocument>, root, Root);
    DEFINE_MEMBER_WITH_HASFLAG(String*, rootMargin, RootMargin);
    DEFINE_MEMBER_WITH_HASFLAG(doubleOrSequenceOfdouble, threshold, Threshold);
};

class IntersectionObserverCallback : public gc {
public:
    static IntersectionObserverCallback* toIntersectionObserverCallback(
        ScriptValue callback);

    IntersectionObserverCallback(ScriptValue callback);

    ScriptValue scriptValue()
    {
        return m_intersectionObserverCallback;
    }

private:
    ScriptValue m_intersectionObserverCallback = nullptr;
};

class IntersectionObserver final : public ScriptWrappable {
public:
    IntersectionObserver(ExecutionContext* executionContext,
                         IntersectionObserverCallback* callBack);
    IntersectionObserver(ExecutionContext* executionContext,
                         IntersectionObserverCallback* callBack,
                         IntersectionObserverInit options);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isIntersectionObserver() const override;

    Nullable<ElementOrDocument> root();

    String* rootMargin();

    GCAtomicVector<double> thresholds();

    void observe(Element* target);

    void unobserve(Element* target);

    void disconnect();

    GCVector<IntersectionObserverEntry*> takeRecords();

    bool hasRecords()
    {
        return !m_queuedEntries.empty();
    }

    void notify();

    GCVector<Element*>& targets()
    {
        return m_targets;
    }

    void queueIntersectionObserverEntry(IntersectionObserverEntry* entry);

    ElementOrDocument intersectionRoot();

    DOMRect* rootBoundingClientRect();

    bool isValidTarget(Element* target);

private:
    void initialize(Nullable<IntersectionObserverInit> maybeOptions = nullptr);
    String* parseMarginToFoursides(String* rootMargin);

    ExecutionContext* m_executionContext;
    Nullable<ElementOrDocument> m_root;
    String* m_rootMargin = String::emptyString;
    std::vector<double> m_thresholds = { 0 };

    IntersectionObserverCallback* m_callback = nullptr;
    GCVector<Element*> m_targets;
    GCVector<IntersectionObserverEntry*> m_queuedEntries;
};

struct IntersectionObserverRegistration : public gc {
    IntersectionObserver* observer = nullptr;
    int32_t previousThresholdIndex = -1;
    bool previousIsIntersecting = false;
    Unit::Rect previousIntersectRect = { 0, 0, 0, 0 };
};

} // namespace Starfish

#endif
