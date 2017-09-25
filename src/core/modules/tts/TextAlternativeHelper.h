/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_TTS
#include "binding/StarFishHoldable.h"

namespace StarFish {
class Node;

class TextAlternativeHelper : public gc, public StarFishHoldable {
public:
    TextAlternativeHelper(StarFish* starfish);

    // https://www.w3.org/TR/2014/REC-wai-aria-implementation-20140320/#mapping_additional_nd_te
    String* getComputedTextAlternative(Node* node);

private:
    enum AriaByType { AriaLabelledBy, ArialDescribedBy };

    void appendTextAlternativeIfNeeds(Node* node);
    bool appendTextAlterNative(String* text);
    bool isVisitedNode(Node* node);
    bool isAriaHidden(Node* node);
    bool appendFromAriaByTypeIfNeeds(Node* node, AriaByType type);
    bool appendFromAriaLabelIfNeeds(Node* node);
    bool appendFromAltAttributeIfNeeds(Node* node);
    bool appendFromEmbeddedControlIfNeeds(Node* node);
    bool isEmbeddedControl(Node* node);
    String* finalize();

    GCUnorderedSet<Node*> m_vistiedNodes;
    GCVector<String*> m_textAlts;
    bool m_inAriaLabelledbyOrArialDescribedBy;
};
}
#endif
