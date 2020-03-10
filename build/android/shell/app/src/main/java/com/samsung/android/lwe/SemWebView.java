/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
import android.content.pm.PackageManager;
import android.util.AttributeSet;
import android.view.TextureView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.webkit.ValueCallback;

import dalvik.system.PathClassLoader;


import android.util.Log;

import java.lang.reflect.Constructor;

public class SemWebView extends TextureView {
    private static PathClassLoader pcl = null;
    public static final String PACKAGE_NAME = "com.samsung.android.lwe";
    private static final String LweWebViewImplName = "com.samsung.android.lwe.LweWebViewImpl";
    protected static final String sTag = "SemWebView";

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return null;
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
    }

    public SemWebView(Context context) {
        this(context, null);
    }
    public SemWebView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SemWebView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        LweWebView result = null;
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
    }

    public void loadUrl(String url) {
    }

    public String getUrl() {
        return null;
    }

    public void loadData(String data, String mimeType, String encoding) {
    }

    public void reload() {
    }

    public void stopLoading() {
    }

    public void goBack() {
    }

    public void goForward() {
    }

    public boolean canGoBack() {
        return false;
    }

    public boolean canGoForward() {
        return false;
    }

    public void addJavascriptInterface(Object object, String name) {
    }

    public void removeJavascriptInterface(String name) {
    }

    public void clearCache(boolean includeDiskFiles) {
    }

    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
    }

    public void clearHistory() {
    }

    public SemWebSettings getSettings() {
        return null;
    }

    public void setWebViewClient(SemWebViewClient client) {
    }

    public void setWebLweClient(SemWebLweClient client) {
    }

    public void setDownloadListener(SemDownloadListener listener) {
    }
}
