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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/ExecutionContext.h"
#include "core/fileapi/Blob.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/util/URL.h"
#include "core/util/URLSearchParams.h"
#include "core/page/WebBase.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"

namespace Starfish {

URL::URL(ExecutionContext* executionContext, String* url)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_resourceURL(new ResourceURL(url))
{
}

URL::URL(ExecutionContext* executionContext, String* url, String* baseURL)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_resourceURL(new ResourceURL(url, baseURL))
{
}

ScriptBindingInstance* URL::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* URL::createObjectURL(Blob* blob)
{
    BlobURLStore store;
    WebBase* webBase = blob->executionContext()->webBase();
    if (webBase->isValidBlobURL(blob)) {
        auto maybeBlobURLStore = webBase->findBlobURL(blob);
        if (maybeBlobURLStore.hasValue()) {
            store = maybeBlobURLStore.value();
        } else {
            return String::emptyString;
        }
    } else {
        store = webBase->addBlobInBlobURLStore(blob);
    }
    return WebBase::blobURLStoreToString(store,
                                         blob->executionContext()->urlString());
}

void URL::revokeObjectURL(ExecutionContext* executionContext,
                          String* blobURLRef)
{
    BlobURLStore store;
    if (WebBase::stringToBlobURLString(blobURLRef, store)) {
        WebBase* webBase = executionContext->webBase();
        if (webBase->isValidBlobURL(store)) {
            webBase->removeBlobFromBlobURLStore((Blob*)store.m_blob);
        }
#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(STARFISH_WEBWORKER_HOST)
        else {
            WebView* webView = webBase->asWebView();
            if (webView->isValidMediaSourceBlobURL(store)) {
                webView->removeMediaSourceFromBlobURLStore(
                    (MediaSource*)store.m_blob);
            }
        }
#endif
    }
}

#if defined(STARFISH_ENABLE_MULTIMEDIA)
String* URL::createObjectURL(MediaSource* mediaSource)
{
    BlobURLStore store;
    if (mediaSource->webView()->isValidMediaSourceBlobURL(mediaSource)) {
        auto maybeBlobURLStore =
            mediaSource->webView()->findMediaSourceBlobURL(mediaSource);
        if (maybeBlobURLStore.hasValue()) {
            store = maybeBlobURLStore.value();
        } else {
            return String::emptyString;
        }
    } else {
        store =
            mediaSource->webView()->addMediaSourceInBlobURLStore(mediaSource);
    }
    return WebBase::blobURLStoreToString(store,
                                         mediaSource->document()->urlString());
}
#endif

String* URL::origin()
{
    return m_resourceURL->origin();
}

String* URL::href()
{
    return m_resourceURL->href();
}

void URL::setHref(String* newHref)
{
    m_resourceURL = m_resourceURL->setHref(newHref);
}

String* URL::protocol()
{
    return m_resourceURL->protocol();
}

void URL::setProtocol(String* newProtocol)
{
    m_resourceURL = m_resourceURL->setProtocol(newProtocol);
}

String* URL::username()
{
    return m_resourceURL->username();
}

void URL::setUsername(String* newUsername)
{
    m_resourceURL = m_resourceURL->setUsername(newUsername);
}

String* URL::password()
{
    return m_resourceURL->password();
}

void URL::setPassword(String* newPassword)
{
    m_resourceURL = m_resourceURL->setPassword(newPassword);
}

String* URL::host()
{
    return m_resourceURL->host();
}

void URL::setHost(String* newHost)
{
    m_resourceURL = m_resourceURL->setHost(newHost);
}

String* URL::hostname()
{
    return m_resourceURL->hostname();
}

void URL::setHostname(String* newHostname)
{
    m_resourceURL = m_resourceURL->setHostname(newHostname);
}

String* URL::port()
{
    return m_resourceURL->port();
}

void URL::setPort(String* newPort)
{
    m_resourceURL = m_resourceURL->setPort(newPort);
}

String* URL::pathname()
{
    return m_resourceURL->pathname();
}

void URL::setPathname(String* newPath, bool needRemovingDots)
{
    m_resourceURL = m_resourceURL->setPathname(newPath, needRemovingDots);
}

String* URL::search()
{
    return m_resourceURL->search();
}

void URL::setSearch(String* newSearch, bool needsToUpdateSearchParams)
{
    m_resourceURL = m_resourceURL->setSearch(newSearch);
    if (needsToUpdateSearchParams && m_searchParams) {
        m_searchParams->parse(newSearch);
    }
}

String* URL::hash()
{
    return m_resourceURL->hash();
}

void URL::setHash(String* newHash)
{
    m_resourceURL = m_resourceURL->setHash(newHash);
}

URLSearchParams* URL::searchParams()
{
    if (!m_searchParams) {
        m_searchParams = new URLSearchParams(m_executionContext, this);
    }

    return m_searchParams.value();
}
} // namespace Starfish
