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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.SurfaceTexture;
import android.icu.util.TimeZone;
import android.net.Uri;
import android.os.Handler;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.TextureView;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;

import com.samsung.android.lwe.SemLweDownloadListener;
import com.samsung.android.lwe.SemLweWebResourceError;
import com.samsung.android.lwe.SemLweWebResourceRequest;
import com.samsung.android.lwe.SemLweWebSettings;
import com.samsung.android.lwe.SemLweWebView;
import com.samsung.android.lwe.SemLweWebViewClient;
import com.samsung.android.lwe.SemLweWebLweClient;

import java.io.File;
import java.lang.reflect.Method;
import java.util.Locale;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

public class LweWebViewImpl implements LweWebView {
    private static String sTag = "LweWebViewImpl";

    static {
        try {
            System.loadLibrary("crypto");
            System.loadLibrary("ssl");
            try {
                System.loadLibrary("icudata");
                System.loadLibrary("icuuc");
                System.loadLibrary("icui18n");
            } catch(UnsatisfiedLinkError e) {
                // ignore
                // runtime icu binding can occur this error
            }
            System.loadLibrary("cairo");
            System.loadLibrary("lwe");
        } catch (Exception e) {
            Log.e(sTag, "Cannot load: lwe.so");
            e.printStackTrace();
        }
    }

    public enum ImeComposingStatus {
        NORMAL,
        COMPOSING_START,
        COMPOSING_END
    }

    private final static String sLocale = Locale.getDefault().toLanguageTag();
    private final static String sTimezone = TimeZone.getDefault().getDisplayName();
    private final static String[] allowedMimetypes = {"text/html", "text/plain"};
    private final static String[] allowedEncodings = {"UTF-8", "utf8"};

    private float mDpr = 1;
    private long mWebViewInternalHandle;
    private int mWindowWidth;
    private int mWindowHeight;

    private SemLweWebViewClient mWebViewClient = null;
    private SemLweWebLweClient mWebLweClient = null;
    private SemLweDownloadListener mDownloadListener = null;
    private SemLweWebSettings mWebSettings = null;

    private ImeComposingStatus mComposingStatus = ImeComposingStatus.NORMAL;
    private String mIMEComposingStr = null;
    private SemLweWebView mLWEView = null;
    private InputMethodManager mIMM = null;

    private EGL10 mEgl;
    private EGLDisplay mEglDisplay;
    private EGLContext mEglContext;
    private EGLSurface mEglSurface;

    static class ErrorConverter {
        public static int covertErrorCode(int lweErrorCode) {
            switch (lweErrorCode) {
                case 2:
                    return SemLweWebViewClient.ERROR_HOST_LOOKUP;
                case 3:
                    return SemLweWebViewClient.ERROR_UNSUPPORTED_AUTH_SCHEME;
                case 4:
                    return SemLweWebViewClient.ERROR_AUTHENTICATION;
                case 5:
                    return SemLweWebViewClient.ERROR_PROXY_AUTHENTICATION;
                case 6:
                    return SemLweWebViewClient.ERROR_CONNECT;
                case 7:
                    return SemLweWebViewClient.ERROR_IO;
                case 8:
                    return SemLweWebViewClient.ERROR_TIMEOUT;
                case 9:
                    return SemLweWebViewClient.ERROR_REDIRECT_LOOP;
                case 10:
                    return SemLweWebViewClient.ERROR_UNSUPPORTED_SCHEME;
                case 11:
                    return SemLweWebViewClient.ERROR_FAILED_SSL_HANDSHAKE;
                case 12:
                    return SemLweWebViewClient.ERROR_BAD_URL;
                case 13:
                    return SemLweWebViewClient.ERROR_FILE;
                case 14:
                    return SemLweWebViewClient.ERROR_FILE_NOT_FOUND;
                case 15:
                    return SemLweWebViewClient.ERROR_TOO_MANY_REQUESTS;
                default:
                    return SemLweWebViewClient.ERROR_UNKNOWN;
            }
        }

