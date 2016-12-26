/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

namespace StarFish {

class Node;
class Document;
class Element;

class FrameBlockBox;
class Frame;
class ComputedStyle;
class FrameTextTextDecorationData;
class FrameInline;

class FrameTable;
class FrameTableCaption;

class FrameTreeBuilderContext {
public:
    FrameTreeBuilderContext(FrameBlockBox* currentBlockContainer);
    void setCurrentBlockContainer(FrameBlockBox* blockContainer);
    void setCurrentTextDecorationData(FrameTextTextDecorationData* deco);
    FrameBlockBox* currentBlockContainer();
    void computeTextDecorationData(ComputedStyle* style);
    void mergeTextDecorationData(ComputedStyle* style);
    FrameTextTextDecorationData* currentDecorationData();
    std::unordered_map<Node*, FrameInline*>& frameInlineItem();
    bool isInFrameInlineFlow();
    void setIsInFrameInlineFlow(bool b);

protected:
    bool m_isInFrameInlineFlow;
    FrameBlockBox* m_currentBlockContainer;
    FrameTextTextDecorationData* m_currentDecorationData;
    std::unordered_map<Node*, FrameInline*, std::hash<Node*>, std::equal_to<Node*>> m_frameInlineItem;
};

class FrameTreeBuilder {
    // table layout algorithm needs to call buildTree()
    friend FrameTable;
    friend FrameTableCaption;

public:
    static void buildFrameTree(Document* document);
    static void clearTree(Node* current);
#ifdef STARFISH_ENABLE_TEST
    // debug function
    static void dumpFrameTree(Document* document);
#endif

private:
    static void buildTree(Node* current, FrameTreeBuilderContext& ctx, bool force);
    static void frameBlockBoxChildInserter(FrameBlockBox* frameBlockBox, Frame* currentFrame, Node* currentNode, FrameTreeBuilderContext& ctx);
};

}

#endif
