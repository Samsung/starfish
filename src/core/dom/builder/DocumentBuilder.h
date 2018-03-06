/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishDocumentBuilder__
#define __StarFishDocumentBuilder__

#include "binding/DocumentHoldable.h"
#include "core/dom/Document.h"

namespace StarFish {

class HTMLDocumentBuilder;

class DocumentBuilder : public gc, public DocumentHoldable {
public:
    DocumentBuilder(Document* document)
        : DocumentHoldable(document)
    {
    }

    virtual ~DocumentBuilder()
    {
    }

    virtual void build(ResourceURL* url, ResourceURL* referrerURL) = 0;
    virtual void build(String* str) = 0;
    virtual void resume() = 0;

    virtual bool isHTMLDocumentBuilder()
    {
        return false;
    }

    HTMLDocumentBuilder* asHTMLDocumentBuilder()
    {
        STARFISH_ASSERT(isHTMLDocumentBuilder());
        return (HTMLDocumentBuilder*)this;
    }
};
}

#endif
