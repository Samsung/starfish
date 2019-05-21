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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;

import java.io.File;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;
import java.util.ArrayList;

public class LweWebViewImpl implements LweWebView {
    private static String sTag = "LweWebViewImpl";

    static {
        try {
            System.loadLibrary("lightweightwebengine.lwe.samsung");
        } catch (Exception e) {
            Log.e(sTag, "Cannot load: liblightweightwebengine.lwe.samsung.so");
            e.printStackTrace();
       }
    }

    public enum ImeComposingStatus {
        NORMAL,
        COMPOSING_START,
        COMPOSING_END
    }

    private final static String sLocale = "ko-KR";
    private final static String sTimezone = "Asia/Seoul";
    private final static String[] allowedMimetypes = {"text/html", "text/plain"};
    private final static String[] allowedEncodings = {"UTF-8", "utf8"};

    private float sDpr = 1;
    private long mWebViewInternalHandle;
    private Bitmap mScreenBuffer;
    private int mWindowWidth;
    private int mWindowHeight;
    private boolean mSurfaceIsReady;

    private SemWebViewClient mWebViewClient = null;
    private SemWebLweClient mWebLweClient = null;
    private SemDownloadListener mDownloadListener = null;
    private SemWebSettings mWebSettings = null;

    private LweWebViewImpl.ImeComposingStatus mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
    private String mIMEComposingStr = null;
    private SemWebView mLWEView = null;
    private InputMethodManager mIMM = null;

    private ArrayList<Pattern> mWhitelistedUrls = null;

    static class ErrorConverter {
        public static int covertErrorCode(int lweErrorCode) {
            switch (lweErrorCode) {
                case 2:
                    return SemWebViewClient.ERROR_HOST_LOOKUP;
                case 3:
                    return SemWebViewClient.ERROR_UNSUPPORTED_AUTH_SCHEME;
                case 4:
                    return SemWebViewClient.ERROR_AUTHENTICATION;
                case 5:
                    return SemWebViewClient.ERROR_PROXY_AUTHENTICATION;
                case 6:
                    return SemWebViewClient.ERROR_CONNECT;
                case 7:
                    return SemWebViewClient.ERROR_IO;
                case 8:
                    return SemWebViewClient.ERROR_TIMEOUT;
                case 9:
                    return SemWebViewClient.ERROR_REDIRECT_LOOP;
                case 10:
                    return SemWebViewClient.ERROR_UNSUPPORTED_SCHEME;
                case 11:
                    return SemWebViewClient.ERROR_FAILED_SSL_HANDSHAKE;
                case 12:
                    return SemWebViewClient.ERROR_BAD_URL;
                case 13:
                    return SemWebViewClient.ERROR_FILE;
                case 14:
                    return SemWebViewClient.ERROR_FILE_NOT_FOUND;
                case 15:
                    return SemWebViewClient.ERROR_TOO_MANY_REQUESTS;
                default:
                    return SemWebViewClient.ERROR_UNKNOWN;
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
            mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
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
                if (mComposingStatus == LweWebViewImpl.ImeComposingStatus.NORMAL) {
                    mIMEComposingStr = newText;
                    dispatchCompositionStart(mWebViewInternalHandle, newText);
                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.COMPOSING_START;
                }
                dispatchCompositionUpdate(mWebViewInternalHandle, newText);
            }

            return super.setComposingText(text, newCursorPosition);
        }

