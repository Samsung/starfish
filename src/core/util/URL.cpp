/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/fileapi/Blob.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/page/BrowsingContext.h"
#include "core/util/URL.h"

#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

URL::URL(Window* window, String* url)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(window->scriptBindingInstance())
    , m_resourceURL(new ResourceURL(url))
{
}

URL::URL(Window* window, String* url, String* baseURL)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(window->scriptBindingInstance())
    , m_resourceURL(new ResourceURL(url, baseURL))
{
}

URL::URL(ScriptBindingInstance* ins, String* url, String* baseURL)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(ins)
    , m_resourceURL(new ResourceURL(url, baseURL))
{
}

ScriptBindingInstance* URL::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

String* URL::createObjectURL(Blob* blob)
{
    BlobURLStore store;
    if (blob->webView()->isValidBlobURL(blob)) {
        store = blob->webView()->findBlobURL(blob);
    } else {
        store = blob->webView()->addBlobInBlobURLStore(blob);
    }
    return WebView::blobURLStoreToString(store, blob->scriptBindingInstance()
                                                    ->ownerDocument()
                                                    ->documentURI()
                                                    ->urlString());
}

void URL::revokeObjectURL(Document* document, String* blobURLRef)
{
    BlobURLStore store;
    if (WebView::stringToBlobURLString(blobURLRef, store)) {
        if (document->webView()->isValidBlobURL(store)) {
            document->webView()->removeBlobFromBlobURLStore(
                (Blob*)store.m_blob);
        }
#ifdef STARFISH_ENABLE_MULTIMEDIA
        else if (document->webView()->isValidMediaSourceBlobURL(store)) {
            document->webView()->removeMediaSourceFromBlobURLStore(
                (MediaSource*)store.m_blob);
        }
#endif
    }
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
String* URL::createObjectURL(MediaSource* mediaSource)
{
    BlobURLStore store;
    if (mediaSource->webView()->isValidMediaSourceBlobURL(mediaSource)) {
        store = mediaSource->webView()->findMediaSourceBlobURL(mediaSource);
    } else {
        store =
            mediaSource->webView()->addMediaSourceInBlobURLStore(mediaSource);
    }
    return WebView::blobURLStoreToString(store,
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

void URL::setSearch(String* newSearch)
{
    m_resourceURL = m_resourceURL->setSearch(newSearch);
}

String* URL::hash()
{
    return m_resourceURL->hash();
}

void URL::setHash(String* newHash)
{
    m_resourceURL = m_resourceURL->setHash(newHash);
}
}