        public static String covertErrorDescription(int lweErrorCode) {
            switch (lweErrorCode) {
                case 2:
                    return "ERROR_HOST_LOOKUP";
                case 3:
                    return "ERROR_UNSUPPORTED_AUTH_SCHEME";
                case 4:
                    return "ERROR_AUTHENTICATION";
                case 5:
                    return "ERROR_PROXY_AUTHENTICATION";
                case 6:
                    return "ERROR_CONNECT";
                case 7:
                    return "ERROR_IO";
                case 8:
                    return "ERROR_TIMEOUT";
                case 9:
                    return "ERROR_REDIRECT_LOOP";
                case 10:
                    return "ERROR_UNSUPPORTED_SCHEME";
                case 11:
                    return "ERROR_FAILED_SSL_HANDSHAKE";
                case 12:
                    return "ERROR_BAD_URL";
                case 13:
                    return "ERROR_FILE";
                case 14:
                    return "ERROR_FILE_NOT_FOUND";
                case 15:
                    return "ERROR_TOO_MANY_REQUESTS";
                default:
                    return "ERROR_UNKNOWN";
            }
        }
    }

    private int findConfigAttrib(EGLConfig config, int attribute, int defaultValue) {
        int[] value = new int[1];
        if (mEgl.eglGetConfigAttrib(mEglDisplay, config, attribute, value)) {
            return value[0];
        }
        return defaultValue;
    }

    private void initGLContext() {
        if (mEglSurface != null && mEglContext != null) {
            return;
        }

        final int RED_SIZE = 8;
        final int GREEN_SIZE = 8;
        final int BLUE_SIZE = 8;
        final int ALPHA_SIZE = 8;

        final int DEPTH_SIZE = 0;
        final int STENCIL_SIZE = 1;
        final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

        mEgl = (EGL10) EGLContext.getEGL();
        mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
            throw new RuntimeException("eglGetDisplay failed");
        }

        int[] version = new int[2];
        if (!mEgl.eglInitialize(mEglDisplay, version)) {
            throw new RuntimeException("eglInitialize failed");
        }

        int[] configSpec = {
                EGL10.EGL_RED_SIZE, RED_SIZE, EGL10.EGL_GREEN_SIZE, GREEN_SIZE, EGL10.EGL_BLUE_SIZE, BLUE_SIZE,
                EGL10.EGL_ALPHA_SIZE, ALPHA_SIZE, EGL10.EGL_DEPTH_SIZE, DEPTH_SIZE, EGL10.EGL_STENCIL_SIZE, STENCIL_SIZE,
                EGL10.EGL_NONE
        };

        int[] value = new int[1];
        if (!mEgl.eglChooseConfig(mEglDisplay, configSpec, null, 1, value)) {
            throw new IllegalArgumentException("eglChooseConfig failed");
        }
        EGLConfig[] configs = new EGLConfig[value[0]];
        if (!mEgl.eglChooseConfig(mEglDisplay, configSpec, configs, value[0], value)) {
            throw new IllegalArgumentException("eglChooseConfig failed");
        }

        EGLConfig eglConfig = configs[0];
        for (EGLConfig c : configs) {
            int depth = findConfigAttrib(c, EGL10.EGL_DEPTH_SIZE, 0);
            int stencil = findConfigAttrib(c, EGL10.EGL_STENCIL_SIZE, 0);

            if ((depth >= DEPTH_SIZE) && (stencil >= STENCIL_SIZE)) {
                int r = findConfigAttrib(c, EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib(c, EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib(c, EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib(c, EGL10.EGL_ALPHA_SIZE, 0);
                if ((r == RED_SIZE) && (g == GREEN_SIZE)
                        && (b == BLUE_SIZE) && (a == ALPHA_SIZE)) {
                    eglConfig = c;
                    break;
                }
            }
        }

        int[] attribList = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL10.EGL_NONE };

        mEglContext = mEgl.eglCreateContext(mEglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attribList);
        mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, eglConfig, mLWEView.getSurfaceTexture(), null);
    }

