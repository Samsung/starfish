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
        : m_currentBlockContainer(currentBlockContainer)
    {
    }
    FrameBlockBox* m_currentBlockContainer;
};

void FrameTreeBuilder::clearTree(Node* current)
{
    current->setFrame(nullptr);

    Node* n = current->firstChild();
    while(n) {
        clearTree(n);
        n = n->nextSibling();
    }
}

void frameBlockBoxChildInserter(FrameBlockBox* frameBlockBox, Frame* currentFrame)
{
    if (!frameBlockBox->firstChild()) {
        frameBlockBox->appendChild(currentFrame);
        return;
    }

    bool isBlockChild = currentFrame->style()->originalDisplay() == BlockDisplayValue;
    if (frameBlockBox->hasBlockFlow()) {
        if (isBlockChild) {
            // Block... + Block case
            frameBlockBox->appendChild(currentFrame);
        } else {
            // Block... + Inline case
            Frame* last = frameBlockBox->lastChild();
            STARFISH_ASSERT(last->style()->display() == BlockDisplayValue);

            if (last->node()) {
                ComputedStyle* newStyle = new ComputedStyle(currentFrame->style());
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
            std::vector<Frame*, gc_allocator<Frame*>> backup;
            while (frameBlockBox->firstChild()) {
                backup.push_back(frameBlockBox->firstChild());
                frameBlockBox->removeChild(frameBlockBox->firstChild());
            }

            ComputedStyle* newStyle = new ComputedStyle(frameBlockBox->style());
            newStyle->setDisplay(DisplayValue::BlockDisplayValue);
            FrameBlockBox* blockBox = new FrameBlockBox(nullptr, newStyle);
            for(unsigned i = 0; i < backup.size(); i ++) {
                blockBox->appendChild(backup[i]);
            }

            frameBlockBox->appendChild(blockBox);
            frameBlockBox->appendChild(currentFrame);
        } else {
            // Inline... + Inline case
            Frame* parent = currentFrame->node()->parentNode()->frame();
            parent->appendChild(currentFrame);
        }
    }
}

void buildTree(Node* current, FrameTreeBuilderContext& ctx, bool force = false)
{
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
                    currentFrame = new FrameText(current, current->style());
                } else if(current->isComment()) {
                    FrameTreeBuilder::clearTree(current);
                    return ;
                } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                    currentFrame = new FrameReplacedImage(current, element->src());
                } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLBRElement()) {
                    currentFrame = new FrameLineBreak(current);
                } else {
                    currentFrame = new FrameInline(current);
                }
            } else if (display == DisplayValue::NoneDisplayValue) {
                FrameTreeBuilder::clearTree(current);
                return ;
            } else if (display == DisplayValue::InlineBlockDisplayValue) {
                if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                    currentFrame = new FrameReplacedImage(current, element->src());
                } else {
                    currentFrame = new FrameBlockBox(current, nullptr);
                }
            }
        }

        bool isBlockChild = currentFrame->style()->originalDisplay() == BlockDisplayValue;
        if (isBlockChild && !ctx.m_currentBlockContainer->hasBlockFlow() && current->parentNode()->style()->display() == InlineDisplayValue && currentFrame->isNormalFlow()) {
            // divide block. when comes Inline.. + Block(normal flow)
            Frame* parent = current->parentNode()->frame();
            while (true) {
                if (parent->isFrameBlockBox() && parent->node())
                    break;

                parent = parent->parent();
            }
            ctx.m_currentBlockContainer = parent->asFrameBlockBox();
        }

        frameBlockBoxChildInserter(ctx.m_currentBlockContainer, currentFrame);

        STARFISH_ASSERT(currentFrame->parent());
        current->setFrame(currentFrame);
        current->clearNeedsFrameTreeBuild();
    }

    if (current->childNeedsFrameTreeBuild() || force) {
        Frame* currentFrame = current->frame();
        Frame* back = ctx.m_currentBlockContainer;
        bool isBlockContainer = currentFrame->isFrameBlockBox();
        if (isBlockContainer) {
            ctx.m_currentBlockContainer = currentFrame->asFrameBlockBox();
        }

        Node* n = current->firstChild();

        while(n) {
            buildTree(n, ctx, force);
            n = n->nextSibling();
        }

        if (isBlockContainer) {
            ctx.m_currentBlockContainer = back->asFrameBlockBox();
        }

        current->clearChildNeedsFrameTreeBuild();
    }
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

void dump(Frame* frm, unsigned depth)
{
    for (unsigned i = 0; i < depth; i ++) {
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


}
