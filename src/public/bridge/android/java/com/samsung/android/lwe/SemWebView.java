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

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.view.View;
import android.webkit.DownloadListener;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.AbsoluteLayout;

import dalvik.system.PathClassLoader;

/**
 * This class is a view that displays Web pages.
 */
public class SemWebView extends AbsoluteLayout {

    private static PathClassLoader pcl = null;
    /**
     * @hide
     */
    public static final String PACKAGE_NAME = "com.samsung.android.lwe";
    private static final String LweWebViewImplName = "com.samsung.android.lwe.LweWebViewImpl";
    private static boolean USE_LWE = false;

    /**
     * @hide
     */
    protected static final String sTag = "SemWebView";

    private LweWebView mLWEWebView = null;
    private WebView mAndroidWebView = null;

    private boolean canUseLWE() {
        if (USE_LWE && mLWEWebView != null) {
            return true;
        } else if (mAndroidWebView == null) {
            throw new AssertionError("Both LWE and WebView are not available.");
        }
        return false;
    }

    private boolean checkLWEInstallation() {
        /*
        try{
            getContext().getPackageManager().getPackageInfo(PACKAGE_NAME, 0);
            USE_LWE = true;
        }catch (Exception e){}
        */
        USE_LWE = true;
        return USE_LWE;
    }

    private LweWebView getLWEWebViewInstance(Context context, AttributeSet attrs, int defStyle) {
        if (mLWEWebView == null) {
            LweWebView result = null;
            // Should uncomment following code for downloadable mode.
            /*
            try {
                if (pcl == null) {
                    String path = getContext().getPackageManager().getPackageInfo(PACKAGE_NAME, 0).applicationInfo.nativeLibraryDir;
                    String dexpath = getContext().getPackageManager().getPackageInfo(PACKAGE_NAME, 0).applicationInfo.publicSourceDir;
                    pcl = new PathClassLoader(dexpath, path, getContext().getClassLoader());
                }
                Class<?> cls = pcl.loadClass(LweWebViewImplName);
                Constructor<?> cons = cls.getConstructor();
                result = (LweWebView)cons.newInstance();
            } catch (PackageManager.NameNotFoundException e) {
                Log.e(sTag, "apk is not installed");
                e.printStackTrace();
            } catch (Exception e) {
                Log.e(sTag, "apk cannot be loaded");
                e.printStackTrace();
            }
            */
            if (result == null) {
                result = new LweWebViewImpl(context, attrs, defStyle);
            }
            return result;
        }

        return mLWEWebView;
    }

    /**
     * Constructs a new SemWebView with an Activity Context object.
     *
     * @param context An Activity Context to access application assets
     * @since Lightweight Web Engine 1.0
     */
    public SemWebView(Context context) {
        this(context, null);
    }

