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
import android.util.AttributeSet;
import android.view.SurfaceView;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.webkit.DownloadListener;
import android.webkit.ValueCallback;

public class SemWebView extends SurfaceView {

    LweWebView delegate = null;
    /**
     * Creates a new InputConnection for an InputMethod to interact with the WebView.
     *
     * @param outAttrs Fill in with attribute information about the connection.
     * @return InputConnection
     */
    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if(delegate!=null)
            return delegate.getInputConnectionInstance(this);
        return null;
    }

    private LweWebView getWebViewInstance(){
        if(delegate==null)
            return new LweWebViewImpl();
        return delegate;
    }

    public SemWebView(Context context) {
        super(context);
        delegate = getWebViewInstance();
        delegate.initWebView(this,getContext());
    }

    public SemWebView(Context context, AttributeSet attrs) {
        super(context, attrs);
        delegate = getWebViewInstance();
        delegate.initWebView(this,getContext());
    }

    public SemWebView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs);
        delegate = getWebViewInstance();
        delegate.initWebView(this,getContext());
    }

    /**
     * Loads the given URL.
     *
     * @param url the URL of the resource to load
     */
    public void loadUrl(final String url) {
        if(delegate!=null)
            delegate.loadUrl(url);
    }

    /**
     * Gets the URL for the current page.
     *
     * @return the URL for the current page
     */
    public String getUrl() {
        if(delegate!=null)
            delegate.getUrl();
        return null;
    }

    /**
     * Loads the given data into this WebView using a 'data' scheme URL.
     *
     * @param data a String of data in the given encoding
     */
    public void loadData(String data) {
        if(delegate!=null)
            delegate.loadData(data);
    }

    /**
     * Reloads the current URL.
     *
     */
    public void reload() {
        if(delegate!=null)
            delegate.reload();
    }

    /**
     * Stops the current load.
     *
     */
    public void stopLoading() {
        if(delegate!=null)
            delegate.stopLoading();
    }

    /**
     * Goes back in the history of this WebView.
     *
     */
    public void goBack() {
        if(delegate!=null)
            delegate.goBack();
    }

    /**
     * Goes forward in the history of this WebView.
     *
     */
    public void goForward() {
        if(delegate!=null)
            delegate.goForward();
    }

    /**
     * Gets whether this WebView has a back history item.
     *
     * @return true if this WebView has a back history item
     */
    public boolean canGoBack() {
        if(delegate!=null)
            return delegate.canGoBack();
        return false;
    }

    /**
     * Gets whether this WebView has a forward history item.
     *
     * @return true if this WebView has a forward history item
     */
    public boolean canGoForward() {
        if(delegate!=null)
            return delegate.canGoForward();
        return false;
    }

    /**
     * Injects the supplied Java object into this WebView.
     *
     * @param object the Java object to inject into this WebView's JavaScript context.
     *               null values are ignored.
     * @param name the name used to expose the object in JavaScript
     *
     */
    public void addJavascriptInterface(final Object object, final String name) {
        if(delegate!=null)
            delegate.addJavascriptInterface(object,name);
    }

    /**
     * Removes a previously injected Java object from this WebView.
     *
     * @param name the name used to expose the object in JavaScript. This value must never be null.
     */
    public void removeJavascriptInterface(final String name) {
        if(delegate!=null)
            delegate.removeJavascriptInterface(name);
    }

    /**
     * Clears the resource cache.
     *
     */
    public void clearCache() {
        if(delegate!=null)
            delegate.clearCache();
    }

    /**
     * Asynchronously evaluates JavaScript in the context of the currently displayed page.
     *
     * @param script
     * @param resultCallback
     */
    public void evaluateJavascript(final String script,
                                   final ValueCallback<String> resultCallback) {
        if(delegate!=null)
            delegate.evaluateJavascript(script,resultCallback);
    }

    /**
     * Tells this WebView to clear its internal back/forward list.
     *
     */
    public void clearHistory() {
        if(delegate!=null)
            delegate.clearHistory();
    }

    /**
     * Gets the Settings object used to control the settings for this WebView.
     *
     * @return a Settings object that can be used to control this WebView's settings
     */
    public SemWebSettings getSettings() {
        if(delegate!=null)
            return new SemWebSettings(delegate.getDefaultUA(), delegate.getUA(), delegate.getCacheModeValue());
        return null;
    }

    /**
     * Set a settings
     *
     * @param settings a Settings object that is used to control this WebView's settings
     */
    public void setSettings(SemWebSettings settings) {
        if(delegate!=null)
            delegate.setSettings(settings);
    }

    /**
     * Sets the WebViewClient that will receive various notifications and requests.
     * This will replace the current handler.
     *
     * @param client an implementation of WebViewClient
     */
    public void setWebViewClient(SemWebViewClient client) {
        if(delegate!=null)
            delegate.setWebViewClient(client);
    }

    /**
     * Registers the interface to be used when content can not be handled by the rendering engine,
     * and should be downloaded instead. This will replace the current handler.
     *
     * @param listener an implementation of DownloadListener
     */
    public void setDownloadListener(DownloadListener listener) {
        if(delegate!=null)
            delegate.setDownloadListener(listener);
    }

}
