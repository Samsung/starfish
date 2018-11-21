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

package com.samsung.android.lwe;

import android.content.Context;
import android.view.View;
import android.view.inputmethod.InputConnection;
import android.webkit.ValueCallback;

/**
 * @hide
 */
public interface LweWebView {
    void initWebView(View appView);
    String getDefaultUserAgent(Context context);
    String getUserAgentString();
    void setUserAgentString(String userAgent);
    int getCacheMode();
    void setCacheMode(int mode);
    int getDefaultFontSize();
    void setDefaultFontSize(int size);

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

    SemWebSettings getSettings();
    void setWebViewClient(SemWebViewClient client);
    void setDownloadListener(SemDownloadListener listener);
    void setWebLweClient(SemWebLweClient client);
}