    private void destroyGLContext() {
        mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
        mEgl.eglDestroyContext(mEglDisplay, mEglContext);
        mEgl.eglTerminate(mEglDisplay);
        mEglDisplay = null;
        mEglContext = null;
        mEglSurface = null;
    }

    public long getWebViewInternalHandle() {
        return mWebViewInternalHandle;
    }

    public LweWebViewImpl() {
    }

    public InputConnection getInputConnectionInstance(View view){
        return onCreateInputConnection(view);
    }

    public InputConnection onCreateInputConnection(View view) {
        return new ImeInputConnection(view);
    }

    @Override
    public void onVisibilityChanged(View changedView, int visibility) {
        if (mLWEView == changedView && mWebViewInternalHandle != 0) {
            if (visibility == 0) {
                resume(mWebViewInternalHandle);
            } else {
                pause(mWebViewInternalHandle);
            }
        }
    }

    public class ImeInputConnection extends BaseInputConnection {
        public ImeInputConnection(View view) {
            super(view, true);
            mComposingStatus = ImeComposingStatus.NORMAL;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            if (mWebViewInternalHandle != 0) {
                int keyCode = (int) text.toString().charAt(0);
                // only ascii printable
                if ((32 <= keyCode) && (keyCode <= 126)) {
                    dispatchKeyDown(mWebViewInternalHandle, keyCode, 0);
                    dispatchKeyUp(mWebViewInternalHandle, keyCode, 0);
                    // dispatchKeyPress(mWebViewInternalHandle,keyCode,0);
                }
            }
            return super.commitText(text, newCursorPosition);
        }

        @Override
        public boolean setComposingText(CharSequence text, int newCursorPosition) {
            if (mWebViewInternalHandle != 0) {
                String newText = text.toString();
                mIMEComposingStr = newText;
                if (mComposingStatus == ImeComposingStatus.NORMAL) {
                    mIMEComposingStr = newText;
                    dispatchCompositionStart(mWebViewInternalHandle, newText);
                    mComposingStatus = ImeComposingStatus.COMPOSING_START;
                }
                dispatchCompositionUpdate(mWebViewInternalHandle, newText);
            }

            return super.setComposingText(text, newCursorPosition);
        }

        @Override
        public boolean finishComposingText() {
            if (mWebViewInternalHandle != 0) {
                if (mComposingStatus ==
                        ImeComposingStatus.COMPOSING_START) {
                    dispatchCompositionEnd(mWebViewInternalHandle,
                            mIMEComposingStr);
                    mComposingStatus = ImeComposingStatus.NORMAL;
                    mIMEComposingStr = null;
                }
            }
            return super.finishComposingText();
        }
    }

    public String getDefaultUserAgent(Context context) {
        return getDefaultUserAgent();
    }

    public String getUserAgentString() {
        if (mWebViewInternalHandle != 0) {
            return getUserAgentString(mWebViewInternalHandle);
        }
        return getDefaultUserAgent();
    }

    public int getCacheMode() {
        if (mWebViewInternalHandle != 0) {
            return getCacheMode(mWebViewInternalHandle);
        }
        return SemLweWebSettings.LOAD_DEFAULT;
    }

    public void setUserAgentString(String userAgent) {
        if (mWebViewInternalHandle != 0) {
            setUserAgentString(mWebViewInternalHandle, userAgent);
        }
    }

    public void setCacheMode(int mode) {
        if (mode == SemLweWebSettings.LOAD_DEFAULT ||
            mode == SemLweWebSettings.LOAD_NO_CACHE) {
            if (mWebViewInternalHandle != 0) {
                setCacheMode(mWebViewInternalHandle, mode);
            }
        }
    }

    public void setDefaultFontSize(int size) {
        if (mWebViewInternalHandle != 0) {
            setDefaultFontSize(mWebViewInternalHandle, size);
        }
    }

