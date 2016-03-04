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

    bool needsOwnBuffer = false;
    m_needsOwnBuffer = needsOwnBuffer;
}



}
