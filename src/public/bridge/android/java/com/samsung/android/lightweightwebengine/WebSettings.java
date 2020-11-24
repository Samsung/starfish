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

import android.content.Context;

import com.samsung.android.lightweightwebengine.internal.LweWebView;

public class WebSettings {
    public static final int LOAD_DEFAULT = -1;
    // static final int LOAD_NORMAL = 0;
    public static final int LOAD_CACHE_ELSE_NETWORK = 1;
    public static final int LOAD_NO_CACHE = 2;
    public static final int LOAD_CACHE_ONLY = 3;

    private LweWebView mLWEWebView = null;

    public WebSettings(LweWebView webView) {
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

    public void setWebSecurityEnable(boolean enabled) {
        mLWEWebView.setWebSecurityEnable(enabled);
    }

    public boolean getWebSecurityEnable() {
        return mLWEWebView.getWebSecurityEnable();
    }

    public void setSupportZoom(boolean support) {
    }

    public void setMediaPlaybackRequiresUserGesture(boolean require) {
    }

    public void setBuiltInZoomControls(boolean enabled) {
    }

    public void setDisplayZoomControls(boolean enabled) {
    }

    public void setAllowFileAccess(boolean allow) {
    }

    public void setAllowContentAccess(boolean allow) {
    }

    public void setLoadWithOverviewMode(boolean overview) {
    }

    public void setSaveFormData(boolean save) {
    }

    public void setTextZoom(int textZoom) {
    }

    public int getTextZoom() {
        return 0;
    }

    public void setUseWideViewPort(boolean use) {
    }

    public void setLoadsImagesAutomatically(boolean flag) {
    }

    public void setBlockNetworkImage(boolean flag) {
    }

    public void setBlockNetworkLoads(boolean flag) {
    }

    public void setJavaScriptEnabled(boolean flag) {
    }

    public void setAllowUniversalAccessFromFileURLs(boolean flag) {
    }

    public void setAllowFileAccessFromFileURLs(boolean flag) {
    }

    public void setDefaultTextEncodingName(String encoding) {
    }

    public void setMixedContentMode(int mode) {
    }

    public void setDisabledActionModeMenuItems(int menuItems) {
    }
}
