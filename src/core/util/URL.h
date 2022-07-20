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

#ifndef __StarfishURL__
#define __StarfishURL__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class Blob;
class MediaSource;
class URLSearchParams;

class URL : public ScriptWrappable {
public:
    URL(ExecutionContext* executionContext, String* url);
    URL(ExecutionContext* executionContext, String* url, String* baseURL);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(URL)

    static void revokeObjectURL(ExecutionContext* executionContext,
                                String* blobURLRef);
    static String* createObjectURL(Blob* blob);
#if defined(STARFISH_ENABLE_MULTIMEDIA)
    static String* createObjectURL(MediaSource* mediaSource);
#endif

    String* origin();
    String* href();
    void setHref(String* newHref);
    String* protocol();
    void setProtocol(String* newProtocol);
    String* username();
    void setUsername(String* newUsername);
    String* password();
    void setPassword(String* newPassword);
    String* host();
    void setHost(String* newHost);
    String* hostname();
    void setHostname(String* newHostname);
    String* port();
    void setPort(String* newPort);
    String* pathname();
    void setPathname(String* newPath, bool needRemovingDots = true);
    String* search();
    void setSearch(String* newSearch, bool needsToUpdateSearchParams = true);
    String* hash();
    void setHash(String* newHash);

    URLSearchParams* searchParams();

protected:
    ExecutionContext* m_executionContext;
    ResourceURL* m_resourceURL;
    Nullable<URLSearchParams*> m_searchParams;
};
} // namespace Starfish

#endif
