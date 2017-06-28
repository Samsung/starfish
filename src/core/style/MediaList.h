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

#ifndef __StarFishMediaList__
#define __StarFishMediaList__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class MediaQuerySet;
class MediaList : public ScriptWrappable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMediaList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    MediaList(MediaQuerySet* mediaQuerySet);

    String* mediaText() const;
    void setMediaText(String* text);

    unsigned length() const;
    String* item(unsigned index) const;
    void appendMedium(String* newMedium);
    void deleteMedium(String* oldMedium);

    const MediaQuerySet* querySet() const
    {
        return m_mediaQuerySet;
    }

protected:
    MediaQuerySet* m_mediaQuerySet;
};
}

#endif
