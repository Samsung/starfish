/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameTreeBuilder__
#define __StarFishFrameTreeBuilder__

#include "core/style/Style.h"

namespace StarFish {

class ComputedStyle;
class Document;
class Element;
class Frame;
class FrameBlockBox;
class FrameInline;
class FrameTableCaptionBox;
class FrameTableCellBox;
class Node;
class StyleResolver;
class SVGElement;
class PseudoElement;

class FrameTreeBuilderContext {
public:
    FrameTreeBuilderContext(FrameBlockBox* currentBlockContainer);
    void setCurrentBlockContainer(FrameBlockBox* blockContainer);
    FrameBlockBox* currentBlockContainer();
    std::unordered_map<Node*, FrameInline*>& frameInlineItem();
    bool isInFrameInlineFlow() const;
    void setIsInFrameInlineFlow(bool b);
    bool isInFrameFlexFlow() const;
    void setIsInFrameFlexFlow(bool b);

protected:
    bool m_isInFrameInlineFlow;
    bool m_isInFrameFlexFlow;
    FrameBlockBox* m_currentBlockContainer;
    std::unordered_map<Node*, FrameInline*, std::hash<Node*>,
                       std::equal_to<Node*>>
        m_frameInlineItem;
};

class FrameTreeBuilder {
    // To make code more readable and maintainable, table-related code are
    // in separate FrameTableXXX files. To reuse the FrameTree building
    // algorithm, the following FrameTableXXX classes need to access
    // buildTree(). Hence, they are declared as friends of FrameTreeBuilder.
    friend FrameTableCaptionBox;
    friend FrameTableCellBox;

public:
    static void buildFrameTree(Document* document);
    static void clearTree(Node* current);

    static void createPseudoElement(Node* parent,
                                    StyleResolver::PseudoElementType pseudoId,
                                    FrameTreeBuilderContext& ctx);
    static ComputedStyle* pseudoStyleForElementInternal(
        Node* node, StyleResolver::PseudoElementType pseudoId,
        ComputedStyle* parentStyle);

    static Frame* buildSVGFrameTree(SVGElement* svgElement);
#ifdef STARFISH_ENABLE_TEST
    // debug function
    static void dumpFrameTree(Document* document, unsigned depth);
#endif

private:
    static Frame* createFrame(Node* current, FrameTreeBuilderContext& ctx,
                              bool force);
    static Frame* buildTree(Node* current, FrameTreeBuilderContext& ctx,
                            bool force);
    static void insertChild(FrameBlockBox* frameBlockBox, Frame* currentFrame,
                            Node* currentNode, FrameTreeBuilderContext& ctx);
};
}

#endif
