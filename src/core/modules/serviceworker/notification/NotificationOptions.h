/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishNotificationOptions__
#define __StarfishNotificationOptions__

#include "binding/ScriptWrappable.h"

namespace Starfish {

enum class NotificationPermission { Default, Denied, Granted };
enum class NotificationDirection { Auto, Ltr, Rtl };

struct NotificationOptions {
public:
    NotificationOptions()
        : m_title(String::emptyString)
        , m_body(String::emptyString)
        , m_tag(String::emptyString)
        , m_origin(String::emptyString)
    {
    }

    DEFINE_GETTER_SETTER(String*, title, Title);
    DEFINE_GETTER_SETTER(String*, body, Body);
    DEFINE_GETTER_SETTER(String*, tag, Tag);
    DEFINE_GETTER_SETTER(String*, origin, Origin);

private:
    String* m_title;
    String* m_body;
    String* m_tag;
    String* m_origin;
};
} // namespace Starfish

#endif
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
