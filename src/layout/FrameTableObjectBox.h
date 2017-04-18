/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishFrameTableObjectBox__
#define __StarFishFrameTableObjectBox__

#include "layout/FrameBlockBox.h"

namespace StarFish {

// FrameTableObjectBox is an abstract class where
// common table-related methods are implemented

class FrameTableObjectBox : public FrameBlockBox {
public:
    FrameTableObjectBox(Node* node, ComputedStyle* style);
    virtual const char* name() = 0;
    virtual bool isFrameTableObjectBox()
    {
        return true;
    }

    bool bgColorFromAttribute(Color* ret)
    {
        if (!(node() && node()->isElement() &&
              node()->asElement()->isHTMLElement())) {
            return false;
        }

        HTMLElement* elem = node()->asElement()->asHTMLElement();
        if (!(elem->isHTMLTableElement() || elem->isHTMLTDElement() ||
              elem->isHTMLTHElement())) {
            return false;
        }

        String* color = nullptr;
        if (elem->isHTMLTableElement()) {
            color = elem->asHTMLTableElement()->bgColor();
        } else if (elem->isHTMLTDElement()) {
            color = elem->asHTMLTDElement()->bgColor();
        } else if (elem->isHTMLTHElement()) {
            color = elem->asHTMLTHElement()->bgColor();
        }

        if (color && (!color->equals(String::emptyString))) {
            CSSStyleValuePair pair;
            if (pair.updateValueUnitColor(color)) {
                *ret = pair.colorValue();
                return true;
            }
        }
        return false;
    }

protected:
    void paintBorders(Canvas* canvas, LayoutRect& rect);
};
}

#endif
