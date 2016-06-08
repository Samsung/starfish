#include "StarFishConfig.h"
#include "FrameTreeBuilder.h"

#include "dom/DOM.h"

#include "Frame.h"
#include "FrameText.h"
#include "FrameInline.h"
#include "FrameBlockBox.h"
#include "FrameDocument.h"
#include "FrameReplaced.h"
#include "FrameReplacedImage.h"
#include "FrameLineBreak.h"

namespace StarFish {

class FrameTreeBuilderContext {
public:
    FrameTreeBuilderContext(FrameBlockBox* currentBlockContainer)
    {
        m_isInFrameInlineFlow = false;
        setCurrentBlockContainer(currentBlockContainer);
        computeTextDecorationData(currentBlockContainer->style());
    }

    void setCurrentBlockContainer(FrameBlockBox* blockContainer)
    {
        m_currentBlockContainer = blockContainer;
    }

    void setCurrentTextDecorationData(FrameTextTextDecorationData* deco)
    {
        m_currentDecorationData = deco;
    }

    FrameBlockBox* currentBlockContainer()
    {
        return m_currentBlockContainer;
    }

    void computeTextDecorationData(ComputedStyle* style)
    {
        if (style->textDecoration() != NoneTextDecorationValue) {
            m_currentDecorationData = new FrameTextTextDecorationData;
            m_currentDecorationData->m_hasUnderLine = false;
            m_currentDecorationData->m_hasLineThrough = false;
            if (style->textDecoration() == UnderLineTextDecorationValue) {
                m_currentDecorationData->m_hasUnderLine = true;
                m_currentDecorationData->m_underLineColor = style->color();
            } else if (style->textDecoration() == LineThroughTextDecorationValue) {
                m_currentDecorationData->m_hasLineThrough = true;
                m_currentDecorationData->m_lineThroughColor = style->color();
            }
        } else {
            m_currentDecorationData = nullptr;
        }
    }

    void mergeTextDecorationData(ComputedStyle* style)
    {
        if (style->textDecoration() != NoneTextDecorationValue) {
            if (!m_currentDecorationData) {
                m_currentDecorationData = new FrameTextTextDecorationData;
                m_currentDecorationData->m_hasUnderLine = false;
                m_currentDecorationData->m_hasLineThrough = false;
            }
            if (style->textDecoration() == UnderLineTextDecorationValue) {
                m_currentDecorationData->m_hasUnderLine = true;
                m_currentDecorationData->m_underLineColor = style->color();
            } else if (style->textDecoration() == LineThroughTextDecorationValue) {
                m_currentDecorationData->m_hasLineThrough = true;
                m_currentDecorationData->m_lineThroughColor = style->color();
            }
        }
    }

    FrameTextTextDecorationData* currentDecorationData()
    {
        return m_currentDecorationData;
    }

    std::unordered_map<Node*, FrameInline*>& frameInlineItem()
    {
        return m_frameInlineItem;
    }

    bool isInFrameInlineFlow()
    {
        return m_isInFrameInlineFlow;
    }

