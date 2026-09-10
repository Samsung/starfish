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

#ifndef __StarfishCLISocketPath__
#define __StarfishCLISocketPath__

#include <string>

namespace StarfishCLI {

// bind() and connect() take the socket path inside sockaddr_un, whose
// sun_path field holds 108 bytes. The session socket sits under $HOME, and
// $HOME alone can be longer than that, so passing the whole path would make
// the CLI unusable for those users.
//
// Both calls resolve a relative path against the calling process's current
// directory, and the kernel measures only the string it is given. These two
// functions therefore change directory to the socket's parent, pass just the
// file name, and change back. Path depth then stops counting.
//
// Both return an open descriptor, or -1. The current directory is restored
// either way. Neither is safe to call from more than one thread, because the
// current directory belongs to the whole process.

// Creates the listening socket. The parent directory must already exist.
// Applies mode to the socket file and starts listening with backlog.
int bindUnixSocket(const std::string& path, int mode, int backlog);

// Connects to an existing socket.
int connectUnixSocket(const std::string& path);

} // namespace StarfishCLI

#endif
