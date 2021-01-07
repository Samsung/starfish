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

#ifndef __StarfishSVGScriptElement__
#define __StarfishSVGScriptElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

// TODO: SVGScriptElement is a simple copy of HTMLScriptElement. We should
// combine these component later.
class SVGScriptElement : public SVGElement {
    friend class SVGScriptDownloadClient;
    friend class DeferredSVGScriptDownloadClient;

public:
    SVGScriptElement(Document* document, const QualifiedName& qname)
        : SVGElement(document, qname)
        , m_isAlreadyStarted(false)
        , m_isParserInserted(false)
        , m_didScriptExecuted(false)
        , m_shouldResumeParsing(false)
        , m_nonce(nullptr)
    {
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("+++tag:script\n");
#endif
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGScriptElement() const override;

    virtual bool needsClipPathAttributes() override
    {
        return false;
    }

    virtual bool needsTransparentAttributes() override
    {
        return false;
    }

    /* Other methods (not in DOM API) */

    String* herf();
    void setHerf(String* herf);

    String* xlinkHref();
    void setXlinkHref(String* xlinkHref);

    String* type();
    void setType(String* type);

    String* charset();
    void setCharset(String* charset);

    Nullable<String*> crossOrigin();
    void setCrossOrigin(Nullable<String*> crossOrigin);

    String* text();
    void setText(String* s);

    bool async();
    void setAsync(bool b);

    bool defer();
    void setDefer(bool b);

    String* nonce() const;
    void setNonce(String* str);

    bool shouldResumeParsing()
    {
        return m_shouldResumeParsing;
    }
    void setShouldResumeParsing(bool b)
    {
        m_shouldResumeParsing = b;
    }

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
    bool m_shouldResumeParsing;
    String* m_nonce;
};
} // namespace Starfish

#endif