    public int getDefaultFontSize() {
        if (mWebViewInternalHandle != 0) {
            return getDefaultFontSize(mWebViewInternalHandle);
        }
        return 0;
    }

    static float roundToHalf(float d) {
        return Math.round(d * 2) / 2.0f;
    }

    public void initWebView(final View appView) {
        if (appView instanceof SemLweWebView) {
            mLWEView = (SemLweWebView)appView;
        } else {
            return;
        }

        Context appContext = mLWEView.getContext();

        mLWEView.setFocusable(true);
        mLWEView.setFocusableInTouchMode(true);

        mIMM = (InputMethodManager) appContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        mDpr = roundToHalf(appContext.getResources().getDisplayMetrics().xdpi / 150);
        String localStoragePath = appContext.getDataDir().getAbsolutePath() + "/Starfish-localStorage";
        String cookiePath = appContext.getDataDir().getAbsolutePath() + "/Starfish-cookie";

        File cachedDir = appContext.getCacheDir();
        String cachePath = "";
        if (cachedDir != null) {
            cachePath = cachedDir.getAbsolutePath() + "/Starfish-cache";
        } else {
            cachePath = "/data/local/tmp/Starfish-cache";
        }

        init();

        String initialUAString = getDefaultUserAgent();

        try {
            Os.setenv("HOME", appContext.getDataDir().getAbsolutePath(), false);
        } catch (ErrnoException e) {
            e.printStackTrace();
        }
        mWindowWidth = mWindowHeight = 1;
        mLWEView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {

            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                initGLContext();
                mWindowWidth = width;
                mWindowHeight = height;
                if (mWebViewInternalHandle != 0) {
                    resizeTo(mWebViewInternalHandle, mWindowWidth, mWindowHeight);
                    resume(mWebViewInternalHandle);
                }
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
                mWindowWidth = width;
                mWindowHeight = height;
                if (mWebViewInternalHandle != 0) {
                    resizeTo(mWebViewInternalHandle, mWindowWidth, mWindowHeight);
                    resume(mWebViewInternalHandle);
                }
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                if (mWebViewInternalHandle != 0) {
                    destroy(mWebViewInternalHandle);
                }
                mWebViewInternalHandle = 0;
                mWebViewClient = null;
                destroyGLContext();
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {
            }
        });


