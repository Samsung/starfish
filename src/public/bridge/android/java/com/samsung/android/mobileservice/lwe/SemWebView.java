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
import android.content.pm.PackageManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.TextureView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.webkit.ValueCallback;
import dalvik.system.PathClassLoader;
import java.lang.reflect.Constructor;

/**
 * This class is a view that displays Web pages.
 */
public class SemWebView extends TextureView {

    private static PathClassLoader pcl = null;
    private static final String packageName = "com.samsung.android.mobileservice.lwe";
    private static final String LweWebViewImplName = "com.samsung.android.mobileservice.lwe.LweWebViewImpl";

    /**
     * @hide
     */
    protected static final String sTag = "SemWebView";

    private LweWebView delegate = null;

    /**
     * Creates a new InputConnection for an InputMethod to interact with the WebView.
     *
     * @param outAttrs Fill in with attribute information about the connection.
     * @return InputConnection
     * @since Lightweight Web Engine 1.0
     */
    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (delegate != null)
            return delegate.getInputConnectionInstance(this);
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
        if (delegate != null) {
            delegate.onVisibilityChanged(changedView, visibility);
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

    private LweWebView getWebViewInstance() {
        if (delegate == null) {
            LweWebView result = null;
            try {
                if (pcl == null) {
                    String path = getContext().getPackageManager().getPackageInfo(packageName,0).applicationInfo.nativeLibraryDir;
                    String dexpath = getContext().getPackageManager().getPackageInfo(packageName,0).applicationInfo.publicSourceDir;
                    pcl = new PathClassLoader(dexpath,path,getContext().getClassLoader());
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
            /*
            finally {
                // Testing only: uncomment the following code to test locally
                // installed LweWebView.
                if (result == null) {
                    result = new LweWebViewImpl();
                }
            }
            */
            return result;
        }

        return delegate;
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

        delegate = getWebViewInstance();
        if (delegate != null)
            delegate.initWebView(this);
    }

    /**
     * Loads the given URL.
     *
     * @param url The URL of the resource to load
     * @since Lightweight Web Engine 1.0
     */
    public void loadUrl(String url) {
        if (delegate != null) {
            delegate.loadUrl(url);
        }
    }

    /**
     * Gets the URL for the current page.
     *
     * @return The URL for the current page
     * @since Lightweight Web Engine 1.0
     */
    public String getUrl() {
        if (delegate != null)
            return delegate.getUrl();
        return null;
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
        if (delegate != null) {
            if (mimeType == null) {
                mimeType = "text/html";
            }
            if (encoding == null) {
                encoding = "UTF-8";
            }

            delegate.loadData(data, mimeType, encoding);
        }
    }

    /**
     * Reloads the current URL.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void reload() {
        if (delegate != null)
            delegate.reload();
    }

    /**
     * Stops the current load.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void stopLoading() {
        if (delegate != null)
            delegate.stopLoading();
    }

    /**
     * Goes back in the history of this WebView.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void goBack() {
        if (delegate != null)
            delegate.goBack();
    }

    /**
     * Goes forward in the history of this WebView.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void goForward() {
        if (delegate != null)
            delegate.goForward();
    }

    /**
     * Gets whether this WebView has a back history item.
     *
     * @return {@code true} if this WebView has a back history item, <br>
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean canGoBack() {
        if (delegate != null)
            return delegate.canGoBack();
        return false;
    }

    /**
     * Gets whether this WebView has a forward history item.
     *
     * @return {@code true} if this WebView has a forward history item,
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean canGoForward() {
        if (delegate != null)
            return delegate.canGoForward();
        return false;
    }

    /**
     * Injects the supplied Java object into this WebView.
     *
     * @param object The Java object to inject into this WebView's JavaScript context.
     *               null values are ignored.
     * @param name The name used to expose the object in JavaScript
     * @since Lightweight Web Engine 1.0
     */
    public void addJavascriptInterface(Object object, String name) {
        if (delegate != null)
            delegate.addJavascriptInterface(object, name);
    }

    /**
     * Removes a previously injected Java object from this WebView.
     *
     * @param name The name used to expose the object in JavaScript. This value must never be null.
     * @since Lightweight Web Engine 1.0
     */
    public void removeJavascriptInterface(String name) {
        if (delegate != null)
            delegate.removeJavascriptInterface(name);
    }

    /**
     * Clears the resource cache.
     *
     * @param includeDiskFiles if {@code false}, only the RAM cache is cleared.
     * @since Lightweight Web Engine 1.0
     */
    public void clearCache(boolean includeDiskFiles) {
        if (delegate != null) {
            delegate.clearCache(includeDiskFiles);
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
        if (delegate != null)
            delegate.evaluateJavascript(script, resultCallback);
    }

    /**
     * Tells this WebView to clear its internal back/forward list.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void clearHistory() {
        if (delegate != null)
            delegate.clearHistory();
    }

    /**
     * Gets the Settings object used to control the settings for this WebView.
     *
     * @return A Settings object that can be used to control this WebView's settings
     * @since Lightweight Web Engine 1.0
     */
    public SemWebSettings getSettings() {
        if (delegate != null) {
            return delegate.getSettings();
        }

        return null;
    }

    /**
     * Sets the WebViewClient that will receive various notifications and requests.
     * This will replace the current handler.
     *
     * @param client An implementation of SemWebViewClient
     * @since Lightweight Web Engine 1.0
     */
    public void setWebViewClient(SemWebViewClient client) {
        if (delegate != null)
            delegate.setWebViewClient(client);
    }

    /**
     * Sets the lwe handler.
     *
     * @param client An implementation of SemWebLweClient
     * @since Lightweight Web Engine 1.0
     */
    public void setWebLweClient(SemWebLweClient client) {
        if (delegate != null) {
            delegate.setWebLweClient(client);
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
        if (delegate != null)
            delegate.setDownloadListener(listener);
    }
}
