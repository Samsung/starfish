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

#ifndef __FrameTableTreeBuilder__
#define __FrameTableTreeBuilder__

#include "style/Style.h"

namespace StarFish {

class Node;
class FrameBlockBox;
class FrameTreeBuilderContext;
class FrameTableObjectBox;

class FrameTableTreeBuilder {
public:
    static FrameTableObjectBox* buildFrameTableTree(
        Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static bool isProperChildDisplayValueType(DisplayValue parent,
                                              DisplayValue current);
    static bool isProperDescendantDisplayValueType(DisplayValue parent,
                                                   DisplayValue current);
    static FrameTableObjectBox* createFrameTableObjectBox(Node* current);
    static FrameTableObjectBox* createAnonymousFrameTableObjectBoxWithParent(
        FrameBlockBox* parent, Node* current);

    static bool isTableWrapperDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableDisplayValue) ||
               (display == DisplayValue::InlineTableDisplayValue);
    }
    static bool isTableCaptionDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableCaptionDisplayValue);
    }
    static bool isTableRowGroupDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableHeaderGroupDisplayValue) ||
               (display == DisplayValue::TableRowGroupDisplayValue) ||
               (display == DisplayValue::TableFooterGroupDisplayValue);
    }
    static bool isTableRowDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableRowDisplayValue);
    }
    static bool isTableColumnGroupDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableColumnGroupDisplayValue);
    }
    static bool isTableColumnDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableColumnDisplayValue);
    }
    static bool isTableCellDisplayValue(DisplayValue display)
    {
        return (display == DisplayValue::TableCellDisplayValue);
    }

private:
    FrameTableTreeBuilder()
    {
    }
    ~FrameTableTreeBuilder()
    {
    }
};
}

#endif
