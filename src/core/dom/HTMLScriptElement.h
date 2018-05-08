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

#ifndef __StarFishHTMLScriptElement__
#define __StarFishHTMLScriptElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLScriptElement : public HTMLElement {
    friend class ScriptDownloadClient;
    friend class DeferredScriptDownloadClient;

public:
    HTMLScriptElement(Document* document)
        : HTMLElement(document)
        , m_isAlreadyStarted(false)
        , m_isParserInserted(false)
        , m_didScriptExecuted(false)
    {
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("+++tag:script\n");
#endif
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLScriptElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    /* Other methods (not in DOM API) */

    String* src();
    void setSrc(String* src);

    String* type();
    void setType(String* type);

    String* charset();
    void setCharset(String* charset);

    String* text();
    void setText(String* s);

    bool async();
    void setAsync(bool b);

    bool defer();
    void setDefer(bool b);

    virtual void didCharacterDataModified(String* before,
                                          String* after) override;
    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void didNodeInsertedToDocumentTree() override;
    virtual Node* clone() override;
    void markParserInserted()
    {
        m_isParserInserted = true;
    }
    void clearParserInserted()
    {
        m_isParserInserted = false;
    }
    bool didScriptExecuted()
    {
        return m_didScriptExecuted;
    }
    bool executeScript(bool forceSync = false, bool inParser = false);
    bool executeScriptImpl(bool forceSync, bool inParser);
    void markScriptExecuted()
    {
        m_isAlreadyStarted = true;
        m_didScriptExecuted = true;
    }

    bool isValidClassicScriptType();
    bool isValidScriptType();
    bool isEventForSupported();

    bool blockForNoModule();

protected:
    bool m_isAlreadyStarted;
    bool m_isParserInserted;
    bool m_didScriptExecuted;
};
}

#endif
