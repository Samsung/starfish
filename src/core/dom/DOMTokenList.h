/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMTokenList__
#define __StarfishDOMTokenList__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class Node;
class Element;

// https://dom.spec.whatwg.org/#interface-domtokenlist
class DOMTokenList : public ScriptWrappable {
public:
    DOMTokenList(Element* element, QualifiedName localName);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMTokenList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    static void tokenize(String* src, GCVector<StringView>& tokens);
    static void concatTokensInsideParentheses(GCVector<String*>* tokens);
    static bool checkDuplicatedToken(GCVector<StringView>& tokens,
                                     StringView token);
    uint32_t length();
    Optional<String*> item(unsigned long index);
    bool contains(String* token);
    String* addSingleToken(String* src, const GCVector<StringView>& tokens,
                           String* token);
    void add(GCVector<String*>& tokens);
    int checkMatchedTokens(bool* flags, const GCVector<StringView>& tokens,
                           String* token);
    void remove(String* token);
    void remove(GCVector<String*>& tokens);

    bool toggle(String* token);
    bool toggle(String* token, bool forceValue);
    bool toggle(String* token, bool isForced, bool forceValue);
    bool replace(String* token, String* newToken);
    bool supports(String* token);

    String* toString();
    void validateToken(String* token); // Throw Exceptions
    String* value() const;
    void setValue(String* value);

private:
    bool validateTokenValue(String* token); // Throw Exceptions
    bool supportedTokensOfAnchorAndArea(String* token);
    bool supportedTokensOfLink(String* token);
    bool supportedTokensOfMedia(String* token);

    Element* m_element;
    QualifiedName m_localName;
};
} // namespace Starfish

#endif
