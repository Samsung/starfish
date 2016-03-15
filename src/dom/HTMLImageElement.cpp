#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLImageElement.h"

#include "layout/FrameReplacedImage.h"

namespace StarFish {

void HTMLImageElement::didAttributeChanged(QualifiedName name, String* old, String* value)
{
    HTMLElement::didAttributeChanged(name, old, value);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        if (frame()) {
            LayoutSize sizBefore = frame()->asFrameReplaced()->asFrameReplacedImage()->intrinsicSize();
            frame()->asFrameReplaced()->asFrameReplacedImage()->replaceImageData(this, value);

            if (sizBefore == frame()->asFrameReplaced()->asFrameReplacedImage()->intrinsicSize()) {
                setNeedsPainting();
            } else {
                setNeedsLayout();
            }
        } else {
            setNeedsFrameTreeBuild();
        }
    } else if (name == document()->window()->starFish()->staticStrings()->m_width
        || name == document()->window()->starFish()->staticStrings()->m_height) {
        if (frame()) {
            LayoutSize sizeBefore = frame()->asFrameReplaced()->asFrameReplacedImage()->intrinsicSize();
            if ((sizeBefore.width() == width()) && (sizeBefore.height() == height())) {
                setNeedsPainting();
            } else {
                setNeedsLayout();
            }
        } else {
            setNeedsFrameTreeBuild();
        }
    }
}

}
