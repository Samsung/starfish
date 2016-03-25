#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLImageElement.h"

#include "platform/message_loop/MessageLoop.h"
#include "layout/FrameReplacedImage.h"

namespace StarFish {

void HTMLImageElement::didAttributeChanged(QualifiedName name, String* old, String* value)
{
    HTMLElement::didAttributeChanged(name, old, value);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        LayoutSize sizeBefore(0, 0);
        LayoutSize size(0, 0);
        sizeBefore = intrinsicSize();
        if (!value->equals(String::spaceString)) {
            m_imageData = document()->window()->starFish()->fetchImage(document()->window()->starFish()->makeResourcePath(value));
        } else {
            m_imageData = nullptr;
        }

        size = intrinsicSize();
        if (frame()) {
            if (sizeBefore == size) {
                setNeedsPainting();
            } else {
                setNeedsLayout();
            }
        } else {
            setNeedsFrameTreeBuild();
        }

        document()->window()->starFish()->messageLoop()->addIdler([](void* data) {
            HTMLImageElement* element = (HTMLImageElement*)data;
            QualifiedName eventType = element->document()->window()->starFish()->staticStrings()->m_load;
            Event* e = new Event(eventType, EventInit(false, false));
            element->EventTarget::dispatchEvent(element, e);
        }, this);

    } else if (name == document()->window()->starFish()->staticStrings()->m_width
        || name == document()->window()->starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    }
}

}
