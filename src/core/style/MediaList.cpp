/*
 * Copyright (C) 2004, 2006, 2008, 2009, 2010, 2012 Apple Inc. All rights
 * reserved.
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/style/MediaList.h"
#include "core/style/MediaQuery.h"
#include "core/style/MediaQuerySet.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {

MediaList::MediaList(MediaQuerySet* mediaQuerySet)
    : ScriptWrappable(this)
    , m_mediaQuerySet(mediaQuerySet)
{
}

ScriptBindingInstance* MediaList::scriptBindingInstance()
{
    return m_mediaQuerySet->document()->scriptBindingInstance();
}

MediaQuerySet* MediaList::mediaQuerySet() const
{
    return m_mediaQuerySet;
}

void MediaList::setMediaQuerySet(MediaQuerySet* mediaQuerySet)
{
    m_mediaQuerySet = mediaQuerySet;
}

String* MediaList::mediaText() const
{
    return m_mediaQuerySet->mediaText();
}

// TODO: need to handle the style sheets individually.
void MediaList::modifyStyleSheet()
{
    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setWholeDocumentNeedsStyleRecalc();
}

void MediaList::setMediaText(String* text)
{
    m_mediaQuerySet->set(text);
    modifyStyleSheet();
}

unsigned MediaList::length() const
{
    return m_mediaQuerySet->queryVector().size();
}

String* MediaList::item(unsigned index) const
{
    const GCVector<MediaQuery*> queries = m_mediaQuerySet->queryVector();
    if (index < queries.size()) {
        return queries[index]->cssText();
    }
    return String::emptyString;
}

void MediaList::appendMedium(String* newMedium)
{
    m_mediaQuerySet->add(newMedium);
    modifyStyleSheet();
}

void MediaList::deleteMedium(String* oldMedium)
{
    if (!m_mediaQuerySet->remove(oldMedium)) {
        StringBuilder msg;
        msg.appendString("Failed to delete '");
        msg.appendString(oldMedium);
        msg.appendString("'.");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::NOT_FOUND_ERR, s.data());
    }
    modifyStyleSheet();
}
}
