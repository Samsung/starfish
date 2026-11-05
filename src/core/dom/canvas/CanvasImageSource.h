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

#ifndef __StarfishCanvasImageSource__
#define __StarfishCanvasImageSource__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/generated/HTMLImageElementOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrImageBitmapUnion.h"
#include "core/dom/DOMExceptionOr.h"

namespace Starfish {
typedef HTMLImageElementOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrImageBitmap
    CanvasImageSource;

class NativeImageData;

// The image a CanvasImageSource draws. A copy made for this call (a video
// frame, a WebGL drawing buffer) is the caller's to delete once it is no
// longer needed; any other image belongs to its source.
struct CanvasImageSourceImage {
    NULLABLE NativeImageData* image = nullptr;
    bool originClean = true;
    bool isCopy = false;
};

class CanvasImageSourceUtils {
public:
    static DOMExceptionOr<bool> checkUsability(
        ExecutionContext* executionContext, CanvasImageSource image);
    static CanvasImageSourceImage toNativeImageData(
        ExecutionContext* executionContext, CanvasImageSource& image);
};
} // namespace Starfish

#endif
#endif
