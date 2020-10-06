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

package com.samsung.android.lightweightwebengine;

import android.graphics.Bitmap;

public class SemWebViewClient {
    public static final int ERROR_UNKNOWN = -1;
    public static final int ERROR_HOST_LOOKUP = -2;
    public static final int ERROR_UNSUPPORTED_AUTH_SCHEME = -3;
    public static final int ERROR_AUTHENTICATION = -4;
    public static final int ERROR_PROXY_AUTHENTICATION = -5;
    public static final int ERROR_CONNECT = -6;
    public static final int ERROR_IO = -7;
    public static final int ERROR_TIMEOUT = -8;
    public static final int ERROR_REDIRECT_LOOP = -9;
    public static final int ERROR_UNSUPPORTED_SCHEME = -10;
    public static final int ERROR_FAILED_SSL_HANDSHAKE = -11;
    public static final int ERROR_BAD_URL = -12;
    public static final int ERROR_FILE = -13;
    public static final int ERROR_FILE_NOT_FOUND = -14;
    public static final int ERROR_TOO_MANY_REQUESTS = -15;

    public SemWebViewClient() {
    }

    public void onLoadResource(SemWebView view, String url) {
    }

    public void onReceivedError(SemWebView view, SemWebResourceRequest request,
                                SemWebResourceError error) {
    }

    public void onPageFinished(SemWebView view, String url) {
    }

    public void onPageStarted(SemWebView view, String url, Bitmap favicon) {
    }

    public boolean shouldOverrideUrlLoading(SemWebView view, SemWebResourceRequest request) {
        return false;
    }
}
