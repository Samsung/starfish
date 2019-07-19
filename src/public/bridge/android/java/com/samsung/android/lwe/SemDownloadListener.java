/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

package com.samsung.android.lwe;

/**
 * This interface is used to implement a download listener for a SemWebView
 *
 * @deprecated This interface was deprecated in API level 29. This interface will be removed in a future Android release, and will not be supported anymore. Do not use this interface.
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
