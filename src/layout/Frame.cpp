#include "StarFishConfig.h"
#include "Frame.h"
#include "FrameBox.h"

namespace StarFish {

Frame* LayoutContext::blockContainer(Frame* currentFrame)
{
    Frame* f = currentFrame->parent();

    if (!f)
        return currentFrame;

    while (true) {
        if (f->isFrameBlockBox() && f->node() != nullptr) {
            return f;
        }
        f = f->parent();
    }
}

Frame* LayoutContext::containingBlock(Frame* currentFrame)
{
    // https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#containing-block-details
    Frame* block = blockContainer(currentFrame);
    if (currentFrame->style()->position() == AbsolutePositionValue) {
        while (!block->isFrameDocument() && !block->isPositionedElement()) {
            block = blockContainer(block);
        }
        return block;
    } else {
        return block;
    }
}

LayoutUnit LayoutContext::parentContentWidth(Frame* currentFrame)
{
    return blockContainer(currentFrame)->asFrameBox()->contentWidth();
}

bool LayoutContext::parentHasFixedHeight(Frame* currentFrame)
{
    Frame* container = blockContainer(currentFrame);
    while (container) {
        if (container->style()->height().isFixed()) {
            return true;
        } else if (container->style()->height().isAuto()) {
            return false;
        } else {
            STARFISH_ASSERT(container->style()->height().isPercent());
            container = blockContainer(container);
        }
    }
    return false;
}

LayoutUnit LayoutContext::parentFixedHeight(Frame* currentFrame)
{
    Frame* container = blockContainer(currentFrame);
    std::vector<Length> reverse;
    while (container) {
        STARFISH_ASSERT(!container->style()->height().isAuto());
        if (container->style()->height().isFixed()) {
            reverse.push_back(container->style()->height());
            break;
        } else {
            STARFISH_ASSERT(container->style()->height().isPercent());
            reverse.push_back(container->style()->height());
            container = blockContainer(container);
        }
    }
    LayoutUnit result = reverse.back().fixed();
    reverse.pop_back();
    while (reverse.size()) {
        result = result * reverse.back().percent();
        reverse.pop_back();
    }

    return result;
}

}
