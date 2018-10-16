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

import android.content.Context;

/**
 * This class manages settings state for a SemWebView.
 */
public class SemWebSettings {
    /**
     * Default cache usage mode
     */
    public static final int LOAD_DEFAULT = -1;
    // static final int LOAD_NORMAL = 0;

    /**
     * Use cached resources when they are available, even if they have expired.
     */
    public static final int LOAD_CACHE_ELSE_NETWORK = 1;

    /**
     * Don't use the cache, load from the network.
     */
    public static final int LOAD_NO_CACHE = 2;

    /**
     * Don't use the network, load from the cache.
     */
    public static final int LOAD_CACHE_ONLY = 3;

    private static final int DEFAULT_FONT_SIZE = 16;
    private LweWebViewImpl mWebView = null;

    /**
     * Creates a SemWebSettings object
     *
     * @hide Internal use only
     * @param webView LweWebViewImpl
     * @since Lightweight Web Engine 1.0
     */
    SemWebSettings(LweWebViewImpl webView) {
        mWebView = webView;
    }

    /**
     * Returns the default User-Agent used by a WebView. An instance of WebView could use a
     * different User-Agent if a call is made to setUserAgentString(String).
     *
     * @param context: A Context object used to access application assets
     * @return The WebView's default user-agent string
     * @since Lightweight Web Engine 1.0
     */
    public String getDefaultUserAgent(Context context) {
        if (mWebView.getWebViewInternalHandle() != 0) {
            mWebView.getDefaultUserAgent(context);
        }
        return "";
    }

    /**
     * Gets the WebView's user-agent string.
     *
     * @return The WebView's user-agent string
     * @since Lightweight Web Engine 1.0
     */
    public String getUserAgentString() {
        if (mWebView.getWebViewInternalHandle() != 0) {
            mWebView.getUserAgentString();
        }
        return "";
    }

    /**
     * Gets the current setting for overriding the cache mode.
     *
     * @return The current setting for overriding the cache mode
     * Value is LOAD_DEFAULT, or LOAD_NO_CACHE.
     * @since Lightweight Web Engine 1.0
     */
    public int getCacheMode() {
        if (mWebView.getWebViewInternalHandle() != 0) {
            mWebView.getCacheMode();
        }
        return LOAD_DEFAULT;
    }

    /**
     * Sets the WebView's user-agent string. If the string is null or empty,
     * the system default value will be used.
     *
     * @param ua New user-agent string. This value may be null.
     * @since Lightweight Web Engine 1.0
     */
    public void setUserAgentString(String ua) {
        if (mWebView.getWebViewInternalHandle() != 0) {
            mWebView.setUserAgentString(mWebView.getWebViewInternalHandle(), ua);
        }
    }

    /**
     * Overrides the way the cache is used.
     *
     * @param mode The mode to use. Value is LOAD_DEFAULT, or LOAD_NO_CACHE.
     * @since Lightweight Web Engine 1.0
     */
    public void setCacheMode(int mode) {
        if (mode == LOAD_DEFAULT || mode == LOAD_NO_CACHE) {
            if (mWebView.getWebViewInternalHandle() != 0) {
                mWebView.setCacheMode(mWebView.getWebViewInternalHandle(), mode);
            }
        }
    }

    /**
     * Gets the WebView's default font size.
     *
     * @return A non-negative integer between 1 and 72.
     * @since Lightweight Web Engine 1.0
     */
    public int getDefaultFontSize() {
        if (mWebView.getWebViewInternalHandle() != 0) {
            return mWebView.getDefaultFontSize();
        }
        return DEFAULT_FONT_SIZE;
    }

    /**
     * Sets the WebView's default font size.
     * @param size A non-negative integer between 1 and 72. Any number outside the specified range will be pinned.
     * @since Lightweight Web Engine 1.0
     */
    public void setDefaultFontSize(int size) {
        if (1 <= size && size <= 72) {
            if (mWebView.getWebViewInternalHandle() != 0) {
                mWebView.setDefaultFontSize(mWebView.getWebViewInternalHandle(), size);
            }
        }
    }
}
