#include "StarFishConfig.h"
#include "FrameDocument.h"

namespace StarFish {

void FrameDocument::layout(LayoutContext& ctx)
{
    int w = node()->asDocument()->window()->width();
    int h = node()->asDocument()->window()->height();

    style()->setWidth(Length(Length::Fixed, w));
    style()->setHeight(Length(Length::Fixed, h));

    STARFISH_ASSERT(firstChild() == lastChild());
    STARFISH_ASSERT(firstChild()->isRootElement());

    FrameBlockBox::layout(ctx);

    firstChild()->asFrameBox()->asFrameBlockBox()->m_visibleRect.unite(m_visibleRect);
}

}
