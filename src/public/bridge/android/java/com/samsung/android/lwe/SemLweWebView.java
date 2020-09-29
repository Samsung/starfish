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

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.webkit.ValueCallback;

import com.samsung.android.lwe.internal.LweWebView;
import com.samsung.android.lwe.internal.LweWebViewImpl;

import dalvik.system.PathClassLoader;

public class SemLweWebView extends SurfaceView {
    private static PathClassLoader pcl = null;
    /**
     * @hide
     */
    public static final String PACKAGE_NAME = "com.samsung.android.lwe";

    /**
     * @hide
     */
    protected static final String sTag = "SemLweWebView";

    private LweWebView mLWEWebView = null;

    private LweWebView getLWEWebViewInstance(Context context, AttributeSet attrs, int defStyle) {
        if (mLWEWebView != null) {
            return mLWEWebView;
        }

        mLWEWebView = new LweWebViewImpl();
        return mLWEWebView;
    }

    /**
     * Creates a new InputConnection for an InputMethod to interact with the WebView.
     *
     * @param outAttrs Fill in with attribute information about the connection.
     * @return InputConnection
     * @since Lightweight Web Engine 1.0
     */
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mLWEWebView != null) {
            return mLWEWebView.getInputConnectionInstance(this);
        }
        return null;
    }

    /**
     * Called when the visibility of the view or an ancestor of the view has changed.
     *
     * @param changedView The view whose visibility changed. May be this or an ancestor view.
     * @param visibility The new visibility, one of View.VISIBLE, View.INVISIBLE or View.GONE.
     * @since Lightweight Web Engine 1.0
     */
    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        if (mLWEWebView != null) {
            mLWEWebView.onVisibilityChanged(changedView, visibility);
        }
    }

    /**
     * Called when the window containing has change its visibility (between GONE,
     * INVISIBLE, and VISIBLE). Note that this tells you whether or not your window is
     * being made visible to the window manager; this does not tell you whether or
     * not your window is obscured by other windows on the screen, even if it is
     * itself visible.
     *
     * @param visibility The new visibility of the window.
     * @since Lightweight Web Engine 1.0
     */
    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
    }

    public SemLweWebView(Context context) {
        this(context, null);
    }
    public SemLweWebView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }
    public SemLweWebView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mLWEWebView = getLWEWebViewInstance(context, attrs, defStyle);
        if (mLWEWebView != null) {
            mLWEWebView.initWebView(this, attrs);
        }
    }

    public void loadUrl(String url) {
        mLWEWebView.loadUrl(url);
    }
    public String getUrl() {
        return mLWEWebView.getUrl();
    }
    public void loadData(String data, String mimeType, String encoding) {
        if (mimeType == null) {
            mimeType = "text/html";
        }
        if (encoding == null) {
            encoding = "UTF-8";
        }
        mLWEWebView.loadData(data, mimeType, encoding);
    }
    public void reload() {
        mLWEWebView.reload();
    }
    public void stopLoading() {
        mLWEWebView.stopLoading();
    }
    public void goBack() {
        mLWEWebView.goBack();
    }
    public void goForward() {
        mLWEWebView.goForward();
    }
    public boolean canGoBack() {
        return mLWEWebView.canGoBack();
    }
    public boolean canGoForward() {
        return mLWEWebView.canGoForward();
    }
    @SuppressLint("JavascriptInterface")
    public void addJavascriptInterface(Object object, String name) {
        mLWEWebView.addJavascriptInterface(object, name);
    }
    public void removeJavascriptInterface(String name) {
        mLWEWebView.removeJavascriptInterface(name);
    }
    public void clearCache(boolean includeDiskFiles) {
        mLWEWebView.clearCache(includeDiskFiles);
    }
    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
        mLWEWebView.evaluateJavascript(script, resultCallback);
    }
    public void clearHistory() {
        mLWEWebView.clearHistory();
    }
    public SemLweWebSettings getSettings() {
        return mLWEWebView.getSettings();
    }
    public void setWebViewClient(SemLweWebViewClient client) {
        mLWEWebView.setWebViewClient(client);
    }
    public void setWebLweClient(SemLweWebLweClient client) {
        mLWEWebView.setWebLweClient(client);
    }
    public void setDownloadListener(SemLweDownloadListener listener) {
        mLWEWebView.setDownloadListener(listener);
    }
}
