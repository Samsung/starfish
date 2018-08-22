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

package com.samsung.android.mobileservice.lwe;

import android.net.Uri;
import android.webkit.ValueCallback;

public class SemWebViewClient {
    public void onLoadResource(SemWebView view, String url) {
    }

    public void onReceivedError(SemWebView view, SemResourceError error) {
    }

    public void onPageFinished(SemWebView view, String url) {
    }

    public void onPageStarted(SemWebView view, String url) {
    }

    public void onProgressChanged(SemWebView view, int newProgress) {
    }

    public boolean shouldOverrideUrlLoading(SemWebView view, String request) {
        return false;
    }

    public boolean onShowFileChooser(SemWebView webView, ValueCallback<Uri[]> uris) {
        return false;
    }
}