    void setIsInFrameInlineFlow(bool b)
    {
        m_isInFrameInlineFlow = b;
    }

protected:
    bool m_isInFrameInlineFlow;
    FrameBlockBox* m_currentBlockContainer;
    FrameTextTextDecorationData* m_currentDecorationData;
    std::unordered_map<Node*, FrameInline*, std::hash<Node*>, std::equal_to<Node*>> m_frameInlineItem;
};

void FrameTreeBuilder::clearTree(Node* current)
{
    current->setFrame(nullptr);

    Node* n = current->firstChild();
    while (n) {
        clearTree(n);
        n = n->nextSibling();
    }
}

static void frameBlockBoxChildInserter(FrameBlockBox* frameBlockBox, Frame* currentFrame, Node* currentNode, FrameTreeBuilderContext& ctx)
{
    if (!frameBlockBox->firstChild()) {
        frameBlockBox->appendChild(currentFrame);
        return;
    }

    bool isBlockChild = currentFrame->style()->originalDisplay() == BlockDisplayValue;

    if (!isBlockChild) {
        if (currentNode->parentNode()->style()->display() == InlineDisplayValue) {
            auto iter = ctx.frameInlineItem().find(currentNode->parentNode());
            iter->second->appendChild(currentFrame);
            return;
        }
    }

    if (frameBlockBox->hasBlockFlow()) {
        if (isBlockChild) {
            // Block... + Block case
            frameBlockBox->appendChild(currentFrame);
        } else {
            // Block... + Inline case
            Frame* last = frameBlockBox->lastChild();
            if (!last->isNormalFlow() && currentFrame->isNormalFlow()) {
                Frame* lastAnnyBlockBox = nullptr;

                Frame* cur = last->previous();
                while (cur) {
                    STARFISH_ASSERT(cur->isFrameBlockBox());
                    if (cur->isNormalFlow() && cur->node() == nullptr) {
                        lastAnnyBlockBox = cur;
                        break;
                    }
                    cur = cur->previous();
                }

                if (lastAnnyBlockBox) {
                    lastAnnyBlockBox->appendChild(currentFrame);
                    return;
                }
            }
            STARFISH_ASSERT(last->style()->display() == BlockDisplayValue);

            if (last->node()) {
                ComputedStyle* newStyle = new ComputedStyle(currentFrame->style());
                newStyle->loadResources(last->node());
                newStyle->arrangeStyleValues(currentFrame->style());
                newStyle->setDisplay(DisplayValue::BlockDisplayValue);

                last = new FrameBlockBox(nullptr, newStyle);
                frameBlockBox->appendChild(last);
                last->appendChild(currentFrame);
            } else {
                last->appendChild(currentFrame);
            }
        }
    } else {
        if (isBlockChild) {
            // Inline... + Block case
            std::vector<Frame*, gc_allocator<Frame*> > backup;
            while (frameBlockBox->firstChild()) {
                backup.push_back(frameBlockBox->firstChild());
                frameBlockBox->removeChild(frameBlockBox->firstChild());
            }

            ComputedStyle* newStyle = new ComputedStyle(frameBlockBox->style());
            newStyle->setDisplay(DisplayValue::BlockDisplayValue);
            newStyle->loadResources(currentNode);
            newStyle->arrangeStyleValues(frameBlockBox->style());

            FrameBlockBox* blockBox = new FrameBlockBox(nullptr, newStyle);
            for (unsigned i = 0; i < backup.size(); i++) {
                blockBox->appendChild(backup[i]);
            }

            frameBlockBox->appendChild(blockBox);
            frameBlockBox->appendChild(currentFrame);
        } else {
            // Inline... + Inline case
            frameBlockBox->appendChild(currentFrame);
        }
    }
}

void buildTree(Node* current, FrameTreeBuilderContext& ctx, bool force = false)
{
    bool prevIsInFrameInlineFlow = ctx.isInFrameInlineFlow();
    bool didSplitBlock = false;
    std::vector<FrameInline*, gc_allocator<FrameInline*>> stackedFrameInline;
    FrameTextTextDecorationData* textDecoBack = ctx.currentDecorationData();
    if (current->needsFrameTreeBuild() || force) {
        force = true;

        Frame* currentFrame;
        {
            DisplayValue display = current->style()->display();
            if (display == DisplayValue::BlockDisplayValue) {
                if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                    currentFrame = new FrameReplacedImage(current, element->src());
                } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLBRElement()) {
                    currentFrame = new FrameLineBreak(current);
                } else {
                    currentFrame = new FrameBlockBox(current, nullptr);
                }
            } else if (display == DisplayValue::InlineDisplayValue) {
                if (current->isCharacterData() && current->asCharacterData()->isText()) {
                    currentFrame = new FrameText(current, current->style(), ctx.currentDecorationData());
                } else if (current->isComment()) {
                    FrameTreeBuilder::clearTree(current);
                    return;
                } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                    currentFrame = new FrameReplacedImage(current, element->src());
                } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLBRElement()) {
                    currentFrame = new FrameLineBreak(current);
                } else {
                    currentFrame = new FrameInline(current);
                    ctx.setIsInFrameInlineFlow(true);
                    ctx.frameInlineItem().insert(std::make_pair(current, currentFrame->asFrameInline()));
                }
            } else if (display == DisplayValue::NoneDisplayValue) {
                FrameTreeBuilder::clearTree(current);
                return;
            } else if (display == DisplayValue::InlineBlockDisplayValue) {
                if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                    currentFrame = new FrameReplacedImage(current, element->src());
                } else {
                    currentFrame = new FrameBlockBox(current, nullptr);
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        bool isBlockChild = currentFrame->style()->originalDisplay() == BlockDisplayValue;
        if (isBlockChild && ctx.isInFrameInlineFlow() && currentFrame->isNormalFlow()) {
            // divide block. when comes Inline.. + Block(normal flow)
            didSplitBlock = true;
            Frame* parent = current->parentNode()->frame();
            while (true) {
                if (parent->isFrameBlockBox() && parent->node())
                    break;
                parent = parent->parent();
            }

            Node* nd = current->parentNode();
            while (true) {
                if (nd->frame()->isFrameBlockBox()) {
                    break;
                }
                auto iter = ctx.frameInlineItem().find(nd);
                STARFISH_ASSERT(iter != ctx.frameInlineItem().end());
                FrameInline* in = new FrameInline(nd);
                if (iter->second->isLeftMBPCleared()) {
                    in->setLeftMBPCleared();
                }
                if (iter->second->isRightMBPCleared()) {
                    in->setRightMBPCleared();
                }

                if (in->style()->direction() == DirectionValue::LtrDirectionValue) {
                    in->setLeftMBPCleared();
                    iter->second->setRightMBPCleared();
                } else {
                    iter->second->setLeftMBPCleared();
                    in->setRightMBPCleared();
                }

                stackedFrameInline.push_back(in);
                iter->second = in;
                STARFISH_ASSERT(ctx.frameInlineItem().find(nd)->second == in);
                nd = nd->parentNode();
            }

            ctx.setCurrentBlockContainer(parent->asFrameBlockBox());
            ctx.setIsInFrameInlineFlow(false);
        }

        frameBlockBoxChildInserter(ctx.currentBlockContainer(), currentFrame, current, ctx);

        STARFISH_ASSERT(currentFrame->parent());
        current->setFrame(currentFrame);
        current->clearNeedsFrameTreeBuild();
    }

    Frame* currentFrame = current->frame();
    if (currentFrame->style()->display() == InlineBlockDisplayValue || !currentFrame->isNormalFlow()) {
        ctx.computeTextDecorationData(currentFrame->style());
    } else {
        ctx.mergeTextDecorationData(currentFrame->style());
    }

    if (current->childNeedsFrameTreeBuild() || force) {
        Frame* currentFrame = current->frame();
        FrameBlockBox* back = ctx.currentBlockContainer();

        bool isBlockContainer = currentFrame->isFrameBlockBox();
        if (isBlockContainer) {
            ctx.setCurrentBlockContainer(currentFrame->asFrameBlockBox());
        }

        Node* n = current->firstChild();

        while (n) {
            buildTree(n, ctx, force);
            n = n->nextSibling();
        }

        if (isBlockContainer) {
            ctx.setCurrentBlockContainer(back);
        }

        current->clearChildNeedsFrameTreeBuild();
    }

    if (didSplitBlock) {
        FrameInline* prev = nullptr;
        for (size_t i = stackedFrameInline.size(); i > 0; i--) {
            FrameInline* in = stackedFrameInline[i - 1];
            if (i == stackedFrameInline.size()) {
                STARFISH_ASSERT(ctx.currentBlockContainer()->hasBlockFlow());

                ComputedStyle* newStyle = new ComputedStyle(ctx.currentBlockContainer()->style());
                newStyle->setDisplay(DisplayValue::BlockDisplayValue);
                newStyle->loadResources(current);
                newStyle->arrangeStyleValues(ctx.currentBlockContainer()->style());

                FrameBlockBox* blockBox = new FrameBlockBox(nullptr, newStyle);

                ctx.currentBlockContainer()->appendChild(blockBox);
                blockBox->appendChild(in);
                ctx.setCurrentBlockContainer(blockBox);
            } else {
                prev->appendChild(in);
            }
            prev = in;
        }
    }
    ctx.setIsInFrameInlineFlow(prevIsInFrameInlineFlow);
    ctx.setCurrentTextDecorationData(textDecoBack);
}

void FrameTreeBuilder::buildFrameTree(Document* document)
{
    STARFISH_ASSERT(document->frame());

    Node* n = document->rootElement();

    // FIXME display of html element always considered as "block"
    ASSERT(n->style()->display() == DisplayValue::BlockDisplayValue);
    FrameTreeBuilderContext ctx(document->frame()->asFrameBlockBox());
    buildTree(n, ctx);
}
#ifdef STARFISH_ENABLE_TEST
void dump(Frame* frm, unsigned depth)
{
    for (unsigned i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("%s", frm->name());
    printf("[%p]", frm);
    if (frm->node()) {
        frm->node()->dump();
    } else {
        printf("[anonymous block box] ");
    }

    frm->dump(depth);

    printf("\n");

    Frame* f = frm->firstChild();
    while (f) {
        dump(f, depth + 1);
        f = f->next();
    }
}

void FrameTreeBuilder::dumpFrameTree(Document* document)
{
    dump(document->frame(), 0);
}
#endif
}