        @Override
        public boolean finishComposingText() {
            if (mWebViewInternalHandle != 0) {
                if (mComposingStatus ==
                        LweWebViewImpl.ImeComposingStatus.COMPOSING_START) {
                    dispatchCompositionEnd(mWebViewInternalHandle,
                            mIMEComposingStr);
                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
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
        return SemWebSettings.LOAD_DEFAULT;
    }

    public void setUserAgentString(String userAgent) {
        if (mWebViewInternalHandle != 0) {
            setUserAgentString(mWebViewInternalHandle, userAgent);
        }
    }

    public void setCacheMode(int mode) {
        if (mode == SemWebSettings.LOAD_DEFAULT ||
            mode == SemWebSettings.LOAD_NO_CACHE) {
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

    class StateChangeListener implements View.OnAttachStateChangeListener {
        @Override
        public void onViewAttachedToWindow(View v) {
        }

        @Override
        public void onViewDetachedFromWindow(View v) {
            if (mWebViewInternalHandle != 0) {
                destroy(mWebViewInternalHandle);
            }
            destoryBuffer();
            mWebViewInternalHandle = 0;
            mWebViewClient = null;
        }
    }

    public void initWebView(final View appView) {
        mSurfaceIsReady = false;
        if (appView instanceof SemWebView) {
            mLWEView = (SemWebView)appView;
        } else {
            return;
        }

        Context appContext = mLWEView.getContext();

        mLWEView.setFocusable(true);
        mLWEView.setFocusableInTouchMode(true);

        mIMM = (InputMethodManager) appContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        sDpr = appContext.getResources().getDisplayMetrics().xdpi / 150;
        String localStoragePath = appContext.getDataDir().getAbsolutePath() + "/Starfish-localStorage";
        String cookiePath = appContext.getDataDir().getAbsolutePath() + "/Starfish-cookie";
        String cachePath = "/data/local/tmp/Starfish-cache";
        File cachedDir = appContext.getCacheDir();
        if (cachedDir != null) {
            cachePath = cachedDir.getAbsolutePath() + "/Starfish-cache";
        }
        init();

        String initialUAString = getDefaultUserAgent();

        mWindowWidth = mWindowHeight = 1;
        mWebViewInternalHandle =
                create(mWindowWidth, mWindowHeight, sDpr,
                        initialUAString, sLocale, sTimezone,
                        localStoragePath, cookiePath, cachePath);
        mLWEView.getHolder().addCallback(
                new SurfaceHolder.Callback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        mSurfaceIsReady = true;
                        int width = mLWEView.getWidth();
                        int height = mLWEView.getHeight();
                        if (mWebViewInternalHandle != 0) {
                            if ((mWindowWidth != width || mWindowHeight != height)) {
                                resizeTo(mWebViewInternalHandle, width, height);
                            }
                            mWindowWidth = width;
                            mWindowHeight = height;
                            resume(mWebViewInternalHandle);
                        }
                    }

                    @Override
                    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                        if (mWebViewInternalHandle != 0) {
                            if ((mWindowWidth != width || mWindowHeight != height)) {
                                resizeTo(mWebViewInternalHandle, width, height);
                                mWindowWidth = width;
                                mWindowHeight = height;
                            }
                            resume(mWebViewInternalHandle);
                        }
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        mSurfaceIsReady = false;
                        if (mWebViewInternalHandle != 0) {
                            pause(mWebViewInternalHandle);
                        }
                        destoryBuffer();
                    }
                }
        );

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
        mLWEView.addOnAttachStateChangeListener(new StateChangeListener());
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
            mWebViewClient.onReceivedError(mLWEView, new WebResourceRequestImpl(url),
                    new SemWebResourceError(ErrorConverter.covertErrorCode(errorCode),
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
            return mWebViewClient.shouldOverrideUrlLoading(mLWEView,
                    new WebResourceRequestImpl(request));
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
            mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
        }
    }

    private void hideSoftKeyboard() {
        if (mLWEView != null && mIMM != null) {
            mIMM = (InputMethodManager)mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            mIMM.hideSoftInputFromWindow(mLWEView.getWindowToken(), 0);
            mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
        }
    }

    public void loadUrl(final String url) {
        if (url == null) {
            return;
        }

        ArrayList<Pattern> whitelistedUrls = whitelistedUrls();
        if (mWhitelistedUrls == null) {
            return;
        }

        boolean loaded = false;
        for (Pattern p : whitelistedUrls) {
            if (p.matcher(url).find()) {
                loadUrl(mWebViewInternalHandle, url);
                loaded = true;
            }
        }

        if (!loaded) {
            Log.e(sTag, "URL is not permitted. Please contact duddlf.choi@samsung.com to whitelist an URL.");
            onReceivedError(SemWebViewClient.ERROR_BAD_URL, url);
        }
    }

    private ArrayList<Pattern> whitelistedUrls() {
        if (mWhitelistedUrls != null) {
            return mWhitelistedUrls;
        }

        mWhitelistedUrls = new ArrayList<Pattern>();
        BufferedReader reader = null;
        String line = null;
        try {
            Resources res =
                    mLWEView.getContext().getPackageManager().getResourcesForApplication(SemWebView.PACKAGE_NAME);

            int rid = res.getIdentifier("whitelist", "raw", SemWebView.PACKAGE_NAME);
            if (rid == 0) {
                Log.e(sTag, "cannot locate whitelist.txt");
                return mWhitelistedUrls;
            }

            reader = new BufferedReader(new InputStreamReader(
                    res.openRawResource(rid)));
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }
                Pattern p = Pattern.compile(line);
                mWhitelistedUrls.add(p);
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(sTag, "package not found: " + SemWebView.PACKAGE_NAME);
            e.printStackTrace();
        } catch (PatternSyntaxException e) {
            Log.e(sTag, "invalid regex: " + line);
            e.printStackTrace();
        } catch (IOException e) {
            Log.e(sTag, "failed to read whitelist.txt");
            e.printStackTrace();
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (Exception e) {
            }
        }

        return mWhitelistedUrls;
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

    public SemWebSettings getSettings() {
        if (mWebSettings == null) {
            mWebSettings = new SemWebSettings(this);
        }
        return mWebSettings;
    }

    private Bitmap createBuffer() {
        mScreenBuffer = Bitmap.createBitmap(mWindowWidth, mWindowHeight, Bitmap.Config.ARGB_8888);
        return mScreenBuffer;
    }

    private void destoryBuffer() {
        if (mScreenBuffer != null) {
            mScreenBuffer.recycle();
            mScreenBuffer = null;
        }
    }

    private void onRendered(int x, int y, int width, int height) {
        if (mSurfaceIsReady) {
            Canvas canvas = null;
            try {
                canvas = mLWEView.getHolder().lockCanvas();
                if (canvas != null) {
                    Paint paint = new Paint();
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
                    Rect updateArea = new Rect(x, y, width, height);
                    canvas.drawBitmap(mScreenBuffer, updateArea, updateArea, paint);
                }
            } catch (Exception e) {
                Log.e(sTag, "failed to onRendered");
                e.printStackTrace();
            } finally {
                if (canvas != null) {
                    mLWEView.getHolder().unlockCanvasAndPost(canvas);
                }
            }
        }
    }

    public void setWebViewClient(SemWebViewClient client) {
        mWebViewClient = client;
    }

    public void setWebLweClient(SemWebLweClient client) {
        mWebLweClient = client;
    }

    public void setDownloadListener(SemDownloadListener listener) {
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
