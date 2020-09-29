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

package com.samsung.android.lwe.internal;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.inputmethod.InputConnection;
import android.webkit.ValueCallback;

import com.samsung.android.lwe.SemLweDownloadListener;
import com.samsung.android.lwe.SemLweWebSettings;
import com.samsung.android.lwe.SemLweWebViewClient;
import com.samsung.android.lwe.SemLweWebLweClient;

public interface LweWebView {
    void initWebView(View appView, AttributeSet attrs);
    String getDefaultUserAgent(Context context);
    String getUserAgentString();
    void setUserAgentString(String userAgent);
    int getCacheMode();
    void setCacheMode(int mode);
    int getDefaultFontSize();
    void setDefaultFontSize(int size);
    void setWebSecurityEnable(boolean enabled);
    boolean getWebSecurityEnable();
    InputConnection getInputConnectionInstance(View view);
    void onVisibilityChanged(View changedView, int visibility);

    void loadUrl(final String url);
    String getUrl();
    void loadData(String data, String mimeType, String encoding);
    void reload();
    void stopLoading();
    void goBack();
    void goForward();
    boolean canGoBack();
    boolean canGoForward();
    void addJavascriptInterface(final Object object, final String name);
    void removeJavascriptInterface(final String name);
    void clearCache(boolean includeDiskFiles);
    void evaluateJavascript(final String script,
                            final ValueCallback<String> resultCallback);
    void clearHistory();
    SemLweWebSettings getSettings();
    void setWebViewClient(SemLweWebViewClient client);
    void setDownloadListener(SemLweDownloadListener listener);
    void setWebLweClient(SemLweWebLweClient client);
}