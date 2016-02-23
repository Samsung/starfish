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

void FrameTreeBuilder::clearTree(Node* current)
{
    current->setFrame(nullptr);

    Node* n = current->firstChild();
    while(n) {
        clearTree(n);
        n = n->nextSibling();
    }
}

void buildTree(Node* current, bool force = false)
{
    if (current->needsFrameTreeBuild() || force) {
        force = true;
        Frame* parent = current->parentNode()->frame();
        DisplayValue display = current->style()->originalDisplay();
        Frame* currentFrame;

        bool isBlockChild = false;
        if (display == DisplayValue::BlockDisplayValue) {
            isBlockChild = true;
            if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                currentFrame = new FrameReplacedImage(current, current->style(), element->src());
            } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLBRElement()) {
                currentFrame = new FrameLineBreak(current, current->style());
            } else {
                currentFrame = new FrameBlockBox(current, current->style());
            }
        } else if (display == DisplayValue::InlineDisplayValue) {
            if (current->isCharacterData() && current->asCharacterData()->isText()) {
                currentFrame = new FrameText(current, current->style());
            } else if(current->isComment()) {
                FrameTreeBuilder::clearTree(current);
                return ;
            } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                currentFrame = new FrameReplacedImage(current, current->style(), element->src());
            } else if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLBRElement()) {
                currentFrame = new FrameLineBreak(current, current->style());
            } else {
                currentFrame = new FrameInline(current, current->style());
            }
        } else if (display == DisplayValue::NoneDisplayValue) {
            FrameTreeBuilder::clearTree(current);
            return ;
        } else if (display == DisplayValue::InlineBlockDisplayValue) {
            if (current->isElement() && current->asElement()->isHTMLElement() && current->asElement()->asHTMLElement()->isHTMLImageElement()) {
                auto element = current->asElement()->asHTMLElement()->asHTMLImageElement();
                currentFrame = new FrameReplacedImage(current, current->style(), element->src());
            } else {
                currentFrame = new FrameBlockBox(current, current->style());
            }
        }

        if (parent->isFrameBlockBox()) {
            if (!parent->firstChild()) {
                parent->appendChild(currentFrame);
            } else {
                if (parent->asFrameBlockBox()->hasBlockFlow()) {
                    if (isBlockChild) {
                        // Block... + Block case
                        parent->appendChild(currentFrame);
                    } else {
                        // Block... + Inline case
                        Frame* last = parent->lastChild();
                        STARFISH_ASSERT(last->style()->display() == BlockDisplayValue);

                        if (last->isNormalFlow()) {
                            if (last->node()) {
                                ComputedStyle* newStyle = new ComputedStyle(parent->style());
                                newStyle->setDisplay(DisplayValue::BlockDisplayValue);
                                last = new FrameBlockBox(nullptr, newStyle);
                                parent->appendChild(last);
                            }
                        } else {
                            if (last->previous() && last->previous()->isFrameBlockBox() && !last->previous()->asFrameBlockBox()->hasBlockFlow()) {
                                last = last->previous();
                            } else {
                                ComputedStyle* newStyle = new ComputedStyle(parent->style());
                                newStyle->setDisplay(DisplayValue::BlockDisplayValue);
                                last = new FrameBlockBox(nullptr, newStyle);
                                parent->appendChild(last);
                            }
                        }

                        last->appendChild(currentFrame);
                    }
                } else {
                    if (isBlockChild) {
                        // Inline... + Block case
                        std::vector<Frame*, gc_allocator<Frame*>> backup;
                        while (parent->firstChild()) {
                            backup.push_back(parent->firstChild());
                            parent->removeChild(parent->firstChild());
                        }

                        ComputedStyle* newStyle = new ComputedStyle(parent->style());
                        newStyle->setDisplay(DisplayValue::BlockDisplayValue);
                        FrameBlockBox* blockBox = new FrameBlockBox(nullptr, newStyle);
                        for(unsigned i = 0; i < backup.size(); i ++) {
                            blockBox->appendChild(backup[i]);
                        }

                        parent->appendChild(blockBox);
                        parent->appendChild(currentFrame);
                    } else {
                        // Inline... + Inline case
                        parent->appendChild(currentFrame);
                    }

                }
            }
        } else {
            STARFISH_ASSERT(parent->isFrameInline());
            parent->appendChild(currentFrame);
        }

        STARFISH_ASSERT(currentFrame->parent());
        current->setFrame(currentFrame);
        current->clearNeedsFrameTreeBuild();
    }

    if (current->childNeedsFrameTreeBuild() || force) {
        Node* n = current->firstChild();
        while(n) {
            buildTree(n, force);
            n = n->nextSibling();
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
    buildTree(n);
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
