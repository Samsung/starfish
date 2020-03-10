/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarfishOverflowStatus__
#define __StarfishOverflowStatus__

#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameReplaced.h"

namespace Starfish {

struct OverflowStatus {
    Frame* m_child;
    FrameBox* m_absChild;
    bool m_seenContainingBlockForAbsBlock;
    bool m_seenAbsBlock;
    bool m_seenFixedBlock;
    OverflowStatus(Frame* child)
    {
        reset(child);
    }

    bool canApplyOverflow(Frame* parent, bool considerIFrame = true)
    {
        if (!parent) {
            return false;
        }

        if (!parent->style()) {
            STARFISH_ASSERT(parent->isLineBox());
            return false;
        }

        if (considerIFrame && parent->isFrameReplaced() &&
            parent->asFrameReplaced()->isFrameReplacedIFrame()) {
            return true;
        }

        if (!m_seenAbsBlock && parent->isAbsolutePositioned()) {
            m_seenAbsBlock = true;
            m_absChild = parent->asFrameBox();
        }

        if (m_seenAbsBlock) {
            if (m_seenFixedBlock) {
                return false;
            }
            if (parent->style()->position() ==
                PositionValue::FixedPositionValue) {
                m_seenFixedBlock = true;
                if (m_child && m_child->style() &&
                    m_child->style()->position() ==
                        PositionValue::FixedPositionValue) {
                    return false;
                }
                return parent->shouldApplyOverflow();
            } else {
                bool b = parent->canBeContainingBlockOfAbsolutePositionedBox(
                    m_absChild);
                if (!m_seenContainingBlockForAbsBlock && b) {
                    if (parent->style()->position() == RelativePositionValue) {
                        m_seenAbsBlock = false;
                        return parent->shouldApplyOverflow();
                    }
                }
                m_seenContainingBlockForAbsBlock =
                    b || m_seenContainingBlockForAbsBlock;
                return b && parent->shouldApplyOverflow();
            }
        }

        return parent->shouldApplyOverflow();
    }

    void reset(Frame* f)
    {
        m_child = f;
        if (m_child->isAbsolutePositioned()) {
            m_absChild = f->asFrameBox();
            m_seenAbsBlock = true;
        } else {
            m_absChild = nullptr;
            m_seenAbsBlock = false;
        }
        m_seenContainingBlockForAbsBlock = false;
        m_seenFixedBlock = false;
    }
};

} // namespace Starfish

#endif
