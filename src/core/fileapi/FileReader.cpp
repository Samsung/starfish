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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/fileapi/FileReader.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/ProgressEvent.h"
#include "core/fileapi/Blob.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace Starfish {

FileReader::FileReader(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_readyState(ReadyState::Empty)
    , m_blob(nullptr)
    , m_resultInText(nullptr)
    , m_resultInArrayBuffer(nullptr)
    , m_requstedIdler(MessageLoopInvalidID)
{
}

DEFINE_EVENT_LISTENER(FileReader, loadstart);
DEFINE_EVENT_LISTENER(FileReader, progress);
DEFINE_EVENT_LISTENER(FileReader, abort);
DEFINE_EVENT_LISTENER(FileReader, error);
DEFINE_EVENT_LISTENER(FileReader, load);
DEFINE_EVENT_LISTENER(FileReader, loadend);

ExecutionContext* FileReader::executionContext() const
{
    return m_executionContext;
}

ScriptBindingInstance* FileReader::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void FileReader::readAsArrayBuffer(Blob* blob)
{
    readArrayBuffer(blob);
}

void FileReader::readAsBinaryString(Blob* blob)
{
    STARFISH_UNIMPLEMENTED();
}

void FileReader::readAsText(Blob* blob)
{
    readText(blob);
}

void FileReader::readAsText(Blob* blob, String* encoding)
{
    readText(blob, encoding);
}

void FileReader::readAsDataURL(Blob* blob)
{
    STARFISH_UNIMPLEMENTED();
}

void FileReader::abort()
{
    m_resultInText = nullptr;
    m_resultInArrayBuffer = nullptr;

    if (m_readyState == ReadyState::Empty) {
        return;
    }

    if (m_requstedIdler != 0) {
        executionContext()->webBase()->messageLoop()->removeIdler(
            m_requstedIdler);
        m_requstedIdler = 0;
    }

    if (m_readyState == ReadyState::Loading) {
        m_readyState = ReadyState::Done;
    }

    dispatchProgressEvent(ProgressState::Abort);
    dispatchProgressEvent(ProgressState::LoadEnd);
}

uint8_t FileReader::readyState() const
{
    return static_cast<uint8_t>(m_readyState);
}

void FileReader::readText(Blob* blob)
{
    readText(blob, nullptr);
}

void FileReader::readText(Blob* blob, String* encoding)
{
    // If fr’s state is "loading", throw an InvalidStateError DOMException.
    if (m_readyState == ReadyState::Loading) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    m_blob = blob;
    // Set fr’s state to "loading".
    m_readyState = ReadyState::Loading;
    dispatchProgressEvent(ProgressState::LoadStart);

    // Set fr’s result to null.
    m_resultInText = nullptr;
    m_resultInArrayBuffer = nullptr;

    if (encoding && encoding->equals("UTF-16")) {
        // UTF-16 encoding
        m_requstedIdler =
            executionContext()->webBase()->messageLoop()->addIdler(
                executionContext()->globalScope(),
                [](size_t handle, void* data) {
                    FileReader* fr = (FileReader*)data;
                    Blob* blob = fr->m_blob;
                    fr->m_resultInText = String::fromUTF16(
                        (const char16_t*)blob->data(), (blob->size() / 2));
                    fr->m_readyState = ReadyState::Done;

                    if (fr->m_resultInText) {
                        fr->dispatchProgressEvent(ProgressState::Load);
                    } else {
                        fr->dispatchProgressEvent(ProgressState::InError);
                    }
                    fr->dispatchProgressEvent(ProgressState::LoadEnd);
                },
                this);
    } else {
        // UTF-8 encoding
        m_requstedIdler =
            executionContext()->webBase()->messageLoop()->addIdler(
                executionContext()->globalScope(),
                [](size_t handle, void* data) {
                    FileReader* fr = (FileReader*)data;
                    Blob* blob = fr->m_blob;
                    fr->m_resultInText = String::fromUTF8(
                        (const char*)blob->data(), blob->size());
                    fr->m_readyState = ReadyState::Done;

                    if (fr->m_resultInText) {
                        fr->dispatchProgressEvent(ProgressState::Load);
                    } else {
                        fr->dispatchProgressEvent(ProgressState::InError);
                    }
                    fr->dispatchProgressEvent(ProgressState::LoadEnd);
                },
                this);
    }
}

void FileReader::readArrayBuffer(Blob* blob)
{
    // If fr’s state is "loading", throw an InvalidStateError DOMException.
    if (m_readyState == ReadyState::Loading) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    m_blob = blob;
    // Set fr’s state to "loading".
    m_readyState = ReadyState::Loading;
    dispatchProgressEvent(ProgressState::LoadStart);

    // Set fr’s result to null.
    m_resultInText = nullptr;
    m_resultInArrayBuffer = nullptr;

    m_requstedIdler = executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data) {
            FileReader* fr = (FileReader*)data;
            Blob* blob = fr->m_blob;
            void* buffer = malloc(blob->size());
            memcpy(buffer, blob->data(), blob->size());
            fr->m_resultInArrayBuffer = createScriptArrayBuffer(
                fr->executionContext()->scriptBindingInstance(), buffer,
                blob->size());
            fr->m_readyState = ReadyState::Done;

            if (fr->m_resultInArrayBuffer) {
                fr->dispatchProgressEvent(ProgressState::Load);
            } else {
                fr->dispatchProgressEvent(ProgressState::InError);
            }
            fr->dispatchProgressEvent(ProgressState::LoadEnd);
        },
        this);
}

void FileReader::dispatchProgressEvent(ProgressState progState)
{
    String* eventName = String::emptyString;
    if (progState == ProgressState::Progress) {
        eventName = executionContext()
                        ->starfish()
                        ->staticStrings()
                        ->m_progress.localName();
    } else if (progState == ProgressState::InError) {
        eventName = executionContext()
                        ->starfish()
                        ->staticStrings()
                        ->m_error.localName();
    } else if (progState == ProgressState::Abort) {
        eventName = executionContext()
                        ->starfish()
                        ->staticStrings()
                        ->m_abort.localName();
    } else if (progState == ProgressState::Load) {
        eventName =
            executionContext()->starfish()->staticStrings()->m_load.localName();
    } else if (progState == ProgressState::LoadEnd) {
        eventName = executionContext()
                        ->starfish()
                        ->staticStrings()
                        ->m_loadend.localName();
    } else if (progState == ProgressState::LoadStart) {
        eventName = executionContext()
                        ->starfish()
                        ->staticStrings()
                        ->m_loadstart.localName();
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    ProgressEvent* pe = new ProgressEvent(executionContext(), eventName);
    EventTarget::dispatchEventByUA(this, pe);
}

Nullable<DOMStringOrArrayBuffer> FileReader::result() const
{
    if (m_resultInText) {
        // read as text
        return DOMStringOrArrayBuffer::createDOMString(m_resultInText);
    } else if (m_resultInArrayBuffer) {
        // read as ArrayBuffer
        return DOMStringOrArrayBuffer::createArrayBuffer(m_resultInArrayBuffer);
    }
    return DOMStringOrArrayBuffer::createDOMString(String::emptyString);
}

} // namespace Starfish
