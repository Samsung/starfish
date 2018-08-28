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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.webkit.DownloadListener;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class LweWebViewImpl implements LweWebView{
    static {
        try{
            System.loadLibrary("lightweightwebengine");
        }catch(final UnsatisfiedLinkError e){}
    }
    public enum ImeComposingStatus {
        NORMAL,
        COMPOSING_START,
        COMPOSING_END
    }

    static class TimerData {
        Runnable runnable;
        int fn;
        int data;
    }

    static HashMap<String, TimerData> sTimerMap = new HashMap<>();
    static HashMap<String, TimerData> sIdlerMap = new HashMap<>();
    static AtomicInteger sTimerUid = new AtomicInteger(0);

    protected static String sTag = "StarFish";
    protected static float sDpr = 1;

    protected static String sLocale = "ko-KR";
    protected static String sTimezone = "Asia/Seoul";
    protected static String sLocalStoragePath;
    protected static String sCookiePath;
    protected static String sCachePath;

    protected static Handler sWebViewHandler;
    protected static Integer sWebViewThreadLocker = new Integer(0);
    protected static Looper sWebViewThreadLooper;
    protected static Thread sWebViewThread;

    protected Bitmap mScreenBuffer;
    protected long mWebViewInternalHandle;

    protected int mWindowWidth;
    protected int mWindowHeight;

    private boolean mCanGoBack = false;
    private boolean mCanGoForward = false;
    private String mCurrentURL = null;
    private SemWebViewClient mWebViewClient = null;
    private SemDownloadListener mDownloadListener = null;

    private int mCacheMode = SemWebSettings.LOAD_DEFAULT;
    private String mDefaultUserAgent = null;
    private String mUserAgentString = null;
    private LweWebViewImpl.ImeComposingStatus mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
    private String mIMEComposingStr = null;
    private SemWebView mLWEView = null;
    private InputMethodManager mIMM = null;

    public InputConnection getInputConnectionInstance(View view){
        return new ImeInputConnection(view);
    }

    public class ImeInputConnection extends BaseInputConnection {
        public ImeInputConnection(View view) {
            super(view, true);
            mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            final String newText = text.toString();
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                int keyCode = (int) newText.charAt(0);
                                // only ascii printable
                                if ((32 <= keyCode) && (keyCode <= 126)) {
                                    dispatchKeyDown(mWebViewInternalHandle, keyCode, 0);
                                    dispatchKeyUp(mWebViewInternalHandle, keyCode, 0);
                                    // dispatchKeyPress(mWebViewInternalHandle,keyCode,0);
                                }
                            }
                        }
                    });
                }
            }
            return super.commitText(text, newCursorPosition);
        }

        @Override
        public boolean setComposingText(CharSequence text, int newCursorPosition) {
            final String newText = text.toString();
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                mIMEComposingStr = newText.toString();
                                if (mComposingStatus == LweWebViewImpl.ImeComposingStatus.NORMAL) {
                                    mIMEComposingStr = newText.toString();
                                    dispatchCompositionStart(mWebViewInternalHandle,
                                            newText.toString());
                                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.COMPOSING_START;
                                }
                                dispatchCompositionUpdate(mWebViewInternalHandle,
                                        newText.toString());
                            }
                        }
                    });
                }
            }

            return super.setComposingText(text, newCursorPosition);
        }

        @Override
        public boolean finishComposingText() {
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null && mIMEComposingStr != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                if (mComposingStatus ==
                                        LweWebViewImpl.ImeComposingStatus.COMPOSING_START) {
                                    dispatchCompositionEnd(mWebViewInternalHandle,
                                            mIMEComposingStr);
                                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
                                    mIMEComposingStr = null;
                                }
                            }
                        }
                    });
                }
            }
            return super.finishComposingText();
        }

    }

    public String getDefaultUA(){
        return mDefaultUserAgent;
    }

    public String getUA(){
        return mUserAgentString;
    }

    public int getCacheModeValue(){
        return mCacheMode;
    }

    class StateChangeListener implements View.OnAttachStateChangeListener {
        @Override
        public void onViewAttachedToWindow(View v) {
        }

        @Override
        public void onViewDetachedFromWindow(View v) {
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                destroy(mWebViewInternalHandle);
                            }
                            mWebViewInternalHandle = 0;
                            mWebViewClient = null;
                        }
                    });
                }
            }
        }
    }

    public void initWebView(final View appView){

        mLWEView = (SemWebView)appView;
        Context appContext = mLWEView.getContext();
        mIMM = (InputMethodManager) appContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        sDpr = appContext.getResources().getDisplayMetrics().xdpi / 150;
        sLocalStoragePath = appContext.getDataDir().getAbsolutePath() + "/StarFish-localStorage";
        sCookiePath = appContext.getDataDir().getAbsolutePath() + "/StarFish-cookie";
        sCachePath = appContext.getCacheDir().getAbsolutePath() + "/StarFish-cache";
        init();
        synchronized (sWebViewThreadLocker) {
            mDefaultUserAgent = getDefaultUserAgent();
            if (mUserAgentString == null) {
                mUserAgentString = mDefaultUserAgent;
            }
        }
        mLWEView.getHolder().setFormat(PixelFormat.RGBA_8888);
        mLWEView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder surfaceHolder) {
            }

            @Override
            public void surfaceChanged(SurfaceHolder surfaceHolder, int i, final int w,
                                       final int h) {
                synchronized (sWebViewThreadLocker) {
                    mWindowWidth = w;
                    mWindowHeight = h;
                    mScreenBuffer = Bitmap.createBitmap(mWindowWidth, mWindowHeight,
                                                        Bitmap.Config.ARGB_8888);
                    if (sWebViewHandler != null) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0 && mScreenBuffer != null) {
                                    updateBuffer(mWebViewInternalHandle, mScreenBuffer,
                                                 mWindowWidth, mWindowHeight,
                                                 mWindowWidth * 4);
                                    resume(mWebViewInternalHandle);
                                }
                            }
                        });
                    }
                }
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
                synchronized (sWebViewThreadLocker) {
                    mScreenBuffer = null;
                    if (sWebViewHandler != null) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    pause(mWebViewInternalHandle);
                                }
                            }
                        });
                    }
                }
            }
        });

        synchronized (sWebViewThreadLocker) {
            if (sWebViewThread == null) {
                sWebViewThread = new Thread() {

                    @Override
                    public void run() {
                        Looper.prepare();

                        sWebViewHandler = new Handler() {
                            public void handleMessage(Message msg) {
                                // process incoming messages here
                            }
                        };

                        synchronized (sWebViewThreadLocker) {
                            sWebViewThreadLooper = Looper.myLooper();
                        }

                        Looper.loop();
                        sWebViewHandler = null;
                    }
                };
                sWebViewThread.setPriority(Thread.MAX_PRIORITY);
                sWebViewThread.start();
            }
        }

        mLWEView.post(new Runnable() {
            @Override
            public void run() {
                synchronized (sWebViewThreadLocker) {
                    if (mLWEView.getWidth() == 0 || sWebViewHandler == null) {
                        final View webview = mLWEView;
                        webview.post(this);
                        return;
                    }
                }
                mWindowWidth = mLWEView.getWidth();
                mWindowHeight = mLWEView.getHeight();

            }
        });

        mLWEView.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                int keyValue = 0;
                switch (keyCode) {
                    case KeyEvent.KEYCODE_ENTER:
                        keyValue = 13; // ascii - CR
                        break;
                    case KeyEvent.KEYCODE_DEL:
                        keyValue = 8; // ascii - BS
                        break;
                }
                final int key = keyValue;
                final int eventAction = event.getAction();
                synchronized (sWebViewThreadLocker) {
                    if (sWebViewHandler != null && key != 0) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    if (eventAction == KeyEvent.ACTION_DOWN) {
                                        dispatchKeyDown(mWebViewInternalHandle, key, 0);
                                    } else if (eventAction == KeyEvent.ACTION_UP) {
                                        dispatchKeyUp(mWebViewInternalHandle, key, 0);
                                    }
                                }
                            }
                        });
                    }
                }
                return true;
            }
        });

        mLWEView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, final MotionEvent motionEvent) {

                int[] location = new int[2];
                view.getLocationOnScreen(location);
                float screenX = motionEvent.getRawX();
                float screenY = motionEvent.getRawY();
                final float viewX = screenX - location[0];
                final float viewY = screenY - location[1];

                synchronized (sWebViewThreadLocker) {
                    if (sWebViewHandler != null) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                                        dispatchMouseDown(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() ==
                                               MotionEvent.ACTION_CANCEL) {
                                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
                                        dispatchMouseMove(mWebViewInternalHandle, viewX, viewY,
                                                          true, false);
                                    }
                                }
                            }
                        });
                    }
                }
                return true;
            }
        });
        mLWEView.addOnAttachStateChangeListener(new StateChangeListener());
    }


    static private int registerTimerMap(final HashMap<String, TimerData> map, int ms, int fn,
                                        int data) {
        final int mms = ms;
        final int uid = sTimerUid.addAndGet(1);
        Runnable r = new Runnable() {
            @Override
            public void run() {
                if (map.containsKey(uid + "")) {
                    TimerData td = map.get(uid + "");
                    // Log.e(sTag, String.format("java serviceQueueTimer %d %d %d called", uid,
                    // (int)td.fn, (int)td.data));
                    boolean ret = serviceQueueTimer(uid, td.fn, td.data);
                    // Log.e(sTag, String.format("java serviceQueueTimer %d %d %d call ended", uid,
                    // (int)td.fn, (int)td.data));
                    if (ret) {
                        sWebViewHandler.postDelayed(this, mms);
                    } else {
                        if (map.containsKey(uid + "")) {
                            map.remove(uid + "");
                        }
                    }

                }
            }
        };
        TimerData td = new TimerData();
        td.runnable = r;
        td.fn = fn;
        td.data = data;

        synchronized (map) {
            final long current = SystemClock.uptimeMillis();
            map.put(uid + "", td);
            sWebViewHandler.postDelayed(r, ms);
        }
        return uid;
    }

    static private int startIdler(int ms, int fn, int data) {
        return registerTimerMap(sIdlerMap, ms, fn, data);
    }

    static private int startTimer(int ms, int fn, int data) {
        return registerTimerMap(sTimerMap, ms, fn, data);
    }

    static private void cancelTimer(int uid) {
        if (sTimerMap.containsKey(uid + "")) {
            TimerData r = sTimerMap.get(uid + "");
            sWebViewHandler.removeCallbacks(r.runnable);
            sTimerMap.remove(uid + "");
        }
    }

    static private void cancelIdler(int uid) {
        if (sIdlerMap.containsKey(uid + "")) {
            TimerData r = sIdlerMap.get(uid + "");
            sWebViewHandler.removeCallbacks(r.runnable);
            sIdlerMap.remove(uid + "");
        }
    }

    static private void runAllRemainingIdler() {
        while (!sIdlerMap.isEmpty()) {
            String key = sIdlerMap.keySet().iterator().next();
            TimerData r = sIdlerMap.get(key);
            sWebViewHandler.removeCallbacks(r.runnable);
            r.runnable.run();
            sIdlerMap.remove(key);
        }
    }

    private void flushRendering(int updatedX, int updatedY, int updatedWidth, int updatedHeight) {
        synchronized (sWebViewThreadLocker) {
            if (mScreenBuffer != null && mWebViewInternalHandle != 0) {
                Canvas canvas = ((SurfaceView)mLWEView).getHolder().lockCanvas();
                if (canvas != null) {
                    Paint paint = new Paint();
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
                    Rect updateRect = new Rect(0, 0, mWindowWidth, mWindowHeight);
                    canvas.drawBitmap(mScreenBuffer, updateRect, updateRect, paint);
                    mLWEView.getHolder().unlockCanvasAndPost(canvas);
                }
            }
        }
    }

    private void showDropdownMenu(String[] list, int checkedPosition) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mLWEView.getContext());
        builder.setSingleChoiceItems(list, checkedPosition,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        onDropdownMenuItemSelected(which);
                    }
                });

        builder.show();
    }

    private void onDropdownMenuItemSelected(int position) {
        final int positionIdx = position;
        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        onDropdownMenuItemSelected(mWebViewInternalHandle, positionIdx);
                    }
                }
            });
        }
    }

    private void showAlert(String title, String message) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mLWEView.getContext());
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setPositiveButton("OK", null);

        AlertDialog dialog = builder.create();
        dialog.show();
    }


    // Listener called from native code
    private void onLoadResource(String url) {
        if (mWebViewClient != null) {
            mWebViewClient.onLoadResource(mLWEView, url);
        }
    }

    private void onReceivedError(int errorCode, boolean canGoBack, boolean canGoForward) {
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if (mWebViewClient != null) {
            //TODO
            mWebViewClient.onReceivedError(mLWEView, new SemResourceError(errorCode, "NotSupported"));
        }
    }

    private void onPageFinished(String url, boolean canGoBack, boolean canGoForward) {
        mCurrentURL = url;
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if (mWebViewClient != null) {
            mWebViewClient.onPageFinished(mLWEView, url);
        }
    }

    private void onPageStarted(String url, boolean canGoBack, boolean canGoForward) {
        mCurrentURL = url;
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if (mWebViewClient != null) {
            mWebViewClient.onPageStarted(mLWEView, url);
        }
    }

    private boolean shouldOverrideUrlLoading(String request) {
        if (mWebViewClient != null) {
            return mWebViewClient.shouldOverrideUrlLoading(mLWEView, request);
        }
        return false;
    }

    private void onProgressChanged(int newProgres) {
        if (mWebViewClient != null) {
            mWebViewClient.onProgressChanged(mLWEView, newProgres);
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
        mLWEView.post(new Runnable() {
            @Override
            public void run() {
                if (mLWEView != null) {
                    mLWEView.setFocusableInTouchMode(true);
                    mLWEView.setFocusable(true);
                    if (mIMM == null) {
                        mIMM = (InputMethodManager) mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    }
                    mIMM.showSoftInput(mLWEView, InputMethodManager.SHOW_IMPLICIT);
                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
                }
            }
        });
    }

    private void hideSoftKeyboard() {
        mLWEView.post(new Runnable() {
            @Override
            public void run() {
                if (mLWEView != null && mIMM != null) {
                    mLWEView.setFocusableInTouchMode(false);
                    mLWEView.setFocusable(false);
                    if (mIMM == null) {
                        mIMM = (InputMethodManager) mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    }
                    mIMM.hideSoftInputFromWindow(mLWEView.getWindowToken(), 0);
                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
                }
            }
        });
    }


    public void loadUrl(final String url) {
        if (url == null) {
            return;
        }

        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle == 0) {
                            mWebViewInternalHandle =
                                    create(mWindowWidth, mWindowHeight, sDpr,
                                            mUserAgentString, sLocale, sTimezone,
                                            sLocalStoragePath, sCookiePath, sCachePath);
                        }
                        loadUrl(mWebViewInternalHandle, url);
                    }
                });
            }
        }
    }
    public String getUrl() {
        return mCurrentURL;
    }

    public void loadData(String data) {
        if (data == null) {
            return;
        }

        final String htmlData = data;
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle == 0) {
                            mWebViewInternalHandle =
                                    create(mWindowWidth, mWindowHeight, sDpr, mUserAgentString, sLocale,
                                            sTimezone, sLocalStoragePath, sCookiePath, sCachePath);
                        }
                        loadData(mWebViewInternalHandle, htmlData);
                    }
                });
            }
        }
    }

    public void reload() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            reload(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void stopLoading() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            stopLoading(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void goBack() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            goBack(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void goForward() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            goForward(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public boolean canGoBack() {
        return mCanGoBack;
    }
    public boolean canGoForward() {
        return mCanGoForward;
    }


    public void addJavascriptInterface(final Object object, final String name) {
        if ((object == null) || (name == null)) {
            return;
        }

        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
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
                                    Log.d(sTag, "ERROR: addJavascriptInterface : invalid signature");
                                }
                            }
                        }
                    }
                }
            });
        }
    }

    public void removeJavascriptInterface(final String name) {
        if (name == null) {
            return;
        }

        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        removeJavascriptInterface(mWebViewInternalHandle, name);
                    }
                }
            });
        }
    }

    public void clearCache() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            clearCache(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void evaluateJavascript(final String script,
                                   final ValueCallback<String> resultCallback) {
        if ((script == null) || (resultCallback == null)) {
            return;
        }

        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        String resultStr = evaluateJavaScript(mWebViewInternalHandle, script);
                        if (resultCallback != null) {
                            resultCallback.onReceiveValue(resultStr);
                        }
                    }
                }
            });
        }
    }

    public void clearHistory() {
        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        clearHistory(mWebViewInternalHandle);
                    }
                }
            });
        }
    }

    public void setSettings(SemWebSettings settings) {
        if (settings == null) {
            return;
        }

        final String UA = mUserAgentString = settings.getUserAgentString();
        final int cacheMode = mCacheMode = settings.getCacheMode();
        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        setUserAgentString(mWebViewInternalHandle, UA);
                        setCacheMode(mWebViewInternalHandle, cacheMode);
                    }
                }
            });
        }
    }

    public void setWebViewClient(SemWebViewClient client) {
        mWebViewClient = client;
    }
    public void setDownloadListener(SemDownloadListener listener) {
        mDownloadListener = listener;
    }

    // Following methods are internal use only
    native private void loadUrl(long starFish, String url);
    native private void loadData(long starFish, String data);
    native private long create(int initialNaturalWidth, int initialNaturalHeight,
                               float devicePixelRatio, String userAgentString, String locale,
                               String timezoneID, String localstoragePath, String cookiePath,
                               String cachePath);
    native private void destroy(long starFish);
    native private void goBack(long starFish);
    native private void goForward(long starFish);
    native private void reload(long starFish);
    native private void stopLoading(long starFish);
    native private void clearHistory(long starFish);
    native private void clearCache(long starFish);
    native private void pause(long starFish);
    native private void resume(long starFish);
    native private void addJavascriptInterface(long starFish, String objectName,
                                               String functionName, Object instance);
    native private void removeJavascriptInterface(long starFish, String objectName);
    native private String evaluateJavaScript(long starFish, String data);
    native private String getDefaultUserAgent();
    native private void setUserAgentString(long starFish, String userAgent);
    native private void setCacheMode(long starFish, int mode);
    static native private void init();
    static native private void updateBuffer(long starFish, Bitmap b, int w, int h, int stride);
    static native private boolean serviceQueueTimer(int uid, int fn, int data);
    static native private void dispatchMouseDown(long starFish, float x, float y);
    static native private void dispatchMouseMove(long starFish, float x, float y,
                                                 boolean isLButtonPressed,
                                                 boolean isRButtonPressed);
    static native private void dispatchMouseUp(long starFish, float x, float y);
    static native private void dispatchKeyDown(long starFish, int keyValue, int modifier);
    static native private void dispatchKeyUp(long starFish, int keyValue, int modifier);
    static native private void dispatchKeyPress(long starFish, int keyValue, int modifier);
    static native private void dispatchCompositionStart(long starFish, String Value);
    static native private void dispatchCompositionUpdate(long starFish, String Value);
    static native private void dispatchCompositionEnd(long starFish, String Value);
    native private void onDropdownMenuItemSelected(long starFish, int position);


}