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

#ifndef __StarfishRenderedText__
#define __StarfishRenderedText__

namespace Starfish {

class Document;
class DocumentFragment;
class Element;
class String;

// The text an element shows, as the innerText and outerText getters return
// it (HTML, "rendered text collection steps"). The caller flushes style and
// the frame tree first and only passes an element that is being rendered.
String* collectRenderedText(Element* element);

// The nodes the innerText and outerText setters insert for a string (HTML,
// "rendered text fragment"): a Text node per run and a br per line break.
DocumentFragment* renderedTextFragment(Document* document, String* input);

} // namespace Starfish

#endif
