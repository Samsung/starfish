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

/**
 * This class is a Web view client for a SemWebView.
 */
public class SemWebViewClient {
    /**
     * Constructs a new SemWebViewClient
     *
     * @since Lightweight Web Engine 1.0
     */
    public SemWebViewClient() {
    }

    /**
     * Notify the host application that the WebView will load the resource specified by the given
     * url.
     *
     * @param view The WebView that is initiating the callback.
     * @param url The url of the resource the WebView will load.
     * @since Lightweight Web Engine 1.0
     */
    public void onLoadResource(SemWebView view, String url) {
    }

    /**
     * Report an error to the host application.
     *
     * @param view The WebView that is initiating the callback.
     * @param error Information about the error occurred
     * @since Lightweight Web Engine 1.0
     */
    public void onReceivedError(SemWebView view, SemWebResourceError error) {
    }

    /**
     * Notify the host application that a page has finished loading.
     * When onPageFinished() is called, the rendering picture may not be updated yet.
     *
     * @param view The WebView that is initiating the callback.
     * @param url The url of the page
     * @since Lightweight Web Engine 1.0
     */
    public void onPageFinished(SemWebView view, String url) {
    }

    /**
     * Notify the host application that a page has started loading.
     *
     * @param view The WebView that is initiating the callback.
     * @param url The url to be loaded
     * @since Lightweight Web Engine 1.0
     */
    public void onPageStarted(SemWebView view, String url) {
    }

    /**
     * Give the host application a chance to take control when a URL is about to be loaded in the
     * current WebView. If a WebViewClient is not provided, by default WebView will continue to load
     * the URL. If a WebViewClient is provided, returning true causes the current WebView to abort
     * loading the URL, while returning false causes the WebView to continue loading the URL as
     * usual.
     *
     * @param view The WebView that is initiating the callback.
     * @param request Object containing the details of the request
     * @return {@code true} to cancel the current load, <br>
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean shouldOverrideUrlLoading(SemWebView view, String request) {
        return false;
    }
}
