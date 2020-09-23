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

import com.samsung.android.lwe.internal.LweWebView;

public class SemLweWebSettings {
    public static final int LOAD_DEFAULT = -1;
    // static final int LOAD_NORMAL = 0;
    public static final int LOAD_CACHE_ELSE_NETWORK = 1;
    public static final int LOAD_NO_CACHE = 2;
    public static final int LOAD_CACHE_ONLY = 3;

    private LweWebView mLWEWebView = null;

    public SemLweWebSettings(LweWebView webView) {
        mLWEWebView = webView;
    }
    public String getDefaultUserAgent(Context context) {
        return mLWEWebView.getDefaultUserAgent(context);
    }
    public String getUserAgentString() {
        return mLWEWebView.getUserAgentString();
    }
    public int getCacheMode() {
        return mLWEWebView.getCacheMode();
    }
    public void setUserAgentString(String ua) {
        mLWEWebView.setUserAgentString(ua);
    }
    public void setCacheMode(int mode) {
        mLWEWebView.setCacheMode(mode);
    }
    public int getDefaultFontSize() {
        return mLWEWebView.getDefaultFontSize();
    }
    public void setDefaultFontSize(int size) {
        mLWEWebView.setDefaultFontSize(size);
    }
}
