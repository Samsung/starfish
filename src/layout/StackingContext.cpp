#include "StarFishConfig.h"
#include "StackingContext.h"

#include "FrameBox.h"

namespace StarFish {

StackingContext::StackingContext(FrameBox* owner, StackingContext* parent)
{
    m_owner = owner;
    m_parent = parent;
    if (m_parent) {
        int32_t num = owner->style()->zIndex();
        auto iter = m_parent->m_childContexts.find(num);
        if (iter == m_parent->m_childContexts.end()) {
            iter = m_parent->m_childContexts.insert(std::make_pair(num, new StackingContextChild())).first;
        }
        iter->second->push_back(this);
    }
    m_needsOwnBuffer = false;
}

bool StackingContext::computeStackingContextProperties(bool forceNeedsBuffer)
{
    bool childNeedsBuffer = false;
    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        auto iter2 = iter->second->begin();
        while (iter2 != iter->second->end()) {
            childNeedsBuffer = (*iter2)->computeStackingContextProperties(childNeedsBuffer);
            iter2++;
        }
        iter++;
    }

    m_matrix.reset();

    m_needsOwnBuffer = forceNeedsBuffer || childNeedsBuffer || m_owner->style()->opacity() != 1;
    return m_needsOwnBuffer;
}

}
