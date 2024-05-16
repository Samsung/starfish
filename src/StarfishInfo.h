/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishInfo__
#define __StarfishInfo__

#define APP_NAME "Netscape"
#define APP_CODE_NAME "Mozilla"
#define PRODUCT_NAME "Gecko"
#define STARFISH_NAME "Starfish"
#define VENDOR_NAME "Samsung Electronics Co., Ltd."
#define VERSION STARFISH_VERSION_STR
#define USER_AGENT(STARFISH_NAME, VERSION) \
    "Mozilla/5.0 (like Gecko/54.0 Firefox/54.0) " STARFISH_NAME "/" VERSION
#define USER_AGENT_MAXIMUM_DATE_VALUE 8.64e15

#endif