        mLWEView.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (mWebViewInternalHandle != 0) {
                    if (hasFocus) {
                        focus(mWebViewInternalHandle);
                    } else {
                        blur(mWebViewInternalHandle);
                    }
                }
            }
        });

        mLWEView.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                char keyValue = (char) event.getUnicodeChar();

                switch (keyCode) {
                    case KeyEvent.KEYCODE_DPAD_RIGHT:
                        keyValue = 22;
                        break;
                    case KeyEvent.KEYCODE_DPAD_LEFT:
                        keyValue = 21;
                        break;
                    case KeyEvent.KEYCODE_DPAD_UP:
                        keyValue = 20;
                        break;
                    case KeyEvent.KEYCODE_DPAD_DOWN:
                        keyValue = 19;
                        break;
                    case KeyEvent.KEYCODE_TAB:
                        keyValue = 18;
                        break;
                    case KeyEvent.KEYCODE_ENTER:
                        keyValue = 13; // ascii - CR
                        break;
                    case KeyEvent.KEYCODE_DEL:
                        keyValue = 8; // ascii - BS
                        break;
                    case KeyEvent.KEYCODE_POWER:
                    case KeyEvent.KEYCODE_BACK:
                    case KeyEvent.KEYCODE_SEARCH:
                    case KeyEvent.KEYCODE_CAMERA:
                    case KeyEvent.KEYCODE_VOLUME_UP:
                    case KeyEvent.KEYCODE_VOLUME_DOWN:
                    case KeyEvent.KEYCODE_VOLUME_MUTE:
                    case KeyEvent.KEYCODE_VOICE_ASSIST:
                        return false;
                }
                final char key = keyValue;
                final int eventAction = event.getAction();
                if (mWebViewInternalHandle != 0) {
                    if (eventAction == KeyEvent.ACTION_DOWN) {
                        dispatchKeyDown(mWebViewInternalHandle, key, 0);
                    } else if (eventAction == KeyEvent.ACTION_UP) {
                        dispatchKeyUp(mWebViewInternalHandle, key, 0);
                    }
                }
                return true;
            }
        });

        mLWEView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, final MotionEvent motionEvent) {
                view.performClick();
                int[] location = new int[2];
                view.getLocationOnScreen(location);
                float screenX = motionEvent.getRawX();
                float screenY = motionEvent.getRawY();
                final float viewX = screenX - location[0];
                final float viewY = screenY - location[1];

                if (mLWEView.hasFocus() == false) {
                    mLWEView.requestFocus();
                }


                if (mWebViewInternalHandle != 0) {
                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                        dispatchMouseDown(mWebViewInternalHandle, viewX, viewY);
                    } else if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                    } else if (motionEvent.getAction() == MotionEvent.ACTION_CANCEL) {
                        dispatchMouseMove(mWebViewInternalHandle, viewX, viewY, true, false);
                    } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
                        dispatchMouseMove(mWebViewInternalHandle, viewX, viewY, true, false);
                    }
                }
                return true;
            }
        });

        mWebViewInternalHandle =
                create(mWindowWidth, mWindowHeight, mDpr,
                        initialUAString, sLocale, sTimezone,
                        localStoragePath, cookiePath, cachePath);
    }

    private void showDropdownMenu(final String[] list, final int checkedPosition) {
        Handler handler = new Handler(mLWEView.getContext().getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(mLWEView.getContext());
                builder.setSingleChoiceItems(list, checkedPosition,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                onDropdownMenuItemSelected(which);
                            }
                        });

                builder.show();
            }
        });
    }

    private void onDropdownMenuItemSelected(int position) {
        if (mWebViewInternalHandle != 0) {
            onDropdownMenuItemSelected(mWebViewInternalHandle, position);
        }
    }

    private void showAlert(final String title, final String message) {
        Handler handler = new Handler(mLWEView.getContext().getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(mLWEView.getContext());
                builder.setTitle(title);
                builder.setMessage(message);
                builder.setPositiveButton("OK", null);
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        });
    }


    // Listener called from native code
    private void onLoadResource(String url) {
        if (mWebViewClient != null) {
            mWebViewClient.onLoadResource(mLWEView, url);
        }
    }

    private void onReceivedError(int errorCode, String url) {
        if (mWebViewClient != null) {
            class MyWebResourceRequestImpl implements SemLweWebResourceRequest {
                String mUrl;

                public MyWebResourceRequestImpl(String url) {
                    mUrl = url;
                }

                public Uri getUrl() {
                    return Uri.parse(mUrl);
                }
            }

            mWebViewClient.onReceivedError(mLWEView, new MyWebResourceRequestImpl(url),
                    new SemLweWebResourceError(ErrorConverter.covertErrorCode(errorCode),
                                            ErrorConverter.covertErrorDescription(errorCode)));
        }
    }

    private void onPageFinished(String url) {
        if (mWebViewClient != null) {
            mWebViewClient.onPageFinished(mLWEView, url);
        }
    }

    private void onPageStarted(String url) {
        if (mWebViewClient != null) {
            mWebViewClient.onPageStarted(mLWEView, url, null);
        }
    }

    private boolean shouldOverrideUrlLoading(String request) {
        if (mWebViewClient != null) {
            class MyWebResourceRequestImpl implements SemLweWebResourceRequest {
                String mUrl;

                public MyWebResourceRequestImpl(String url) {
                    mUrl = url;
                }

                public Uri getUrl() {
                    return Uri.parse(mUrl);
                }
            }

            return mWebViewClient.shouldOverrideUrlLoading(mLWEView,
                    new MyWebResourceRequestImpl(request));
        }
        return false;
    }

    private void onProgressChanged(int newProgres) {
        if (mWebLweClient != null) {
            mWebLweClient.onProgressChanged(mLWEView, newProgres);
        }
    }

    private void onDownloadStart(String url, String userAgent, String contentDisposition,
                                 String mimetype, long contentLength) {
        if (mDownloadListener != null) {
            mDownloadListener.onDownloadStart(url, userAgent, contentDisposition,
                    mimetype, contentLength);
        }
    }

    // public boolean onShowFileChooser(WebView webView, ValueCallback<Uri[]> Uris)

    private void showSoftKeyboard() {
        if (mLWEView != null) {
            if (mIMM == null) {
                mIMM = (InputMethodManager)mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            }
            mIMM.showSoftInput(mLWEView, InputMethodManager.SHOW_IMPLICIT);
            mComposingStatus = ImeComposingStatus.NORMAL;
        }
    }

    private void hideSoftKeyboard() {
        if (mLWEView != null && mIMM != null) {
            mIMM = (InputMethodManager)mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            mIMM.hideSoftInputFromWindow(mLWEView.getWindowToken(), 0);
            mComposingStatus = ImeComposingStatus.NORMAL;
        }
    }

    public void loadUrl(final String url) {
        if (url == null) {
            return;
        }
        loadUrl(mWebViewInternalHandle, url);
    }

    public String getUrl() {
        if (mWebViewInternalHandle != 0) {
            return getUrl(mWebViewInternalHandle);
        }
        return null;
    }

    public void loadData(String data, String mimeType, String encoding) {
        if (mWebViewInternalHandle != 0 && data != null && isSupportedMimeType(mimeType) && isSupportedEncoding(encoding)) {
            loadData(mWebViewInternalHandle, data);
        }
    }

    public void reload() {
        if (mWebViewInternalHandle != 0) {
            reload(mWebViewInternalHandle);
        }
    }

    public void stopLoading() {
        if (mWebViewInternalHandle != 0) {
            stopLoading(mWebViewInternalHandle);
        }
    }

    public void goBack() {
        if (mWebViewInternalHandle != 0) {
            goBack(mWebViewInternalHandle);
        }
    }

    public void goForward() {
        if (mWebViewInternalHandle != 0) {
            goForward(mWebViewInternalHandle);
        }
    }

    public boolean canGoBack() {
        if (mWebViewInternalHandle != 0) {
            return canGoBack(mWebViewInternalHandle);
        }
        return false;
    }

    public boolean canGoForward() {
        if (mWebViewInternalHandle != 0) {
            return canGoForward(mWebViewInternalHandle);
        }
        return false;
    }


    public void addJavascriptInterface(final Object object, final String name) {
        if ((object == null) || (name == null)) {
            return;
        }
        if (mWebViewInternalHandle != 0) {
            Method[] methods = object.getClass().getMethods();
            for (Method m : methods) {
                if (m.isAnnotationPresent(JavascriptInterface.class)) {
                    if (m.getReturnType().toString().equals("class java.lang.String") &&
                            m.getParameterTypes()[0].toString().equals(
                                    "class java.lang.String")) {
                        addJavascriptInterface(mWebViewInternalHandle, name,
                                m.getName(), object);
                    } else {
                        Log.e(sTag, "addJavascriptInterface : invalid signature");
                    }
                }
            }
        }
    }

    public void removeJavascriptInterface(final String name) {
        if (name == null) {
            return;
        }
        if (mWebViewInternalHandle != 0) {
            removeJavascriptInterface(mWebViewInternalHandle, name);
        }
    }

    public void clearCache(boolean includeDiskFiles) {
        if (mWebViewInternalHandle != 0) {
            clearCache(mWebViewInternalHandle);
        }
    }

    public void evaluateJavascript(final String script,
                                   final ValueCallback<String> resultCallback) {
        if ((script == null) || (resultCallback == null)) {
            return;
        }
        if (mWebViewInternalHandle != 0) {
            String resultStr = evaluateJavaScript(mWebViewInternalHandle, script);
            resultCallback.onReceiveValue(resultStr);
        }
    }

    public void clearHistory() {
        if (mWebViewInternalHandle != 0) {
            clearHistory(mWebViewInternalHandle);
        }
    }

    private void glMakeCurrent() {
        if (canUseGL()) {
            mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
        }
    }

    private void glSwapBuffers() {
        if (canUseGL()) {
            mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);
        }
    }

    private boolean canUseGL()
    {
        return mEgl != null && mEglDisplay != null;
    }

    public SemLweWebSettings getSettings() {
        if (mWebSettings == null) {
            mWebSettings = new SemLweWebSettings(this);
        }
        return mWebSettings;
    }

    public void setWebViewClient(SemLweWebViewClient client) {
        mWebViewClient = client;
    }

    public void setWebLweClient(SemLweWebLweClient client) {
        mWebLweClient = client;
    }

    public void setDownloadListener(SemLweDownloadListener listener) {
        mDownloadListener = listener;
    }

    boolean isSupportedMimeType(String mimeType) {
        for (String str : allowedMimetypes) {
            if (mimeType.equalsIgnoreCase(str)) {
                return true;
            }
        }
        return false;
    }

    boolean isSupportedEncoding(String encoding) {
        for (String str : allowedEncodings) {
            if (encoding.equalsIgnoreCase(str)) {
                return true;
            }
        }
        return false;
    }


    // Following methods are internal use only
    native private void loadUrl(long starfish, String url);
    native private void loadData(long starfish, String data);
    native private long create(int width, int height,
                               float devicePixelRatio, String userAgentString, String locale,
                               String timezoneID, String localstoragePath, String cookiePath,
                               String cachePath);
    native private void destroy(long starfish);
    native private void goBack(long starfish);
    native private void goForward(long starfish);
    native private boolean canGoBack(long starfish);
    native private boolean canGoForward(long starfish);
    native private void reload(long starfish);
    native private void stopLoading(long starfish);
    native private void clearHistory(long starfish);
    native private void clearCache(long starfish);
    native private void pause(long starfish);
    native private void resume(long starfish);
    native private void focus(long starfish);
    native private void blur(long starfish);
    native private void addJavascriptInterface(long starfish, String objectName,
                                               String functionName, Object instance);
    native private void removeJavascriptInterface(long starfish, String objectName);
    native private String evaluateJavaScript(long starfish, String data);
    native private String getDefaultUserAgent();
    native public String getUrl(long starfish);


    // Accessed by SemWebSettings
    native public void setUserAgentString(long starfish, String userAgent);
    native public String getUserAgentString(long starfish);
    native public void setCacheMode(long starfish, int mode);
    native public int getCacheMode(long starfish);
    native public void setDefaultFontSize(long starfish, int size);
    native public int getDefaultFontSize(long starfish);

    static native private void init();
    static native private void resizeTo(long starfish, int width, int height);
    static native private void dispatchMouseDown(long starfish, float x, float y);
    static native private void dispatchMouseMove(long starfish, float x, float y,
                                                 boolean isLButtonPressed,
                                                 boolean isRButtonPressed);
    static native private void dispatchMouseUp(long starfish, float x, float y);
    static native private void dispatchKeyDown(long starfish, int keyValue, int modifier);
    static native private void dispatchKeyUp(long starfish, int keyValue, int modifier);
    static native private void dispatchKeyPress(long starfish, int keyValue, int modifier);
    static native private void dispatchCompositionStart(long starfish, String Value);
    static native private void dispatchCompositionUpdate(long starfish, String Value);
    static native private void dispatchCompositionEnd(long starfish, String Value);
    native private void onDropdownMenuItemSelected(long starfish, int position);

}
