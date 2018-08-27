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
import android.view.View;
import android.view.inputmethod.InputConnection;
import android.webkit.DownloadListener;
import android.webkit.ValueCallback;

interface LweWebView {
    void initWebView(View appView, Context AppContext);
    String getDefaultUA();
    String getUA();
    int getCacheModeValue();
    InputConnection getInputConnectionInstance(View view);

    void loadUrl(final String url);
    String getUrl();
    void loadData(String data);
    void reload();
    void stopLoading();
    void goBack();
    void goForward();
    boolean canGoBack();
    boolean canGoForward();
    void addJavascriptInterface(final Object object, final String name);
    void removeJavascriptInterface(final String name);
    void clearCache();
    void evaluateJavascript(final String script,
                            final ValueCallback<String> resultCallback);
    void clearHistory();

    void setSettings(SemWebSettings settings);
    void setWebViewClient(SemWebViewClient client);
    void setDownloadListener(DownloadListener listener);
}