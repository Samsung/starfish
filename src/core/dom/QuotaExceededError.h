/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishQuotaExceededError__
#define __StarfishQuotaExceededError__

#include "core/dom/DOMException.h"
#include "core/dom/QuotaExceededErrorOptions.h"

namespace Starfish {

// https://webidl.spec.whatwg.org/#quotaexceedederror
// The name is "QuotaExceededError" and the code is QUOTA_EXCEEDED_ERR.
class QuotaExceededError : public DOMException {
public:
    // Thrown by the engine. quota and requested are null.
    QuotaExceededError(ExecutionContext* executionContext,
                       const char* message = nullptr);

    // Constructors exposed to script.
    QuotaExceededError(ExecutionContext* executionContext, String* message);
    QuotaExceededError(ExecutionContext* executionContext, String* message,
                       QuotaExceededErrorOptions options);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(QuotaExceededError)

    Optional<double> quota()
    {
        return m_quota;
    }

    Optional<double> requested()
    {
        return m_requested;
    }

private:
    Optional<double> m_quota;
    Optional<double> m_requested;
};
} // namespace Starfish

#endif
