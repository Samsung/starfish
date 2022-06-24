/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFileReader__
#define __StarfishFileReader__

#include "core/dom/EventTarget.h"
#include "binding/generated/DOMStringOrArrayBufferUnion.h"

namespace Starfish {

class Blob;
class FileReader : public EventTarget {
public:
    enum class ReadyState : uint8_t {
        Empty,
        Loading,
        Done,
    };

    enum class ProgressState {
        None,
        LoadStart,
        Progress,
        Load,
        InError,
        Abort,
        LoadEnd,
    };

    FileReader(ExecutionContext* executionContext);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(FileReader)

    virtual ExecutionContext* executionContext() const override;

    void readAsArrayBuffer(Blob* blob);
    void readAsBinaryString(Blob* blob);
    void readAsText(Blob* blob);
    void readAsText(Blob* blob, String* encoding);
    void readAsDataURL(Blob* blob);

    void abort();

    uint8_t readyState() const;

    Nullable<DOMStringOrArrayBuffer> result() const;

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(progress);
    DECLARE_EVENT_LISTENER(abort);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(loadend);
#undef VIRTUAL
#undef OVERRIDE

private:
    void read(Blob* blob, String* encoding);
    void read(Blob* blob);
    void dispatchProgressEvent(ProgressState progState);

    ExecutionContext* m_executionContext;
    ReadyState m_readyState;
    Blob* m_blob;
    String* m_result;
    size_t m_requstedIdler;
};

} // namespace Starfish

#endif
