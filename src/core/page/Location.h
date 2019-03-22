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

#ifndef __StarfishLocation__
#define __StarfishLocation__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace Starfish {

class Starfish;
class DocumentURL;

class Location : public ScriptWrappable, public DocumentHoldable {
    friend class HTMLFormElement;

public:
    Location(Document* document);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Location)

    ResourceURL* url();
    String* href();
    String* origin();
    String* host();
    String* hostname();
    String* port();
    String* protocol();
    String* pathname();
    String* search();
    String* hash();

    void assign(String* url);
    void assign(String* url, ReferrerURL* referrerURL);
    void replace(String* url);
    void reload(bool force = true);

    void setHref(String* newURL);
    void setHost(String* newHost);
    void setHostname(String* newHostname);
    void setPort(String* newPort);
    void setProtocol(String* newProtocol);
    void setPathname(String* newPath, bool needRemovingDots = true);
    void setSearch(String* search);
    void setHash(String* search);

    void dispose()
    {
    }

    void setLocation(String* newURL, ReferrerURL* referrerURL);

private:
    void assign(ResourceURL* url, bool force = false);
    void assign(ResourceURL* url, ReferrerURL* referrerURL, bool force = false);
};
}

#endif
