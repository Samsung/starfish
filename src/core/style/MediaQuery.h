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
 * CSS Media Query
 *
 * Copyright (C) 2005, 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __StarFishMediaQuery__
#define __StarFishMediaQuery__

namespace StarFish {

class MediaQueryExp;
class MediaQuery : public gc {
public:
    enum RestrictorType { Only, Not, None };

    static MediaQuery* create(RestrictorType, String* mediaType,
                              GCVector<MediaQueryExp*> expressions);
    static MediaQuery* createNotAll();

    RestrictorType restrictor() const
    {
        return m_restrictor;
    }
    const GCVector<MediaQueryExp*>& expressions() const
    {
        return m_expressions;
    }

    String* mediaType() const
    {
        return m_mediaType;
    }

    bool operator==(const MediaQuery& other) const;

    String* serialize() const;
    String* cssText() const;

protected:
    MediaQuery(RestrictorType, String* mediaType,
               GCVector<MediaQueryExp*> expressions);
    MediaQuery(const MediaQuery& o);

    RestrictorType m_restrictor;
    String* m_mediaType;
    GCVector<MediaQueryExp*> m_expressions;
};
}

#endif
