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

/**
 * Manages settings state for a SemWebView.
 */
public class SemWebSettings {
    /**
     * Default cache usage mode.
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


    private int mCacheMode = LOAD_DEFAULT;
    private String mDefaultUserAgent = null;
    private String mUserAgentString = null;

    SemWebSettings(String dua, String ua, int cacheMode) {
        mDefaultUserAgent = dua;
        mUserAgentString = ua;
        setCacheMode(cacheMode);
    }

    /**
     * Returns the default User-Agent used by a WebView. An instance of WebView could use a
     * different User-Agent if a call is made to setUserAgentString(String).
     *
     * @return A Context object used to access application assets
     */
    public String getDefaultUserAgent() {
        return mDefaultUserAgent;
    }

    /**
     * Gets the WebView's user-agent string.
     *
     * @return The WebView's user-agent string
     */
    public String getUserAgentString() {
        return mUserAgentString;
    }

    /**
     * Gets the current setting for overriding the cache mode.
     *
     * @return The current setting for overriding the cache mode
     * Value is LOAD_DEFAULT, or LOAD_NO_CACHE.
     */
    public int getCacheMode() {
        return mCacheMode;
    }

    /**
     * Sets the WebView's user-agent string. If the string is null or empty,
     * the system default value will be used.
     *
     * @param ua New user-agent string. This value may be null.
     */
    public void setUserAgentString(String ua) {
        mUserAgentString = ua;
    }

    /**
     * Overrides the way the cache is used.
     *
     * @param mode The mode to use. Value is LOAD_DEFAULT, or LOAD_NO_CACHE.
     */
    public void setCacheMode(int mode) {
        if (mode == LOAD_DEFAULT || mode == LOAD_NO_CACHE) {
            mCacheMode = mode;
        }
    }

    /*
    void setAllowUniveralAceessFromFilesURLs(boolean allowUniveralAceessFromFilesURLs){
    }
    void setJavaScriptEnable(boolean javaScriptEnable){
    }
    */

}
