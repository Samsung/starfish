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
    MediaQuerySet* mediaQuerySet() const;
    void setMediaQuerySet(MediaQuerySet* mediaQuerySet);

    void modifyStyleSheet();

protected:
    MediaQuerySet* m_mediaQuerySet;
};
}

#endif
