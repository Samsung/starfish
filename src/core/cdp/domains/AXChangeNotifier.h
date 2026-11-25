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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAXChangeNotifier__)
#define __StarfishCDPAXChangeNotifier__

namespace Starfish {

class Document;
class Node;

// Report a DOM change that can change what the accessibility tree says
// about `node`. The Accessibility domain turns it into the
// Accessibility.nodesUpdated event, and drops it when no client asked for
// that node.
//
// Call sites pass the node whose own accessibility data changed. For a
// child being added or removed that is the parent, because the change is
// to the parent's children.
void notifyAXNodeChanged(Node* node);

// True once a client has turned the Accessibility domain on. DOM mutation
// sites call in from a hot path, so the case that matters, no client
// listening, costs one load and a branch. It is never cleared: a page that
// was inspected once keeps the slow path for the rest of its life, which is
// cheaper than tracking every session's state from here.
extern bool g_axNodeChangesObserved;

inline void notifyAXNodeChangedIfObserved(Node* node)
{
    if (g_axNodeChangesObserved) {
        notifyAXNodeChanged(node);
    }
}

// The document finished loading, so its accessibility tree is complete.
// Turns into the Accessibility.loadComplete event.
void notifyAXLoadComplete(Document* document);

} // namespace Starfish

#endif
