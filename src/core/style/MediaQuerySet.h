/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
/*
 * Copyright (C) 2004, 2006, 2008, 2009, 2010, 2012 Apple Inc. All rights
 * reserved.
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

#ifndef __StarFishMediaQuerySet__
#define __StarFishMediaQuerySet__

namespace StarFish {

class Document;
class MediaQuery;
class MediaQuerySet : public gc {
public:
    static MediaQuerySet* create(Document* document)
    {
        return new MediaQuerySet(document);
    }

    void addMediaQuery(MediaQuery* mediaQuery);

    GCVector<MediaQuery*>& queryVector()
    {
        return m_queries;
    }

    String* mediaText() const;

    MediaQuerySet* create(String* mediaString);
    bool set(String* mediaString);
    void add(String* mediaString);
    bool remove(String* mediaString);

    Document* document() const;

protected:
    MediaQuerySet(Document* document);
    MediaQuerySet(MediaQuerySet& o);
    Document* m_document;
    GCVector<MediaQuery*> m_queries;
};
}

#endif
