/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
class FrameTableObjectBox;
class Node;
class StyleResolver;
class SVGElement;
class PseudoElement;

class FrameTreeBuilderContext {
public:
    FrameTreeBuilderContext(FrameBlockBox* currentBlockContainer);
    void setCurrentBlockContainer(FrameBlockBox* blockContainer);
    FrameBlockBox* currentBlockContainer();
    void setLastAnonymousTableObjectParent(FrameTableObjectBox* parent);
    FrameTableObjectBox* lastAnonymousTableObjectParent();
    std::unordered_map<Node*, FrameInline*>& frameInlineItem();
    bool isInFrameInlineFlow() const;
    void setIsInFrameInlineFlow(bool b);
    bool isInFrameFlexFlow() const;
    void setIsInFrameFlexFlow(bool b);
    bool isInFrameTableFlow() const;

protected:
    bool m_isInFrameInlineFlow;
    bool m_isInFrameFlexFlow;
    FrameBlockBox* m_currentBlockContainer;
    FrameTableObjectBox* m_lastAnonymousTableObjectParent;
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
    static void needsFrameTreeBuildFromChildrenOfThisFrame(Frame* f);
    static Frame* findNearestBlock(Frame* f);

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
    static String* dumpFrameTreeAsText(Document* document, unsigned depth);
#endif

private:
    static Frame* createFrame(Node* current, FrameTreeBuilderContext& ctx,
                              bool force);
    static Frame* buildTree(Node* current, FrameTreeBuilderContext& ctx,
                            bool force);
    static void insertChild(FrameBlockBox* blockContainer, Frame* currentFrame,
                            Node* currentNode, FrameTreeBuilderContext& ctx);
    static void insertFlexItemChild(FrameBlockBox* blockContainer,
                                    Frame* currentFrame, Node* currentNode,
                                    FrameTreeBuilderContext& ctx);
    static void insertTableObjectChild(
        FrameBlockBox* blockContainer,
        FrameTableObjectBox* lastAnonymousTableObjectParent,
        Frame* currentFrame, Node* currentNode, FrameTreeBuilderContext& ctx);
    static FrameTableObjectBox* createAnonymousTableObjectParent(
        FrameBlockBox* blockContainer,
        FrameTableObjectBox* lastAnonymousTableObjectParent,
        Frame* currentFrame, Node* currentNode, FrameTreeBuilderContext& ctx);
};
}

#endif