    /**
     * Constructs a new SemWebView with layout parameters.
     *
     * @param context An Activity Context to access application assets
     * @param attrs An AttributeSet passed to our parent
     * @since Lightweight Web Engine 1.0
     */
    public SemWebView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    /**
     * Constructs a new SemWebView with layout parameters and a default style.
     *
     * @param context An Activity Context to access application assets
     * @param attrs An AttributeSet passed to our parent
     * @param defStyle An attribute in the current theme that contains a reference to a style
     *                 resource that supplies default values for the view. Can be 0 to not look
     *                 for defaults.
     * @since Lightweight Web Engine 1.0
     */
    public SemWebView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        if (checkLWEInstallation()) {
            mLWEWebView = getLWEWebViewInstance(context, attrs, defStyle);
            if (mLWEWebView != null)
                mLWEWebView.initWebView(this);

            addView((View)mLWEWebView);
        } else {
            mAndroidWebView = new WebView(context, attrs, defStyle);
            mAndroidWebView.getSettings().setJavaScriptEnabled(true);
            mAndroidWebView.setWebViewClient(new WebViewClient() {
                                             @Override
                                             public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                                                 return false;
                                             }});
            addView(mAndroidWebView);
        }
    }

    /**
     * Loads the given URL.
     *
     * @param url The URL of the resource to load
     * @since Lightweight Web Engine 1.0
     */
    public void loadUrl(String url) {
        if (canUseLWE()) {
            mLWEWebView.loadUrl(url);
        } else {
            mAndroidWebView.loadUrl(url);
        }
    }

    /**
     * Gets the URL for the current page.
     *
     * @return The URL for the current page
     * @since Lightweight Web Engine 1.0
     */
    public String getUrl() {
        if (canUseLWE()) {
            return mLWEWebView.getUrl();
        } else {
            return mAndroidWebView.getUrl();
        }
    }

    /**
     * Loads the given data into this WebView using a 'data' scheme URL.
     *
     * @param data A String of data in the given encoding
     * @param mimeType The MIME type of the data, e.g., 'text/html'.
     *                 This value may be null.
     * @param encoding The encoding of the data
     *                 This value may be null.
     * @since Lightweight Web Engine 1.0
     */
    public void loadData(String data, String mimeType, String encoding) {

        if (canUseLWE()) {
            if (mimeType == null) {
                mimeType = "text/html";
            }
            if (encoding == null) {
                encoding = "UTF-8";
            }
            mLWEWebView.loadData(data, mimeType, encoding);
        } else {
            mAndroidWebView.loadData(data, mimeType, encoding);
        }
    }

    /**
     * Reloads the current URL.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void reload() {
        if (canUseLWE()) {
            mLWEWebView.reload();
        } else {
            mAndroidWebView.reload();
        }
    }

    /**
     * Stops the current load.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void stopLoading() {
        if (canUseLWE()) {
            mLWEWebView.stopLoading();
        } else {
            mAndroidWebView.stopLoading();
        }
    }

    /**
     * Goes back in the history of this WebView.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void goBack() {
        if (canUseLWE()) {
            mLWEWebView.goBack();
        } else {
            mAndroidWebView.goBack();
        }
    }

    /**
     * Goes forward in the history of this WebView.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void goForward() {
        if (canUseLWE()) {
            mLWEWebView.goForward();
        } else {
            mAndroidWebView.goForward();
        }
    }

    /**
     * Gets whether this WebView has a back history item.
     *
     * @return {@code true} if this WebView has a back history item, <br>
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean canGoBack() {
        if (canUseLWE()) {
            return mLWEWebView.canGoBack();
        } else {
            return mAndroidWebView.canGoBack();
        }
    }

    /**
     * Gets whether this WebView has a forward history item.
     *
     * @return {@code true} if this WebView has a forward history item,
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean canGoForward() {
        if (canUseLWE()) {
            return mLWEWebView.canGoForward();
        } else {
            return mAndroidWebView.canGoForward();
        }
    }

    /**
     * Injects the supplied Java object into this WebView.
     *
     * @param object The Java object to inject into this WebView's JavaScript context.
     *               null values are ignored.
     * @param name The name used to expose the object in JavaScript
     * @since Lightweight Web Engine 1.0
     */
    @SuppressLint("JavascriptInterface")
    public void addJavascriptInterface(Object object, String name) {
        if (canUseLWE()) {
            mLWEWebView.addJavascriptInterface(object, name);
        } else {
            mAndroidWebView.addJavascriptInterface(object, name);
        }
    }

    /**
     * Removes a previously injected Java object from this WebView.
     *
     * @param name The name used to expose the object in JavaScript. This value must never be null.
     * @since Lightweight Web Engine 1.0
     */
    public void removeJavascriptInterface(String name) {
        if (canUseLWE()) {
            mLWEWebView.removeJavascriptInterface(name);
        } else {
            mAndroidWebView.removeJavascriptInterface(name);
        }
    }

    /**
     * Clears the resource cache.
     *
     * @param includeDiskFiles if {@code false}, only the RAM cache is cleared.
     * @since Lightweight Web Engine 1.0
     */
    public void clearCache(boolean includeDiskFiles) {
        if (canUseLWE()) {
            mLWEWebView.clearCache(includeDiskFiles);
        } else {
            mAndroidWebView.clearCache(includeDiskFiles);
        }
    }

    /**
     * Asynchronously evaluates JavaScript in the context of the currently displayed page.
     *
     * @param script The JavaScript to execute.
     * @param resultCallback A callback to be invoked when the script execution completes with the
     *                       result of the execution (if any). May be null if no notification of
     *                       the result is required.
     * @since Lightweight Web Engine 1.0
     */
    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
        if (canUseLWE()) {
            mLWEWebView.evaluateJavascript(script, resultCallback);
        } else {
            mAndroidWebView.evaluateJavascript(script, resultCallback);
        }
    }

    /**
     * Tells this WebView to clear its internal back/forward list.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void clearHistory() {
        if (canUseLWE()) {
            mLWEWebView.clearHistory();
        } else {
            mAndroidWebView.clearHistory();
        }
    }

    /**
     * Gets the Settings object used to control the settings for this WebView.
     *
     * @return A Settings object that can be used to control this WebView's settings
     * @since Lightweight Web Engine 1.0
     */
    public SemWebSettings getSettings() {
        if (canUseLWE()) {
            return mLWEWebView.getSettings();
        } else {
            return new SemWebSettings(mAndroidWebView);
        }
    }

    /**
     * Sets the WebViewClient that will receive various notifications and requests.
     * This will replace the current handler.
     *
     * @param client An implementation of SemWebViewClient
     * @since Lightweight Web Engine 1.0
     */
    public void setWebViewClient(SemWebViewClient client) {
        if (canUseLWE()) {
            mLWEWebView.setWebViewClient(client);
        } else {

            class WebViewClientWrapper extends WebViewClient {
                private SemWebView mSemWebview;
                private SemWebViewClient mSemWebViewClient;

                WebViewClientWrapper(SemWebView webview, SemWebViewClient client){
                    mSemWebview = webview;
                    mSemWebViewClient = client;
                }

                @Override
                public void onPageStarted(WebView view, String url, Bitmap favicon) {
                    mSemWebViewClient.onPageStarted(mSemWebview, url, favicon);
                }

                @Override
                public void onLoadResource(WebView view, String url) {
                    mSemWebViewClient.onLoadResource(mSemWebview, url);
                }

                @Override
                public void onPageFinished(WebView view, String url) {
                    mSemWebViewClient.onPageFinished(mSemWebview, url);
                }


                @Override
                public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                    mSemWebViewClient.onReceivedError(mSemWebview, new WebResourceRequestImpl(request.getUrl().toString()), new SemWebResourceError(error.getErrorCode(), error.getDescription()));
                }

                @Override
                public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                    return false;
                }
            }
            mAndroidWebView.setWebViewClient(new WebViewClientWrapper(this, client));
        }
    }

    /**
     * Sets the lwe handler.
     *
     * @param client An implementation of SemWebLweClient
     * @since Lightweight Web Engine 1.0
     */
    public void setWebLweClient(SemWebLweClient client) {
        if (canUseLWE()) {
            mLWEWebView.setWebLweClient(client);
        } else {
            class WebChromeClientWrapper extends WebChromeClient {
                private SemWebView mSemWebview;
                private SemWebLweClient mSemWebLweClient;
                WebChromeClientWrapper(SemWebView webview, SemWebLweClient client){
                    mSemWebview = webview;
                    mSemWebLweClient = client;
                }
                @Override
                public void onProgressChanged(WebView view, int newProgress) {
                    mSemWebLweClient.onProgressChanged(mSemWebview, newProgress);
                }
            }
            mAndroidWebView.setWebChromeClient(new WebChromeClientWrapper(this, client));
        }
    }

    /**
     * Registers the interface to be used when content can not be handled by the rendering engine,
     * and should be downloaded instead. This will replace the current handler.
     *
     * @param listener An implementation of SemDownloadListener
     * @since Lightweight Web Engine 1.0
     */
    public void setDownloadListener(SemDownloadListener listener) {
        if (canUseLWE()) {
            mLWEWebView.setDownloadListener(listener);
        } else {
            class DownloadListenerWrapper implements DownloadListener {
                private SemDownloadListener mSemDownloadListener;
                DownloadListenerWrapper(SemDownloadListener client){
                    mSemDownloadListener = client;
                }
                public void onDownloadStart(String url, String userAgent, String contentDisposition, String mimetype,
                                     long contentLength){
                    mSemDownloadListener.onDownloadStart(url, userAgent, contentDisposition, mimetype, contentLength);
                }
            }
            mAndroidWebView.setDownloadListener(new DownloadListenerWrapper(listener));
        }
    }
}
