/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishDOM__
#define __StarFishDOM__

// When you want to add new dom, you have to add dom manually.
// And please keep them in alphabetic order with proper ifdef guard.

#include "dom/Attr.h"
#include "dom/CDATASection.h"
#include "dom/CharacterData.h"
#include "dom/Comment.h"
#include "dom/CSSStyleDeclaration.h"
#include "dom/CSSStyleRule.h"
#include "dom/Document.h"
#include "dom/DocumentFragment.h"
#include "dom/DocumentType.h"
#include "dom/DOMException.h"
#include "dom/DOMPoint.h"
#include "dom/DOMPointReadOnly.h"
#include "dom/DOMQuad.h"
#include "dom/DOMRect.h"
#include "dom/DOMRectList.h"
#include "dom/DOMRectReadOnly.h"
#include "dom/DOMSettableTokenList.h"
#include "dom/DOMTokenList.h"
#include "dom/Element.h"
#include "dom/Event.h"
#include "dom/EventTarget.h"
#include "dom/FocusEvent.h"
#include "dom/HTMLBodyElement.h"
#include "dom/HTMLBRElement.h"
#include "dom/HTMLCaptionElement.h"
#include "dom/HTMLColElement.h"
#include "dom/HTMLColGroupElement.h"
#include "dom/HTMLCollection.h"
#include "dom/HTMLDivElement.h"
#include "dom/HTMLDocument.h"
#include "dom/HTMLElement.h"
#include "dom/HTMLHtmlElement.h"
#include "dom/HTMLHeadElement.h"
#include "dom/HTMLHeadingElement.h"
#include "dom/HTMLHtmlElement.h"
#include "dom/HTMLImageElement.h"
#include "dom/HTMLLIElement.h"
#include "dom/HTMLLinkElement.h"
#include "dom/HTMLMetaElement.h"
#include "dom/HTMLObjectElement.h"
#include "dom/HTMLParagraphElement.h"
#include "dom/HTMLPreElement.h"
#include "dom/HTMLScriptElement.h"
#include "dom/HTMLSpanElement.h"
#include "dom/HTMLStrongElement.h"
#include "dom/HTMLStyleElement.h"
#include "dom/HTMLTableCellElement.h"
#include "dom/HTMLTableElement.h"
#include "dom/HTMLTBodyElement.h"
#include "dom/HTMLTDElement.h"
#include "dom/HTMLTFootElement.h"
#include "dom/HTMLTHeadElement.h"
#include "dom/HTMLTHElement.h"
#include "dom/HTMLTRElement.h"
#include "dom/HTMLUListElement.h"
#include "dom/HTMLUnknownElement.h"
#include "dom/KeyboardEvent.h"
#include "dom/MouseEvent.h"
#include "dom/NamedNodeMap.h"
#include "dom/Node.h"
#include "dom/NodeList.h"
#include "dom/ProgressEvent.h"
#include "dom/PseudoElement.h"
#include "dom/PseudoElementData.h"
#include "dom/Text.h"
#include "dom/TouchEvent.h"
#include "dom/UIEvent.h"

#ifdef STARFISH_EXP
#include "dom/DOMImplementation.h"
#endif

#ifdef STARFISH_ENABLE_DOMPARSER
#include "dom/DOMParser.h"
#endif

#ifdef STARFISH_ENABLE_MULTI_PAGE
#include "dom/HTMLAnchorElement.h"
#endif

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "dom/HTMLAudioElement.h"
#include "dom/HTMLMediaElement.h"
#include "dom/HTMLSourceElement.h"
#include "dom/HTMLTrackElement.h"
#include "dom/HTMLVideoElement.h"
#include "dom/TextTrack.h"
#include "dom/TextTrackCue.h"
#include "dom/TextTrackCueList.h"
#include "dom/TextTrackList.h"
#include "dom/VTTCue.h"
#endif

#endif
