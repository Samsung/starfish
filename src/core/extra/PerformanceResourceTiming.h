/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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
#if !defined(__StarfishPerformanceResourceTiming__)
#define __StarfishPerformanceResourceTiming__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class PerformanceResourceTiming : public ScriptWrappable {
    friend class Window;
    friend class Document;
    friend class BrowsingContext;
    friend class HTMLResourceClient;
    friend class ResourceLoader;

public:
    PerformanceResourceTiming(ExecutionContext* executionContext);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PerformanceResourceTiming)

    double domContentLoadedEventEnd() const
    {
        return m_domContentLoadedEventEnd;
    }
    double domContentLoadedEventStart() const
    {
        return m_domContentLoadedEventStart;
    }
    double domainLookupEnd() const
    {
        return m_responseStart;
    }
    double domainLookupStart() const
    {
        return m_requestStart;
    }
    double fetchStart() const
    {
        return m_requestStart;
    }
    double loadEventEnd() const
    {
        return m_loadEventEnd;
    }
    double loadEventStart() const
    {
        return m_loadEventStart;
    }
    double navigationStart() const
    {
        return m_requestStart;
    }
    double requestStart() const
    {
        return m_requestStart;
    }
    double responseEnd() const
    {
        return m_responseEnd;
    }
    double responseStart() const
    {
        return m_responseStart;
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    double m_requestStart;
    double m_responseStart;
    double m_responseEnd;
    double m_domContentLoadedEventStart;
    double m_domContentLoadedEventEnd;
    double m_loadEventStart;
    double m_loadEventEnd;
};
} // namespace Starfish

#endif
