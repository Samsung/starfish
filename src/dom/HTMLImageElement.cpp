#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLImageElement.h"

#include "layout/FrameReplacedImage.h"

namespace StarFish {

void HTMLImageElement::didAttributeChanged(QualifiedName name, String* old, String* value)
{
    HTMLElement::didAttributeChanged(name, old, value);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        m_src = value;
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
    }
}


}
