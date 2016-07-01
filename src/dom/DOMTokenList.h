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

#ifndef __StarFishDOMTokenList__
#define __StarFishDOMTokenList__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Node;
class Element;

// https://dom.spec.whatwg.org/#interface-domtokenlist
class DOMTokenList : public ScriptWrappable {
public:
    DOMTokenList(ScriptBindingInstance* instance, Element* element, QualifiedName localName)
        : ScriptWrappable(this), m_element(element), m_localName(localName)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    static void tokenize(std::vector<String*, gc_allocator<String*>>* tokens, String* src);
    static void concatTokensInsideParentheses(std::vector<String*, gc_allocator<String*>>* tokens);
    unsigned long length();
    String* item(unsigned long index);
    bool contains(String* token);
    String* addSingleToken(String* src, std::vector<String*, gc_allocator<String*>>* tokens, String* token);
    void add(std::vector<String*, gc_allocator<String*>>* tokens);
    int checkMatchedTokens(bool* flags, std::vector<String*, gc_allocator<String*>>* tokens, String* token);
    void remove(String* token);
    void remove(std::vector<String*, gc_allocator<String*>>* tokens);
    bool toggle(String* token, bool isForced, bool forceValue);
    String* toString();
    void validateToken(String* token); // Throw Exceptions
private:
    Element* m_element;
    QualifiedName m_localName;
};

}

#endif
