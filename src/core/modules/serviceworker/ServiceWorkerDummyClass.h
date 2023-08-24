/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if !defined(__StarfishWorkerDummyClass__)
#define __StarfishWorkerDummyClass__

// These dummy classes are only needed for the process version of the service
// worker.

namespace Starfish {

#if defined(SERVICE_WORKER_USE_SEPARATE_PROCESS) && \
    defined(STARFISH_SERVICE_WORKER_HOST)

class Element;
class String;

class Document {
public:
    Element* documentElement()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }

    bool isXMLDocument()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return false;
    }

    ReferrerPolicy referrerPolicy()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return ReferrerPolicy::Empty;
    }

    ScriptValue scriptValue()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }

    bool dispatchEventByUA(Event* event)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return false;
    }
};

class XMLSerializer {
public:
    static String* serializeToXML(Element* e, bool includeSelf)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }
};

class DOMParser {
public:
    DOMParser(Document* document)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    Document* parseFromString(String* str, String* type)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }
};

#endif

#if !defined(STARFISH_ENABLE_WORKER)

class WorkerGlobalScope {
public:
    bool dispatchEventByUA(Event* event)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return false;
    }
};

#endif

} // namespace Starfish

#endif
