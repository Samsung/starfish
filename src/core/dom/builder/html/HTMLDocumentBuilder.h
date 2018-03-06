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

#ifndef __StarFishHTMLDocumentBuilderElement__
#define __StarFishHTMLDocumentBuilderElement__

#include "core/dom/builder/DocumentBuilder.h"

namespace StarFish {

class Window;
class Resource;
class HTMLParser;
class FormDataSetItem;

class HTMLDocumentBuilder : public DocumentBuilder {
    friend class HTMLResourceClient;

public:
    HTMLDocumentBuilder(Document* document)
        : DocumentBuilder(document)
        , m_parser(nullptr)
        , m_resource(nullptr)
    {
    }

    virtual void build(ResourceURL* url, ResourceURL* referrerURL);
    virtual void build(String* str);
    virtual void resume();
    void openFunctionExplicitCalled();

    virtual bool isHTMLDocumentBuilder()
    {
        return true;
    }

    HTMLParser* parser()
    {
        return m_parser;
    }

protected:
    HTMLParser* m_parser;
    Resource* m_resource;

private:
    String* encodeFormDataSet(GCVector<FormDataSetItem*>* formDataSet,
                              String* formEnctype);
};
}

#endif
