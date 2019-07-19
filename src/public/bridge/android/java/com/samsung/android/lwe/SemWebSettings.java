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

import android.content.Context;
import android.webkit.WebView;

/**
 * This class manages settings state for a SemWebView.
 *
 * @deprecated This class was deprecated in API level 29. This class will be removed in a future Android release, and will not be supported anymore. Do not use this class.
 */
public class SemWebSettings {
    /**
     * Default cache usage mode
     *
     * @since Lightweight Web Engine 1.0
     */
    public static final int LOAD_DEFAULT = -1;
    // static final int LOAD_NORMAL = 0;

    /**
     * Use cached resources when they are available, even if they have expired.
     *
     * @since Lightweight Web Engine 1.0
     */
    public static final int LOAD_CACHE_ELSE_NETWORK = 1;

    /**
     * Don't use the cache, load from the network.
     *
     * @since Lightweight Web Engine 1.0
     */
    public static final int LOAD_NO_CACHE = 2;

    /**
     * Don't use the network, load from the cache.
     *
     * @since Lightweight Web Engine 1.0
     */
    public static final int LOAD_CACHE_ONLY = 3;

    private LweWebView mLWEWebView = null;
    private WebView mAndroidWebView = null;

    /**
     * Creates a SemWebSettings object
     *
     * @hide Internal use only
     * @param webView LweWebView
     * @since Lightweight Web Engine 1.0
     */
    public SemWebSettings(LweWebView webView) {
        mLWEWebView = webView;
    }

    /**
     * Creates a SemWebSettings object
     *
     * @hide Internal use only
     * @param webView WebView
     * @since Lightweight Web Engine 1.0
     */
    public SemWebSettings(WebView webView) {
        mAndroidWebView = webView;
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
        if (mLWEWebView != null) {
            return mLWEWebView.getDefaultUserAgent(context);
        } else if (mAndroidWebView != null) {
            return mAndroidWebView.getSettings().getDefaultUserAgent(context);
        }
        return null;
    }

    /**
     * Gets the WebView's user-agent string.
     *
     * @return The WebView's user-agent string
     * @since Lightweight Web Engine 1.0
     */
    public String getUserAgentString() {
        if (mLWEWebView != null) {
            return mLWEWebView.getUserAgentString();
        } else if (mAndroidWebView != null) {
            return mAndroidWebView.getSettings().getUserAgentString();
        }
        return null;
    }

    /**
     * Gets the current setting for overriding the cache mode.
     *
     * @return The current setting for overriding the cache mode
     * Value is LOAD_DEFAULT, or LOAD_NO_CACHE.
     * @since Lightweight Web Engine 1.0
     */
    public int getCacheMode() {
        if (mLWEWebView != null) {
            return mLWEWebView.getCacheMode();
        } else if (mAndroidWebView != null) {
            return mAndroidWebView.getSettings().getCacheMode();
        }
        return -1;
    }

    /**
     * Sets the WebView's user-agent string. If the string is null or empty,
     * the system default value will be used.
     *
     * @param ua New user-agent string. This value may be null.
     * @since Lightweight Web Engine 1.0
     */
    public void setUserAgentString(String ua) {
        if (mLWEWebView != null) {
            mLWEWebView.setUserAgentString(ua);
        } else if (mAndroidWebView != null) {
            mAndroidWebView.getSettings().setUserAgentString(ua);
        }
    }

    /**
     * Overrides the way the cache is used.
     *
     * @param mode The mode to use. Value is LOAD_DEFAULT, or LOAD_NO_CACHE.
     * @since Lightweight Web Engine 1.0
     */
    public void setCacheMode(int mode) {
        if (mLWEWebView != null) {
            mLWEWebView.setCacheMode(mode);
        } else if (mAndroidWebView != null) {
            mAndroidWebView.getSettings().setCacheMode(mode);
        }
    }

    /**
     * Gets the WebView's default font size.
     *
     * @return A non-negative integer between 1 and 72.
     * @since Lightweight Web Engine 1.0
     */
    public int getDefaultFontSize() {
        if (mLWEWebView != null) {
            return mLWEWebView.getDefaultFontSize();
        } else if (mAndroidWebView != null) {
            mAndroidWebView.getSettings().getDefaultFontSize();
        }
        return -1;
    }

    /**
     * Sets the WebView's default font size.
     * @param size A non-negative integer between 1 and 72. Any number outside the specified range will be pinned.
     * @since Lightweight Web Engine 1.0
     */
    public void setDefaultFontSize(int size) {
        if (mLWEWebView != null) {
            mLWEWebView.setDefaultFontSize(size);
        } else if (mAndroidWebView != null) {
            mAndroidWebView.getSettings().setDefaultFontSize(size);
        }
    }
}
