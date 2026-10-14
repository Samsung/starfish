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

#include "StarfishConfig.h"
#include "core/dom/QuotaExceededError.h"

namespace Starfish {

QuotaExceededError::QuotaExceededError(ExecutionContext* executionContext,
                                       const char* message)
    : DOMException(executionContext, QUOTA_EXCEEDED_ERR, message)
{
}

QuotaExceededError::QuotaExceededError(ExecutionContext* executionContext,
                                       String* message)
    : QuotaExceededError(executionContext, message, QuotaExceededErrorOptions())
{
}

QuotaExceededError::QuotaExceededError(ExecutionContext* executionContext,
                                       String* message,
                                       QuotaExceededErrorOptions options)
    : DOMException(executionContext, QUOTA_EXCEEDED_ERR)
{
    // https://webidl.spec.whatwg.org/#dom-quotaexceedederror-quotaexceedederror
    // The steps check quota and requested before they set any member.
    if (options.hasQuota() && options.quota() < 0) {
        throw new DOMException(executionContext, SCRIPT_RANGE_ERR,
                               "The quota must not be negative.");
    }
    if (options.hasRequested() && options.requested() < 0) {
        throw new DOMException(executionContext, SCRIPT_RANGE_ERR,
                               "The requested value must not be negative.");
    }
    if (options.hasQuota() && options.hasRequested() &&
        options.requested() > options.quota()) {
        throw new DOMException(
            executionContext, SCRIPT_RANGE_ERR,
            "The requested value must not be greater than the quota.");
    }

    setMessage(message);
    if (options.hasQuota()) {
        m_quota = options.quota();
    }
    if (options.hasRequested()) {
        m_requested = options.requested();
    }
}

ScriptBindingInstance* QuotaExceededError::scriptBindingInstance()
{
    return DOMException::scriptBindingInstance();
}
} // namespace Starfish
