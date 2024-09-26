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

#ifndef __StarfishMutationObserver__
#define __StarfishMutationObserver__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

class MutationRecord;

enum class MutationObserverOptionType : uint8_t {
    kNone = 0,
    kChildList = 1 << 0,
    kAttributes = 1 << 1,
    kCharacterData = 1 << 2,
    kAllMutationType = kChildList | kAttributes | kCharacterData,
    kSubtree = 1 << 3,
    kAttributeOldValue = 1 << 4,
    kCharacterDataOldValue = 1 << 5,
    kAllDeliveryOptions = kAttributeOldValue | kCharacterDataOldValue,
    kAttributeFilter = 1 << 6,
};

inline MutationObserverOptionType& operator|=(MutationObserverOptionType& lhs,
                                              MutationObserverOptionType rhs)
{
    using underlyingType =
        std::underlying_type<MutationObserverOptionType>::type;
    lhs = static_cast<MutationObserverOptionType>(
        static_cast<underlyingType>(lhs) | static_cast<underlyingType>(rhs));
    return lhs;
}

inline MutationObserverOptionType operator&(MutationObserverOptionType lhs,
                                            MutationObserverOptionType rhs)
{
    using underlyingType =
        std::underlying_type<MutationObserverOptionType>::type;
    return static_cast<MutationObserverOptionType>(
        static_cast<underlyingType>(lhs) & static_cast<underlyingType>(rhs));
}

inline MutationObserverOptionType operator|(MutationObserverOptionType lhs,
                                            MutationObserverOptionType rhs)
{
    using underlyingType =
        std::underlying_type<MutationObserverOptionType>::type;
    return static_cast<MutationObserverOptionType>(
        static_cast<underlyingType>(lhs) | static_cast<underlyingType>(rhs));
}

inline bool operator!(MutationObserverOptionType option)
{
    return static_cast<bool>(option) == false;
}

inline bool operator&&(MutationObserverOptionType option, bool value)
{
    return static_cast<bool>(option) && value;
}

inline MutationObserverOptionType mutationTypes(
    MutationObserverOptionType options)
{
    return options & MutationObserverOptionType::kAllMutationType;
}

inline MutationObserverOptionType deliveryOptions(
    MutationObserverOptionType options)
{
    return options & MutationObserverOptionType::kAllDeliveryOptions;
}

struct MutationObserverInit {
    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, childList, ChildList);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, attributes, Attributes);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, characterData, CharacterData);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, subtree, Subtree);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, attributeOldValue,
                                      AttributeOldValue);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, characterDataOldValue,
                                      CharacterDataOldValue);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<String*>, attributeFilter,
                                      AttributeFilter);

    // Define getter/setters
    DEFINE_MEMBER_WITH_HASFLAG(bool, childList, ChildList);
    DEFINE_MEMBER_WITH_HASFLAG(bool, attributes, Attributes);
    DEFINE_MEMBER_WITH_HASFLAG(bool, characterData, CharacterData);
    DEFINE_MEMBER_WITH_HASFLAG(bool, subtree, Subtree);
    DEFINE_MEMBER_WITH_HASFLAG(bool, attributeOldValue, AttributeOldValue);
    DEFINE_MEMBER_WITH_HASFLAG(bool, characterDataOldValue,
                               CharacterDataOldValue);
    DEFINE_MEMBER_WITH_HASFLAG(GCVector<String*>, attributeFilter,
                               AttributeFilter);
};

class MutationCallback : public gc {
public:
    static MutationCallback* toMutationCallback(ScriptValue callback);

    MutationCallback(ScriptValue callback);

    ScriptValue scriptValue()
    {
        return m_mutationCallback;
    }

private:
    ScriptValue m_mutationCallback;
};

class MutationObserverRegistration : public gc {
public:
    MutationObserverRegistration(
        MutationObserver* observer, Node* target,
        MutationObserverOptionType options,
        const GCUnorderedSet<String*>& attributeFilter);

    MutationObserver* observer()
    {
        return m_observer;
    }

    Node* target()
    {
        return m_target;
    }

    MutationObserverOptionType options()
    {
        return m_options;
    }

    void update(MutationObserverOptionType options,
                const GCUnorderedSet<String*>& attributeFilter);
    bool isInterestedIn(Node* node, const MutationObserverOptionType option,
                        const Optional<QualifiedName>& name);

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(MutationObserverRegistration)] = { 0 };
            MutationObserverRegistration::fillGCDescriptor(desc);
            descr = GC_make_descriptor(
                desc, GC_WORD_LEN(MutationObserverRegistration));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc,
                   GC_WORD_OFFSET(MutationObserverRegistration, m_observer));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(MutationObserverRegistration, m_target));
        markHashTable(desc, GC_WORD_OFFSET(MutationObserverRegistration,
                                           m_attributeFilter));
    }

private:
    MutationObserver* m_observer = nullptr;
    Node* m_target = nullptr;
    MutationObserverOptionType m_options;
    GCUnorderedSet<String*> m_attributeFilter;
};

class MutationObserver final : public ScriptWrappable {
public:
    MutationObserver(ExecutionContext* executionContext,
                     MutationCallback* callBack);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isMutationObserver() const override;

    void observe(Node* node);
    void observe(Node* node, MutationObserverInit options);

    void disconnect();

    GCVector<MutationRecord*> takeRecords();

    void enqueueMutationRecord(MutationRecord* record);

    void notify();

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(MutationObserver)] = { 0 };
            MutationObserver::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(MutationObserver));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(MutationObserver, m_executionContext));
        GC_set_bit(desc, GC_WORD_OFFSET(MutationObserver, m_callback));
        GC_set_bit(desc, GC_WORD_OFFSET(MutationObserver, m_queuedRecords));
        markHashTable(desc, GC_WORD_OFFSET(MutationObserver, m_registrations));
    }

private:
    ExecutionContext* m_executionContext;
    MutationCallback* m_callback = nullptr;

    GCVector<MutationRecord*> m_queuedRecords;
    GCVector<MutationRecord*> m_pendingQueuedRecords;
    GCUnorderedSet<MutationObserverRegistration*> m_registrations;
};

} // namespace Starfish

#endif
