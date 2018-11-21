/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

package com.samsung.android.lwe;

/**
 * This interface is used to implement a download listener for a SemWebView
 */
public interface SemDownloadListener {
    /**
     * Notify the host application that a file should be downloaded.
     *
     * @param url The full url to the content that should be downloaded
     * @param userAgent The user agent to be used for the download
     * @param contentDisposition Content-disposition http header, if present
     * @param mimetype The mimetype of the content reported by the server
     * @param contentLength The file size reported by the server
     * @since Lightweight Web Engine 1.0
     */
    void onDownloadStart(String url, String userAgent, String contentDisposition, String mimetype,
                         long contentLength);
}
